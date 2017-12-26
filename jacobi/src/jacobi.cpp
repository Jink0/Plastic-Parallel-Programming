#include <thread>
#include <vector>
#include <map>
#include <sys/times.h>
#include <algorithm>

#include <general_utils.hpp>
#include <config_file_utils.hpp>
#include <kernels.hpp>



#ifdef MY_BARRIER
#define MB( x ) x
#pragma message "MY_BARRIER ACTIVE"
#else
#define MB( x )
#endif

#ifdef PTHREAD_BARRIER
#define PTB( x ) x
#pragma message "PTHREAD_BARRIER ACTIVE"
#else
#define PTB( x )
#endif

#ifdef BASIC_KERNEL_SMALL
#define BKS( x ) x
#pragma message "BASIC_KERNEL_SMALL ACTIVE"
#else
#define BKS( x )
#endif

#ifdef BASIC_KERNEL_LARGE
#define BKL( x ) x
#pragma message "BASIC_KERNEL_LARGE ACTIVE"
#else
#define BKL( x )
#endif

#ifdef VARY_KERNEL_LOAD
#define VRY( x ) x
#pragma message "VARY_KERNEL_LOAD ACTIVE"
#else
#define VRY( x )
#endif

#ifdef EXECUTE_KERNELS
#define EXK( x ) x
#pragma message "EXECUTE_KERNELS ACTIVE"
#else
#define EXK( x )
#endif

#ifdef CONVERGENCE_TEST
#define CVG( x ) x
#pragma message "CONVERGE_TEST ACTIVE"
#else
#define CVG( x )
#endif



// Each Worker computes values in one strip of the grids. The main worker loop does two computations to avoid copying from one grid to the other
void worker(uint32_t my_id, uint32_t stage);

// Initialize the grids (grid1 and grid2), set boundaries to 1.0 and interior points to 0.0
void initialize_grids();

// My implementation of a counter barrier
inline void my_barrier(uint32_t stage);

// Performs the jacobi kernel. Computes the average of the given point's four neighbors in the source grid and stores it in the target grid
inline void basic_kernel_small(std::vector<std::vector<double>>& src_grid, std::vector<std::vector<double>>& tgt_grid, uint32_t i, uint32_t j);

// Performs a larger version of the jacobi kernel. Computes average of the given point's 5x5 neighborhood in the source grid and stores it in the target grid
inline void basic_kernel_large(std::vector<std::vector<double>>& src_grid, std::vector<std::vector<double>>& tgt_grid, uint32_t i, uint32_t j);

// Executes the relevant kernels set by the experiment parameters
inline void execute_kernels(uint32_t stage, uint32_t i, uint32_t j);

// Simulate a convergence test. Computes the maximum difference in given strip and sets the global_max_difference variable
inline void convergence_test(uint32_t first, uint32_t last, uint32_t stage, uint32_t id);



// Set of pthread barriers (one for each stage)
std::vector<pthread_barrier_t> pthread_barriers;

// Mutex semaphore for my barrier
pthread_mutex_t my_barrier_mutex;

// Condition variable for leaving my barrier
pthread_cond_t go;

// Count of the number who have arrived at my barrier   
uint32_t num_arrived = 0;



// Experiment parameters
uint32_t num_runs, grid_size, num_stages, use_set_num_repeats;

// Stage parameters
std::vector<uint32_t> num_workers, num_iterations, set_pin_bool;
std::vector<std::vector<uint32_t>> kernels, kernel_durations, kernel_repeats, row_allocations;
std::vector<std::vector<std::vector<uint32_t>>> pinnings;

// Experiment data
std::vector<std::vector<double>> grid1, grid2;

// Used for convergence test
std::vector<std::vector<double>> global_max_difference;



// Border size of our grids
static uint32_t const border_size = 2;



int main(int argc, char *argv[]) {

	// Parse config
	std::map<std::string, std::string> config = parse_config(std::string(argv[1]));

	// Read config
	read_config(config);

	// Move into relevant folder and copy the config file
	move_and_copy("jacobi", argv[1]);
	
	// Print experiment parameters
	print_params();

	// Attempt to open/create output file
	FILE *output_stream = fopen((char*) "output", "w");

	if (output_stream == NULL) {
        // If we couldn't open the file, throw an error
        perror("Error, metric could not open file");
        exit(EXIT_FAILURE);
    }



	// Calculate row allocations
	for (uint32_t i = 0; i < num_stages; i++) {
		uint32_t quotient  = grid_size / num_workers.at(i);
		uint32_t remainder = grid_size % num_workers.at(i);

		std::vector<uint32_t> temp(num_workers.at(i) + 1, quotient);

		temp.at(0) = border_size;

		for (uint32_t j = 1; j < num_workers.at(i) + 1; j++) {
			if (remainder != 0) {
				temp.at(j) += 1;
				remainder  -= 1;
			}

			temp.at(j) += temp.at(j-1);
		}

		row_allocations.push_back(temp);
	}

	// Set global max difference vector size
	global_max_difference.resize(num_stages);

	// Set per stage global max difference vector sizes
	for (uint32_t i = 0; i < num_stages; i++) {
		global_max_difference.at(i).resize(num_workers.at(i));
	}



	// Calculate the max number of workers we will need
	uint32_t max_num_workers = *max_element(std::begin(num_workers), std::end(num_workers));

  	// Create thread handles
	std::vector<std::thread> threads(max_num_workers);

	// Initialize mutex and condition variable for my barrier
	pthread_mutex_init(&my_barrier_mutex, NULL);
	pthread_cond_init(&go, NULL);

	// Initialize pthread barriers
	for (uint32_t i = 0; i < num_stages; i++) {
		pthread_barrier_t b;
		pthread_barrier_init(&b, NULL, num_workers.at(i));
		pthread_barriers.push_back(b);
	}



	// Initialize run times sum for computing average
	uint32_t run_times_sum = 0;

	for (uint32_t r = 1; r < num_runs + 1; r++) {

		initialize_grids();

		// Record start time
		std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

		for (uint32_t stage = 0; stage < num_stages; stage++) {

			// Create workers
			for (uint32_t i = 0; i < num_workers.at(stage); i++) {
				threads.at(i) = std::thread(worker, i, stage);
			}

			// Join with workers
			for (uint32_t i = 0; i < num_workers.at(stage); i++) {
				threads.at(i).join();
			}
		}

		// Calculate time taken
		std::chrono::duration<double> diff = std::chrono::high_resolution_clock::now() - start;

		// Cast to millis
		std::chrono::milliseconds millis = std::chrono::duration_cast<std::chrono::milliseconds>(diff);

		// Print runtime
		print("\nRun ", r, "\nElapsed time: ", millis.count(), "ms\n");

		// Record runtime
		fputs((std::to_string(millis.count()) + "\n").c_str(), output_stream);

		run_times_sum += millis.count();
	}

	// Print average runtime
	print("\nAverage elapsed time over ", num_runs, " runs: ", run_times_sum / num_runs, "ms\n");
}



// Each Worker computes values in one strip of the grids. The main worker loop does two computations to avoid copying from one grid to the other
void worker(uint32_t my_id, uint32_t stage) {

	// Set our affinity
	force_affinity_set(pinnings.at(stage).at(my_id));

	// Determine first and last rows of my strip of the grids
	uint32_t first = row_allocations.at(stage).at(my_id);
	uint32_t last = row_allocations.at(stage).at(my_id + 1);

	// Create grid pointers
	std::vector<std::vector<double>>* src_grid = &grid1;
	std::vector<std::vector<double>>* tgt_grid = &grid2;

	for (uint32_t iter = 0; iter < num_iterations.at(stage); iter++) {

		// Update my points
		for (uint32_t i = first; i < last; i++) {
			for (uint32_t j = border_size; j < grid_size + border_size; j++) {

				BKS(basic_kernel_small(*(src_grid), *(tgt_grid), i, j);)

				BKL(basic_kernel_large(*(src_grid), *(tgt_grid), i, j);)

				EXK(execute_kernels(stage, i, j);)
			}
		}

		// Barriers
		MB(my_barrier(stage);)
		PTB(pthread_barrier_wait(&pthread_barriers.at(stage));)

  		// Simulate convergence test
		CVG(convergence_test(first, last, stage, my_id);)

		// Flip grid pointers
		std::vector<std::vector<double>>* temp = src_grid;
		src_grid = tgt_grid;
		tgt_grid = temp;
	}
}



// Initialize the grids (grid1 and grid2), set boundaries to 1.0 and interior points to 0.0
void initialize_grids() {

	grid1.resize(grid_size + (2 * border_size));
	grid2.resize(grid_size + (2 * border_size));
 
 	// Set all points to 0.0
	for (uint32_t i = 0; i < grid_size + (2 * border_size); i++) {
		grid1[i].resize(grid_size + (2 * border_size));
		grid2[i].resize(grid_size + (2 * border_size));

		std::fill(grid1[i].begin(), grid1[i].end(), 0.0);
		std::fill(grid2[i].begin(), grid2[i].end(), 0.0);
	}

	// Set edges to 1.0
	for (uint32_t i = 0; i < grid_size + (2 * border_size); i++) {

		if (i < border_size || i > grid_size + border_size - 1) {
			std::fill(grid1[i].begin(), grid1[i].end(), 1.0);
			std::fill(grid2[i].begin(), grid2[i].end(), 1.0);

		} else {

			for (uint32_t j = 0; j < border_size; j++) {
				grid1[i][j] = 1.0;
				grid1[i][grid_size + (2 * border_size) - j - 1] = 1.0;
				grid2[i][j] = 1.0;
				grid2[i][grid_size + (2 * border_size) - j - 1] = 1.0;
			}
		}
	} 		
}



// My implementation of a counter barrier
inline void my_barrier(uint32_t stage) {

	pthread_mutex_lock(&my_barrier_mutex);

  	num_arrived++;

  	if (num_arrived == num_workers.at(stage)) {

    	num_arrived = 0;
    	pthread_cond_broadcast(&go);

  	} else {

    	pthread_cond_wait(&go, &my_barrier_mutex);
	}

  	pthread_mutex_unlock(&my_barrier_mutex);
}



// Performs the jacobi kernel. Computes the average of the given point's four neighbors in the source grid and stores it in the target grid
inline void basic_kernel_small(std::vector<std::vector<double>>& src_grid, std::vector<std::vector<double>>& tgt_grid, uint32_t i, uint32_t j) {

	tgt_grid[i][j] = (src_grid[i - 1][j] + src_grid[i + 1][j] + src_grid[i][j - 1] + src_grid[i][j + 1]) * 0.25;
}



// Performs a larger version of the jacobi kernel. Computes average of the given point's 5x5 neighborhood in the source grid and stores it in the target grid
inline void basic_kernel_large(std::vector<std::vector<double>>& src_grid, std::vector<std::vector<double>>& tgt_grid, uint32_t i, uint32_t j) {

	tgt_grid[i][j]  = (src_grid[i - 2][j + 2] + src_grid[i - 1][j + 2] + src_grid[i][j + 2] + src_grid[i + 1][j + 2] + src_grid[i + 2][j + 2]);
	tgt_grid[i][j] += (src_grid[i - 2][j + 1] + src_grid[i - 1][j + 1] + src_grid[i][j + 1] + src_grid[i + 1][j + 1] + src_grid[i + 2][j + 1]);
	tgt_grid[i][j] += (src_grid[i - 2][j]     + src_grid[i - 1][j]                          + src_grid[i + 1][j]     + src_grid[i + 2][j]);
	tgt_grid[i][j] += (src_grid[i - 2][j - 1] + src_grid[i - 1][j - 1] + src_grid[i][j - 1] + src_grid[i + 1][j - 1] + src_grid[i + 2][j - 1]);
	tgt_grid[i][j] += (src_grid[i - 2][j - 2] + src_grid[i - 1][j - 2] + src_grid[i][j - 2] + src_grid[i + 1][j - 2] + src_grid[i + 2][j - 2]);

	tgt_grid[i][j] = tgt_grid[i][j] / 24;
}



// Executes the relevant kernels set by the experiment parameters
inline void execute_kernels(uint32_t stage, uint32_t i, uint32_t j) {

	for (uint32_t k = 0; k < kernels.at(stage).size(); k++) {

		uint64_t local_repeats = kernel_repeats.at(stage).at(k);
		VRY(local_repeats = local_repeats * (((stage + 1 * i + 1 * j + 1) % 3) + 1);)

		switch(kernels.at(stage).at(k)) {
			case none:
				break;

			case cpu:
				hogcpu(local_repeats);
				break;

			case io:
				hogio(local_repeats);
				break;

			case vm:
				hogvm(local_repeats);
				break;

			case hdd:
				hoghdd(local_repeats);
				break;

			default:
				print("Invalid kernel found: ", kernels.at(stage).at(k));
				exit(1);
		}
	}
}



// Simulate a convergence test. Computes the maximum difference in given strip and sets the global_max_difference variable
inline void convergence_test(uint32_t first, uint32_t last, uint32_t stage, uint32_t id) {

  	double max_diff = 0.0;

	for (uint32_t i = first; i < last; i++) {
		for (uint32_t j = border_size; j < grid_size + border_size; j++) {
			uint32_t temp = grid1[i][j] - grid2[i][j];

			if (temp < 0) {
				temp = -temp;
			}

			if (max_diff < temp) {
				max_diff = temp;
			}
		}
	}

	global_max_difference[stage][id] = max_diff;
}
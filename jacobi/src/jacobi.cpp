#include <thread>
#include <vector>
#include <map>
#include <sys/times.h>
#include <algorithm>

#include <utils.hpp>
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

#ifdef RANDOMIZE_LOAD
#define RND( x ) x
#pragma message "RANDOMIZE_LOAD ACTIVE"
#else
#define RND( x )
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



// Initialize the grids (grid1 and grid2), set boundaries to 1.0 and interior points to 0.0
void initialize_grids();

// Each Worker computes values in one strip of the grids. The main worker loop does two computations to avoid copying from one grid to the other.
void worker(uint32_t my_id, uint32_t stage);

void execute_kernels(uint32_t stage, uint32_t id, uint32_t i, uint32_t j);

// Counter barrier
inline void my_barrier(uint32_t stage);

enum kernels_enum {e_none = 0, e_addpd = 1, e_mulpd = 2, e_sqrt = 3, e_compute = 4, e_sinus = 5, e_idle = 6, e_memory_read = 7, e_memory_copy = 8, e_memory_write = 9, e_shared_mem_read_small = 10, e_shared_mem_read_large = 11, e_cpu = 12, e_io = 13, e_vm = 14, e_hdd = 15};

inline void basic_kernel_small(std::vector<std::vector<double>>& src_grid, std::vector<std::vector<double>>& tgt_grid, uint32_t i, uint32_t j);

inline void basic_kernel_large(std::vector<std::vector<double>>& src_grid, std::vector<std::vector<double>>& tgt_grid, uint32_t i, uint32_t j);

inline void execute_kernels(uint32_t stage);

// Simulate a convergence test. Compute the maximum difference in my strip and set global variable.
inline void convergence_test(uint32_t first, uint32_t last, uint32_t stage, uint32_t id);



// Set of pthread barriers (one for each stage)
std::vector<pthread_barrier_t> pthread_barriers;

// Mutex semaphore for my barrier
pthread_mutex_t my_barrier_mutex;

// Condition variable for leaving my barrier
pthread_cond_t go;

// Count of the number who have arrived at my barrier   
uint32_t num_arrived = 0;



// Used for timing
std::chrono::high_resolution_clock::time_point start;
std::chrono::high_resolution_clock::time_point end;

// Experiment parameters
uint32_t num_runs, grid_size, num_stages, use_set_num_repeats;

// Stage parameters
std::vector<uint32_t> num_workers, num_iterations, set_pin_bool;
std::vector<std::vector<uint32_t>> kernels, kernel_durations, kernel_repeats, row_allocations;
std::vector<std::vector<std::vector<uint32_t>>> pinnings;

// Experiment data
std::vector<std::vector<double>> grid1, grid2;

// Used for convergence test
std::vector<std::vector<double>> max_difference_global;

// uint64_t repeats;
uint32_t max_num_workers;

static long long const do_vm_bytes = 1024 * 1024;
static long long const do_vm_stride = 4096;
static long long const do_vm_hang = -1;
static int const do_vm_keep = 1;

static long long const do_hdd_bytes = 1024;

// static uint32_t const border_size = 2;



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

		temp.at(0) = 2;

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
	max_difference_global.resize(num_stages);

	// Set per stage global max difference vector sizes
	for (uint32_t i = 0; i < num_stages; i++) {
		max_difference_global.at(i).resize(num_workers.at(i));
	}



	// Calculate the max number of workers we will need
	max_num_workers = *max_element(std::begin(num_workers), std::end(num_workers));

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



	// Initialize runtimes sum for computing average
	uint32_t runtimes_sum = 0;

	for (uint32_t r = 1; r < num_runs + 1; r++) {

		initialize_grids();

		// Record start time
		start = std::chrono::high_resolution_clock::now();

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

		runtimes_sum += millis.count();
	}

	// Print average runtime
	print("\nAverage elapsed time over ", num_runs, " runs: ", runtimes_sum / num_runs, "ms\n");
}



// Each Worker computes values in one strip of the grids. The main worker loop does two computations to avoid copying from one grid to the other.
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
			for (uint32_t j = 2; j < grid_size + 2; j++) {

				BKS(basic_kernel_small(*(src_grid), *(tgt_grid), i, j);)

				BKL(basic_kernel_large(*(src_grid), *(tgt_grid), i, j);)

				EXK(execute_kernels(stage);)
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

	grid1.resize(grid_size + 4);
	grid2.resize(grid_size + 4);
 
 	// Set all points to 0.0
	for (uint32_t i = 0; i < grid_size + 4; i++) {
		grid1[i].resize(grid_size + 4);
		grid2[i].resize(grid_size + 4);

		for (uint32_t j = 0; j < grid_size + 4; j++) {
		    grid1[i][j] = 0.0;
		    grid2[i][j] = 0.0;
		}
	}

	// Set edges to 1.0
	for (uint32_t i = 0; i < grid_size + 2; i++) {
		grid1[i][0] = 1.0;
	    grid1[i][grid_size + 1] = 1.0;
	    grid2[i][0] = 1.0;
	    grid2[i][grid_size + 1] = 1.0;
	}

  	for (uint32_t j = 0; j < grid_size + 2; j++) {
	    grid1[0][j] = 1.0;
	    grid2[0][j] = 1.0;
	    grid1[grid_size + 1][j] = 1.0;
	    grid2[grid_size + 1][j] = 1.0;
  	}
}

// Counter barrier
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

inline void basic_kernel_small(std::vector<std::vector<double>>& src_grid, std::vector<std::vector<double>>& tgt_grid, uint32_t i, uint32_t j) {

	tgt_grid[i][j] = (src_grid[i - 1][j] + src_grid[i + 1][j] + src_grid[i][j - 1] + src_grid[i][j + 1]) * 0.25;
}

inline void basic_kernel_large(std::vector<std::vector<double>>& src_grid, std::vector<std::vector<double>>& tgt_grid, uint32_t i, uint32_t j) {

	tgt_grid[i][j]  = (src_grid[i - 2][j + 2] + src_grid[i - 1][j + 2] + src_grid[i][j + 2] + src_grid[i + 1][j + 2] + src_grid[i + 2][j + 2]);
	tgt_grid[i][j] += (src_grid[i - 2][j + 1] + src_grid[i - 1][j + 1] + src_grid[i][j + 1] + src_grid[i + 1][j + 1] + src_grid[i + 2][j + 1]);
	tgt_grid[i][j] += (src_grid[i - 2][j]     + src_grid[i - 1][j]                          + src_grid[i + 1][j]     + src_grid[i + 2][j]);
	tgt_grid[i][j] += (src_grid[i - 2][j - 1] + src_grid[i - 1][j - 1] + src_grid[i][j - 1] + src_grid[i + 1][j - 1] + src_grid[i + 2][j - 1]);
	tgt_grid[i][j] += (src_grid[i - 2][j - 2] + src_grid[i - 1][j - 2] + src_grid[i][j - 2] + src_grid[i + 1][j - 2] + src_grid[i + 2][j - 2]);

	tgt_grid[i][j] = tgt_grid[i][j] / 24;
}

inline void execute_kernels(uint32_t stage) {

	for (uint32_t k = 0; k < kernels.at(stage).size(); k++) {

		uint64_t local_repeats = kernel_repeats.at(stage).at(k);
		RND(local_repeats = local_repeats * ((rand() % 3) + 1);)

		switch(kernels.at(stage).at(k)) {
			case e_none:
				break;

			case e_cpu:
				hogcpu(local_repeats);
				break;

			case e_io:
				hogio(local_repeats);
				break;

			case e_vm:
				hogvm(local_repeats, do_vm_bytes, do_vm_stride, do_vm_hang, do_vm_keep);
				break;

			case e_hdd:
				hoghdd(local_repeats, do_hdd_bytes);
				break;

			default:
				print("Invalid kernel found: ", kernels.at(stage).at(k));
				exit(1);
		}
	}
}

// Simulate a convergence test. Compute the maximum difference in my strip and set global variable.
inline void convergence_test(uint32_t first, uint32_t last, uint32_t stage, uint32_t id) {

  	double max_diff = 0.0;

	for (uint32_t i = first; i < last; i++) {
		for (uint32_t j = 2; j < grid_size + 2; j++) {
			uint32_t temp = grid1[i][j] - grid2[i][j];

			if (temp < 0) {
				temp = -temp;
			}

			if (max_diff < temp) {
				max_diff = temp;
			}
		}
	}

	max_difference_global[stage][id] = max_diff;
}
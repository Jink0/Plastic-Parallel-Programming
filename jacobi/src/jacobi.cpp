#include <thread>
#include <vector>
#include <map>
#include <sys/times.h>
#include <algorithm>

#include <utils.hpp>
#include <config_file_utils.hpp>
#include <kernels.hpp>



#ifdef MYBARRIER
#define MB( x ) x
#else
#define MB( x )
#endif

#ifdef PTHREADBARRIER
#define PTB( x ) x
#else
#define PTB( x )
#endif

#ifdef MULPD
#define MLPD( x ) x
#else
#define MLPD( x )
#endif

#ifdef BASIC_KERNEL_SMALL
#define BSCS( x ) x
#else
#define BSCS( x )
#endif

#ifdef BASIC_KERNEL_LARGE
#define BSCL( x ) x
#else
#define BSCL( x )
#endif

#ifdef EXECUTE_KERNELS
#define EXCK( x ) x
#else
#define EXCK( x )
#endif

#ifdef CONVERGE_TEST
#define CNVG( x ) x
#else
#define CNVG( x )
#endif



// Initialize the grids (grid1 and grid2), set boundaries to 1.0 and interior points to 0.0
void initialize_grids();

// Each Worker computes values in one strip of the grids. The main worker loop does two computations to avoid copying from one grid to the other.
void worker(uint32_t my_id, uint32_t stage);

void execute_kernels(uint32_t stage, uint32_t id, uint32_t i, uint32_t j);

// My counter barrier
void my_barrier(uint32_t stage);



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
std::vector<uint32_t> num_workers, num_iterations, set_pin_bool, strip_size;
std::vector<std::vector<uint32_t>> kernels, kernel_durations, kernel_repeats;
std::vector<std::vector<std::vector<uint32_t>>> pinnings;

// Experiment data
std::vector<std::vector<double>> grid1, grid2;

// Used for convergence test
std::vector<std::vector<double>> max_difference_global;

uint64_t rep_pass;



int main(int argc, char *argv[]) {

	// Parse config
	std::map<std::string, std::string> config = parse_config(std::string(argv[1]));

	// Read config
	read_config(config);

	MLPD(
		rep_pass = atoi(argv[2]);
		print("NUM MULPD REPEATS: ", rep_pass, "\n");
		)

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

	//  Calculate strip sizes
	for (uint32_t i = 0; i < num_stages; i++) {
		strip_size.push_back(grid_size / num_workers.at(i));
	}

	// Calculate the max number of workers we will need
	uint32_t max_num_workers = *max_element(std::begin(num_workers), std::end(num_workers));

  	// Create thread handles
	std::vector<std::thread> threads(max_num_workers);

	// Set global max difference vector size
	max_difference_global.resize(num_stages);

	for (uint32_t i = 0; i < num_stages; i++) {
		max_difference_global.at(i).resize(num_workers.at(i));
	}

	// Initialize pthread barriers
	for (uint32_t i = 0; i < num_stages; i++) {
		pthread_barrier_t b;
		pthread_barrier_init(&b, NULL, num_workers.at(i));
		pthread_barriers.push_back(b);
	}

	// Initialize mutex and condition variable for my barrier
	pthread_mutex_init(&my_barrier_mutex, NULL);
	pthread_cond_init(&go, NULL);

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

		// Record end time
		end = std::chrono::high_resolution_clock::now();

		CNVG(
			uint32_t max_diff = 0.0;

			// Find maximum difference
			for (uint32_t i = 0; i < num_workers.back(); i++) {
				if (max_diff < max_difference_global.back().at(i)) {
			  		max_diff = max_difference_global.back().at(i);
				}
			}
		)

		std::chrono::duration<double> diff = end - start;
		std::chrono::milliseconds millis = std::chrono::duration_cast<std::chrono::milliseconds>(diff);

		// Print runtime
		print("\nRun ", r, "\n",
			  "Elapsed time: ", millis.count(), "ms\n");

		// Record runtime
		fputs((std::to_string(millis.count()) + "\n").c_str(), output_stream);
	}
}



// Each Worker computes values in one strip of the grids. The main worker loop does two computations to avoid copying from one grid to the other.
void worker(uint32_t my_id, uint32_t stage) {

	// Set our affinity
	force_affinity_set(pinnings.at(stage).at(my_id));

	// Determine first and last rows of my strip of the grids
	uint32_t first = my_id * strip_size.at(stage) + 2;
	uint32_t last = first + strip_size.at(stage);

	MLPD(
		std::vector<double> vec_A(16);

		for (std::size_t i = 0; i < 16; ++i) {
		    vec_A[i] = 1. + std::numeric_limits<double>::epsilon();
		}

    	uint64_t repeats = rep_pass;
    	)

	for (uint32_t iter = 0; iter < num_iterations.at(stage); iter++) {

		// Update my points
		for (uint32_t i = first; i < last; i++) {
			for (uint32_t j = 2; j < grid_size + 2; j++) {
				BSCS(grid2[i][j] = (grid1[i - 1][j] + grid1[i + 1][j] + grid1[i][j - 1] + grid1[i][j + 1]) * 0.25;)

				BSCL(
					grid2[i][j] =  0;
					grid2[i][j] += (grid1[i - 2][j + 2] + grid1[i - 1][j + 2] + grid1[i][j + 2] + grid1[i + 1][j + 2] + grid1[i + 2][j + 2]);
					grid2[i][j] += (grid1[i - 2][j + 1] + grid1[i - 1][j + 1] + grid1[i][j + 1] + grid1[i + 1][j + 1] + grid1[i + 2][j + 1]);
					grid2[i][j] += (grid1[i - 2][j] + grid1[i - 1][j] + grid1[i + 1][j] + grid1[i + 2][j]);
					grid2[i][j] += (grid1[i - 2][j - 1] + grid1[i - 1][j - 1] + grid1[i][j - 1] + grid1[i + 1][j - 1] + grid1[i + 2][j - 1]);
					grid2[i][j] += (grid1[i - 2][j - 2] + grid1[i - 1][j - 2] + grid1[i][j - 2] + grid1[i + 1][j - 2] + grid1[i + 2][j - 2]);
					grid2[i][j] =  grid2[i][j] / 24;
					)

				MLPD(mulpd_kernel(vec_A.data(), repeats);)

				// Execute kernel functions
				EXCK(execute_kernels(stage, my_id, i, j);)
			}
		}

		// Barriers
		MB(my_barrier(stage);)
		PTB(pthread_barrier_wait(&pthread_barriers.at(stage));)

		CNVG(
			// Simulate a convergence test. Compute the maximum difference in my strip and set global variable
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

			max_difference_global[stage][my_id] = max_diff;
			)

		// Update my points again
		for (uint32_t i = first; i < last; i++) {
			for (uint32_t j = 2; j < grid_size + 2; j++) {
				BSCS(grid1[i][j] = (grid2[i - 1][j] + grid2[i + 1][j] + grid2[i][j - 1] + grid2[i][j + 1]) * 0.25;)

				BSCL(
					grid1[i][j] =  0;
					grid1[i][j] += (grid2[i - 2][j + 2] + grid2[i - 1][j + 2] + grid2[i][j + 2] + grid2[i + 1][j + 2] + grid2[i + 2][j + 2]);
					grid1[i][j] += (grid2[i - 2][j + 1] + grid2[i - 1][j + 1] + grid2[i][j + 1] + grid2[i + 1][j + 1] + grid2[i + 2][j + 1]);
					grid1[i][j] += (grid2[i - 2][j] + grid2[i - 1][j] + grid2[i + 1][j] + grid2[i + 2][j]);
					grid1[i][j] += (grid2[i - 2][j - 1] + grid2[i - 1][j - 1] + grid2[i][j - 1] + grid2[i + 1][j - 1] + grid2[i + 2][j - 1]);
					grid1[i][j] += (grid2[i - 2][j - 2] + grid2[i - 1][j - 2] + grid2[i][j - 2] + grid2[i + 1][j - 2] + grid2[i + 2][j - 2]);
					grid1[i][j] =  grid1[i][j] / 24;
					)

				MLPD(mulpd_kernel(vec_A.data(), repeats);)

				// Execute kernel functions
				EXCK(execute_kernels(stage, my_id, i, j);)
			}
		}

		// Barriers
		MB(my_barrier(stage);)
		PTB(pthread_barrier_wait(&pthread_barriers.at(stage));)

		CNVG(
			// Simulate a convergence test. Compute the maximum difference in my strip and set global variable
		  	max_diff = 0.0;

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

			max_difference_global[stage][my_id] = max_diff;
			)
	}
}



enum kernels_enum {e_none = 0, e_addpd = 1, e_mulpd = 2, e_sqrt = 3, e_compute = 4, e_sinus = 5, e_idle = 6, e_memory_read = 7, e_memory_copy = 8, e_memory_write = 9, e_shared_mem_read_small = 10, e_shared_mem_read_large = 11};

void execute_kernels(uint32_t stage, uint32_t id, uint32_t i, uint32_t j) {
	// Execute kernel functions
	for (uint32_t k = 0; k < kernels.at(stage).size(); k++) {

		if (use_set_num_repeats == 0) {

			std::chrono::microseconds duration(kernel_durations.at(stage).at(k));

			switch(kernels.at(stage).at(k)) {
				case e_none:
					break;

				case e_addpd:
					addpd(duration, id, num_workers.at(stage));
					break;

				case e_mulpd:
					mulpd(duration, id, num_workers.at(stage));
					break;

				case e_sqrt:
					sqrt(duration, id, num_workers.at(stage));
					break;

				case e_compute:
					compute(duration, id, num_workers.at(stage));
					break;

				case e_sinus:
					sinus(duration, id, num_workers.at(stage));
					break;

				case e_idle:
					idle(duration);
					break;

				case e_memory_read:
					memory_read(duration, id, num_workers.at(stage));
					break;

				case e_memory_copy:
					memory_copy(duration, id, num_workers.at(stage));
					break;

				case e_memory_write:
					memory_write(duration, id, num_workers.at(stage));
					break;

				case e_shared_mem_read_small:
					shared_memory_read_small(duration, i, j);
					break;

				case e_shared_mem_read_large:
					shared_memory_read_large(duration, i, j);
					break;
			}

		} else {
			switch(kernels.at(stage).at(k)) {
				case e_none:
					break;

				case e_addpd:
					addpd(kernel_repeats.at(stage).at(k), id, num_workers.at(stage));
					break;

				case e_mulpd:
					mulpd(kernel_repeats.at(stage).at(k), id, num_workers.at(stage));
					break;

				case e_sqrt:
					sqrt(kernel_repeats.at(stage).at(k), id, num_workers.at(stage));
					break;

				case e_compute:
					compute(kernel_repeats.at(stage).at(k), id, num_workers.at(stage));
					break;

				case e_sinus:
					sinus(kernel_repeats.at(stage).at(k), id, num_workers.at(stage));
					break;

				case e_idle:
					print("Cannot use idle with a set number of repeats!");
					exit(1);
					break;

				case e_memory_read:
					memory_read(kernel_repeats.at(stage).at(k), id, num_workers.at(stage));
					break;

				case e_memory_copy:
					memory_copy(kernel_repeats.at(stage).at(k), id, num_workers.at(stage));
					break;

				case e_memory_write:
					memory_write(kernel_repeats.at(stage).at(k), id, num_workers.at(stage));
					break;

				case e_shared_mem_read_small:
					shared_memory_read_small(kernel_repeats.at(stage).at(k), i, j);
					break;

				case e_shared_mem_read_large:
					shared_memory_read_large(kernel_repeats.at(stage).at(k), i, j);
					break;
			}
		}
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
void my_barrier(uint32_t stage) {

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
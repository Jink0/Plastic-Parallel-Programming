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

#ifdef CONVERGE_TEST
#define CNVG( x ) x
#else
#define CNVG( x )
#endif



// Initialize the grids (grid1 and grid2), set boundaries to 1.0 and interior points to 0.0
void initialize_grids();

// Each Worker computes values in one strip of the grids. The main worker loop does two computations to avoid copying from one grid to the other.
void worker(uint32_t my_id, uint32_t stage);

// My counter barrier
MB(void my_barrier(uint32_t stage);)




PTB(
	// Set of pthread barriers (one for each stage)
	std::vector<pthread_barrier_t> pthread_barriers;
)

MB(
	// Mutex semaphore for my barrier
	pthread_mutex_t my_barrier_mutex;

	// Condition variable for leaving my barrier
	pthread_cond_t go;

	// Count of the number who have arrived at my barrier   
	uint32_t num_arrived = 0;
)



// Used for timing
struct tms buffer;        
clock_t start, end;

uint32_t num_runs, grid_size, num_stages;

std::vector<uint32_t> num_workers, num_iterations, set_pin_bool, strip_size;

std::vector<std::vector<double>> grid1, grid2;

std::vector<std::vector<std::vector<uint32_t>>> pinnings;



CNVG(
	// Used for convergence test
	std::vector<std::vector<double>> max_difference_global;
)



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

	

	//  Calculate strip sizes
	for (uint32_t i = 0; i < num_stages; i++) {
		strip_size.push_back(grid_size / num_workers.at(i));
	}

	// Calculate the max number of workers we will need
	uint32_t max_num_workers = *max_element(std::begin(num_workers), std::end(num_workers));

  	// Create thread handles
	std::vector<std::thread> threads(max_num_workers);

	CNVG(
		// Set global max difference vector size
		max_difference_global.resize(num_stages);

		for (uint32_t i = 0; i < num_stages; i++) {
			max_difference_global.at(i).resize(num_workers.at(i));
		}
	)

	PTB(
		// Initialize pthread barriers
		for (uint32_t i = 0; i < num_stages; i++) {
			pthread_barrier_t b;
			pthread_barrier_init(&b, NULL, num_workers.at(i));
			pthread_barriers.push_back(b);
		}
	)

	
	MB(
		// Initialize mutex and condition variable for my barrier
		pthread_mutex_init(&my_barrier_mutex, NULL);
		pthread_cond_init(&go, NULL);
	)

	for (uint32_t r = 1; r < num_runs + 1; r++) {

		initialize_grids();

		// Record start time
		start = times(&buffer);

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
		end = times(&buffer);

		CNVG(
			uint32_t max_diff = 0.0;

			// Find maximum difference
			for (uint32_t i = 0; i < num_workers.back(); i++) {
				if (max_diff < max_difference_global.back().at(i)) {
			  		max_diff = max_difference_global.back().at(i);
				}
			}
		)

		// Print runtime
		print("\nRun ", r, "\n",
			  "Elapsed time: ", end - start, "\n");

		// Record runtime
		fputs((std::to_string(end - start) + "\n").c_str(), output_stream);
	}
}



// Each Worker computes values in one strip of the grids. The main worker loop does two computations to avoid copying from one grid to the other.
void worker(uint32_t my_id, uint32_t stage) {

	// Set our affinity
	force_affinity_set(pinnings.at(stage).at(my_id));

	// Determine first and last rows of my strip of the grids
	uint32_t first = my_id * strip_size.at(stage) + 1;
	uint32_t last = first + strip_size.at(stage) - 1;

	for (uint32_t iter = 1; iter <= num_iterations.at(stage); iter++) {

		// Update my points
		for (uint32_t i = first; i <= last; i++) {
			for (uint32_t j = 1; j <= grid_size; j++) {
				grid2[i][j] = (grid1[i - 1][j] + grid1[i + 1][j] + grid1[i][j - 1] + grid1[i][j + 1]) * 0.25;
			}
		}

		// Barriers
		MB(my_barrier(stage);)
		PTB(pthread_barrier_wait(&pthread_barriers.at(stage));)

		std::chrono::milliseconds millis(4);

		std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

		addpd(millis, my_id, num_workers.at(stage));

		std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

		

		std::chrono::duration<double> diff = end - start;

		std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(diff);

		// print(ms.count(), "\n");

		// addpd(1, my_id, num_workers.at(stage));
		// mulpd(10);
		// sqrt(1);
		// compute(1);
		// sinus(1);
		// idle(5000);
		// memory_read(1);
		// memory_copy(1);
		// memory_write(1);

		// Update my points again
		for (uint32_t i = first; i <= last; i++) {
			for (uint32_t j = 1; j <= grid_size; j++) {
				grid1[i][j] = (grid2[i - 1][j] + grid2[i + 1][j] + grid2[i][j - 1] + grid2[i][j + 1]) * 0.25;
			}
		}

		// Barriers
		MB(my_barrier(stage);)
		PTB(pthread_barrier_wait(&pthread_barriers.at(stage));)

		CNVG(
			// Simulate a convergence test. Compute the maximum difference in my strip and set global variable
		  	double max_diff = 0.0;

			for (uint32_t i = first; i <= last; i++) {
				for (uint32_t j = 1; j <= grid_size; j++) {
					uint32_t temp = grid1[i][j]-grid2[i][j];

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



// Initialize the grids (grid1 and grid2), set boundaries to 1.0 and interior points to 0.0
void initialize_grids() {

	grid1.resize(grid_size + 2);
	grid2.resize(grid_size + 2);
 
 	// Set all points to 0.0
	for (uint32_t i = 0; i <= grid_size + 1; i++) {
		grid1[i].resize(grid_size + 2);
		grid2[i].resize(grid_size + 2);

		for (uint32_t j = 0; j <= grid_size + 1; j++) {
		    grid1[i][j] = 0.0;
		    grid2[i][j] = 0.0;
		}
	}

	// Set edges to 1.0
	for (uint32_t i = 0; i <= grid_size + 1; i++) {
		grid1[i][0] = 1.0;
	    grid1[i][grid_size + 1] = 1.0;
	    grid2[i][0] = 1.0;
	    grid2[i][grid_size + 1] = 1.0;
	}

  	for (uint32_t j = 0; j <= grid_size + 1; j++) {
	    grid1[0][j] = 1.0;
	    grid2[0][j] = 1.0;
	    grid1[grid_size + 1][j] = 1.0;
	    grid2[grid_size + 1][j] = 1.0;
  	}
}



// Counter barrier
MB(void my_barrier(uint32_t stage) {

	pthread_mutex_lock(&my_barrier_mutex);

  	num_arrived++;

  	if (num_arrived == num_workers.at(stage)) {

    	num_arrived = 0;
    	pthread_cond_broadcast(&go);

  	} else {

    	pthread_cond_wait(&go, &my_barrier_mutex);
	}

  	pthread_mutex_unlock(&my_barrier_mutex);
})
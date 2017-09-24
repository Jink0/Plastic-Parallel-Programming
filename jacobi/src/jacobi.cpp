#define _REENTRANT

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/times.h>
#include <limits.h>

#include <iostream>
#include <vector>
#include <thread>

#include <sstream>

#include <utils.hpp>

void Worker(long long my_id);
void InitializeGrids();
void Barrier();

struct tms buffer;        // Used for timing
clock_t start, end;

pthread_mutex_t barrier;  // Mutex semaphore for the barrier
pthread_cond_t go;        // Condition variable for leaving
uint32_t num_arrived = 0;       // Count of the number who have arrived

uint32_t grid_size, num_workers, num_iterations, set_pin_bool, num_cores, strip_size;
std::vector<double> max_difference_global;
std::vector<std::vector<double>> grid1, grid2;



// main() -- read command line, initialize grids, and create threads when the threads are done, print the results
int main(int argc, char *argv[]) {

	// Read command line arguments
	grid_size      = atoi(argv[1]);
	num_workers    = atoi(argv[2]);
	num_iterations = atoi(argv[3]);
	set_pin_bool   = atoi(argv[4]);

	if (set_pin_bool != 0) {
		num_cores = atoi(argv[5]);
	}

	//  Calculate strip size
	strip_size = grid_size / num_workers;

	InitializeGrids();

  	// Thread handles
	std::vector<std::thread> threads(num_workers);

	max_difference_global.resize(num_workers);

	double max_diff = 0.0;

	// Initialize mutex and condition variable
	pthread_mutex_init(&barrier, NULL);
	pthread_cond_init(&go, NULL);

	// Print arguments
	print("\n");

	// Record start time
	start = times(&buffer);

	// Create workers
	for (uint32_t i = 0; i < num_workers; i++) {
		threads.at(i) = std::thread(Worker, i);
	}

	// Join with workers
	for (uint32_t i = 0; i < num_workers; i++) {
		threads.at(i).join();
	}

	// Record end time
	end = times(&buffer);

	for (uint32_t i = 0; i < num_workers; i++) {
		if (max_diff < max_difference_global[i]) {
	  		max_diff = max_difference_global[i];
		}
	}

	// Print results
	print("\nNumber of iterations: ", num_iterations, "\n",
		    "Maximum difference:   ", max_diff, "\n",
	        "Elapsed time:         ", end - start, "\n",
	        "\n");
}



// Each Worker computes values in one strip of the grids. The main worker loop does two computations to avoid copying from one grid to the other.
void Worker(long long my_id) {

	std::vector<uint32_t> core_ids;

	if (set_pin_bool != 0) {
		for (uint32_t i = 0; i < num_cores; i++) {
			core_ids.push_back(i);
		}

	} else {
		core_ids.push_back(my_id);
	}

	force_affinity_set(core_ids);

	// Print starting message
	print("Worker ", my_id, " has started\n");

	// Determine first and last rows of my strip of the grids
	uint32_t first = my_id * strip_size + 1;
	uint32_t last = first + strip_size - 1;

	for (uint32_t iters = 1; iters <= num_iterations; iters++) {

		// Update my points
		for (uint32_t i = first; i <= last; i++) {
			for (uint32_t j = 1; j <= grid_size; j++) {
				grid2[i][j] = (grid1[i - 1][j] + grid1[i + 1][j] + grid1[i][j - 1] + grid1[i][j + 1]) * 0.25;
			}
		}

		Barrier();

		// Update my points again
		for (uint32_t i = first; i <= last; i++) {
			for (uint32_t j = 1; j <= grid_size; j++) {
				grid1[i][j] = (grid2[i - 1][j] + grid2[i + 1][j] + grid2[i][j - 1] + grid2[i][j + 1]) * 0.25;
			}
		}

		Barrier();
	}

  // Compute the maximum difference in my strip and set global variable
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

	max_difference_global[my_id] = max_diff;
}



// Initialize the grids (grid1 and grid2), set boundaries to 1.0 and interior points to 0.0
void InitializeGrids() {

	grid1.resize(grid_size + 2);
	grid2.resize(grid_size + 2);
 
	for (uint32_t i = 0; i <= grid_size + 1; i++) {
		grid1[i].resize(grid_size + 2);
		grid2[i].resize(grid_size + 2);

		for (uint32_t j = 0; j <= grid_size + 1; j++) {
		    grid1[i][j] = 0.0;
		    grid2[i][j] = 0.0;
		}
	}

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



// Reusable counter barrier. Not sense reversing?
void Barrier() {

	pthread_mutex_lock(&barrier);

  	num_arrived++;

  	if (num_arrived == num_workers) {

    	num_arrived = 0;
    	pthread_cond_broadcast(&go);

  	} else {

    	pthread_cond_wait(&go, &barrier);
	}

  	pthread_mutex_unlock(&barrier);
}
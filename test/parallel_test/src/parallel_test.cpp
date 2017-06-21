#include <parallel_test.hpp>

// Workloads for workload generation
#include "workloads.hpp"

#include "metrics.hpp"
#include "utils.hpp"

#include <stdint.h>
#include <omp.h>
#include <iostream>

#include <string>

#define CHUNKSIZE 500
#define N_THREADS 4
#define REPEATS   5

int main(int argc, char *argv[])
{
	for (uint32_t r = 0; r < REPEATS; r++) {
		// Initialise metrics.
	  	metrics_init(N_THREADS, string(argv[1]) + "_Repeat" + std::to_string(r) + ".csv");

		uint32_t as = 200000;

		// Experiment input vectors.
	    deque<int> input1(as);
	    deque<int> input2(as);

	    // Generate data for vectors.
	    for (uint32_t i = 0; i < as; i++) 
	    {
	        // input1[i] = (double) 1. / i;
	        // input2[i] = (double) 1. / i;
	        // input1[i] = (int) i + 1;
	        // input2[i] = (int) i + 1;
	        input1[i] = 10000;
	        input2[i] = 87736;
	    }

	    // Output deque.
	    deque<int> output(as);

	    

	    uint32_t nthreads, tid, i;

		uint32_t chunk = CHUNKSIZE;

		omp_set_dynamic(0);     // Explicitly disable dynamic teams
		omp_set_num_threads(N_THREADS); // Use 4 threads for all consecutive parallel regions

		#pragma omp parallel shared(input1, input2, output, nthreads, chunk) private(i,tid) 
		{
		  tid = omp_get_thread_num();
		  metrics_thread_start(tid);

		  if (tid == 0) {
		    nthreads = omp_get_num_threads();
		    print("Number of threads = ", nthreads, "\n");
		  }

		  print("Thread ", tid, " starting...\n");

		  #pragma omp for schedule(dynamic,chunk)
		  for (i = 0; i < as; i++) {
		  	metrics_starting_work(tid);
		    output.at(i) = collatz(input1.at(i), input2);
		    metrics_finishing_work(tid);

		  }

		  metrics_thread_finished(tid);
	  	}  /* end of parallel section */

		metrics_finalise();

	  	metrics_calc();

	  	metrics_exit();

		for (i = 0; i < as; i++) 
	    {
	        //print(output.at(i));
	    }
	}
}
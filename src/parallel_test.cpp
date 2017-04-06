#include <parallel_test.hpp>

// Workloads for workload generation
#include "workloads.hpp"

#include "metrics.hpp"
#include "utils.hpp"

#include <stdint.h>
#include <omp.h>
#include <iostream>

#define REPEATS 50



int main(int argc, char *argv[])
{
	uint32_t experiment_number = 9;

	for (uint32_t j = 1; j < 5; j++)
	{
		for (uint32_t k = 0; k < REPEATS; k++)
		{
			// Initialise metrics.
		  	metrics_init(j, "Experiment" + to_string(experiment_number) + "_Repeat" + to_string(k) + ".csv");

			uint32_t as = 10000;

			// Experiment input vectors.
		    deque<int> input1(as);
		    deque<int> input2(as);

		    // Generate data for vectors.
		    for (uint32_t i = 0; i < as; i++) 
		    {
		        input1[i] = 1000000;
		        input2[i] = 87736;
		    }

		    // Output deque.
		    deque<int> output(as);

		    uint32_t nthreads, tid, i;

			uint32_t chunk = 1000;

			omp_set_dynamic(0);     // Explicitly disable dynamic teams
			omp_set_num_threads(j); // Use j threads for all consecutive parallel regions

			#pragma omp parallel shared(input1, input2, output, nthreads, chunk) private(i,tid)
			{
			  tid = omp_get_thread_num();
			  metrics_thread_start(tid);

			  if (tid == 0) {
			    nthreads = omp_get_num_threads();
			    print("Number of threads = ", nthreads, "\n");
			  }

			  print("Thread ", tid, " starting...\n");

			  #pragma omp for schedule(dynamic,1)
			  for (i = 0; i < as; i++) {
			  	metrics_starting_work(tid);
			    output.at(i) = collatz(input1.at(i), input2);
			    metrics_finishing_work(tid);

			  }

			  metrics_thread_finished(tid);
		  	}

			metrics_finalise();

		  	metrics_calc();

		  	metrics_exit();
		}
		
		experiment_number++;
	}
}
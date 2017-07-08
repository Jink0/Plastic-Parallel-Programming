#include <parallel_test.hpp>

#include <string>
#include <stdint.h>
#include <iostream>
#include <deque>

#include <omp.h>

#include "utils.hpp"
#include "config_files_utils.hpp"
#include "workloads.hpp"
#include "metrics.hpp"



#define CHUNKSIZE 500
#define N_THREADS 4
#define REPEATS   5

#define DETAILED_METRICS



int main(int argc, char *argv[]) {

	// Retrieve run parameters from given config file.
    struct run_parameters params = translate_run_parameters(read_config_file(argc, argv));

    // Print run parameters.
    print(params);

    // Move to output folder and copy our config to it.
    moveAndCopy(argv[1], "parallel_test");

    // For each experiment,
    for (uint32_t i = 0; i < params.experiments.size(); i++) {

    	// Create output filename.
    	std::string output_filename = ("Experiment" + to_string(i + 1) + ".csv");

    	metrics_start(output_filename);

    	// For each repeat,
    	for (uint32_t j = 0; j < params.number_of_repeats; j++) {

    		// Generate workload.
    		struct workload<int, int, int> work = generate_workload<int, int, int>(params.experiments.at(i));

    		// Create output deque.
		    deque<int> output(work.input1.size(), 0);

		    metrics_repeat_start(params.experiments.at(i).number_of_threads);

		    // Parallel section start
		    switch (params.experiments.at(i).threading_lib) {
		    	case Default:
		    		goto Def;

		    		break;

		    	case pThreads:
		    		print("\npThreads not implemented for parallel_test!\n\n");

		    		exit(EXIT_FAILURE);

		    		break;

		    	case TBB:
		    		print("\nTBB not implemented for parallel_test!\n\n");

		    		exit(EXIT_FAILURE);

		    		break;

		    	case OpenMP:
		    		Def:

				    uint32_t nthreads, tid, k;

				    // Explicitly disable dynamic teams.
					omp_set_dynamic(0);  

					// Use set number of threads for all consecutive parallel regions. 
					omp_set_num_threads(work.params.number_of_threads);

					#pragma omp parallel shared(work, output) private(k, tid) 
					{
						// Get tid.
					  	tid = omp_get_thread_num();

					  	if (tid == 0) {
					    	nthreads = omp_get_num_threads();
					    	print("Number of threads = ", nthreads, "\n");
					  	}

					  	print("Thread ", tid, " starting...\n");

					  	metrics_thread_start(tid);

					  	// Set our schedule.
					  	switch (params.experiments.at(i).initial_schedule) {
					  		case Static:
					  			omp_set_schedule(omp_sched_static, 0);

					  			break;

					  		case Dynamic_chunks:
					  			omp_set_schedule(omp_sched_dynamic, work.params.initial_chunk_size);

					  			break;

					  		case Tapered:
					  			omp_set_schedule(omp_sched_guided, work.params.initial_chunk_size);

					  			break;

					  		case Auto:
					  			omp_set_schedule(omp_sched_auto, work.params.initial_chunk_size);

					  			break;
					  	}

					  	#pragma omp for schedule(runtime)
					  	for (k = 0; k < work.input1.size(); k++) {
					  		metrics_starting_work(tid);

					  		// Do work.
					    	output.at(k) = collatz(work.input1.at(k), work.input2);

					    	metrics_finishing_work(tid);
					  	}

					  metrics_thread_finished(tid);
				  	}

		    		break;
		    }

			metrics_repeat_finished();

			// for (i = 0; i < work.input1.size(); i++) {
			//     print(output.at(i));
			// }

			// Check if output is valid.
		    if (std::find(output.begin(), output.end(), 0) != output.end()) {
		    	print("\n\nWARNING - INCORRECT OUTPUT!!\n\n\n");
		    }
    	}

    	metrics_finished();
    }
}
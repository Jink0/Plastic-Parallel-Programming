#include <parallel_test.hpp>



#include "metrics.hpp"
#include "utils.hpp"

#include <stdint.h>
#include <omp.h>
#include <iostream>

#include <string>

#define CHUNKSIZE 500
#define N_THREADS 4
#define REPEATS   5

#include <deque>

#include "config_files_utils.hpp"

// Workloads for workload generation
#include "workloads.hpp"



int main(int argc, char *argv[]) {
	// Retrieve run parameters from given config file.
    struct run_parameters params = translate_run_parameters(read_config_file(argc, argv));

    std::cout << params;

    createFoldersAndMove(argv[1], "parallel_test");

    for (uint32_t i = 0; i < params.experiments.size(); i++) {
    	for (uint32_t j = 0; j < params.number_of_repeats; j++) {

    		// Create output filename.
    		std::string output_filename = ("Experiment" + to_string(i + 1) + "_Repeat" + to_string(j));

    		struct workload<int, int, int> work = generate_workload<int, int, int>(params.experiments.at(i));

		    deque<int> output(work.input1.size(), 0);

		    metrics_init(work.params.number_of_threads, output_filename);

		    uint32_t nthreads, tid, i;

			omp_set_dynamic(0);     // Explicitly disable dynamic teams
			omp_set_num_threads(work.params.number_of_threads); // Use 4 threads for all consecutive parallel regions

			#pragma omp parallel shared(work, output) private(i, tid) 
			{
			  	tid = omp_get_thread_num();

			  	metrics_thread_start(tid);

			  	if (tid == 0) {
			    	nthreads = omp_get_num_threads();
			    	print("Number of threads = ", nthreads, "\n");
			  	}

			  	print("Thread ", tid, " starting...\n");

			  	#pragma omp for schedule(dynamic, work.params.initial_chunk_size)
			  	for (i = 0; i < work.input1.size(); i++) {
			  		metrics_starting_work(tid);

			    	output.at(i) = collatz(work.input1.at(i), work.input2);

			    	metrics_finishing_work(tid);
			  	}

			  metrics_thread_finished(tid);
		  	}  /* end of parallel section */

		  	metrics_exit();

		 // for (i = 0; i < work.input1.size(); i++) {
		 //        print(output.at(i));
		 //    }

		    if (std::find(output.begin(), output.end(), 0) != output.end()) {
		    	std::cout << "\n\nWARNING - INCORRECT OUTPUT!!\n\n\n";
		    }
    	}
    }

    
}
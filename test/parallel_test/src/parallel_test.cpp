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

#include <deque>

#include "config_files_utils.hpp"

/*
 * Test user function
 */

int collatz(int weight, deque<int> seeds) {
    for (int i = 0; i < weight; i++)     {
        int start = seeds[0];

        if (start < 1) {
            fprintf(stderr,"Error, cannot start collatz with %d\n", start);

            return 0;
        }

        int count = 0;
        while (start != 1) {
            count++;
            if (start % 2) {
                start = 3 * start + 1;
            } else {
                start = start/2;
            }
        }
    }
    
    return 1;
}

// Structure to contain our workload.
template <typename in1, typename in2, typename out>
struct workload {
	// First input deque.
	deque<in1> input1;

	// Second input deque.
	deque<in2> input2;

	// User function pointer.
	out (*userFunction) (in1, deque<in2>);

	struct experiment_parameters params;
};

// 
template <typename in1, typename in2, typename out>
workload<in1, in2, out> generate_workload(struct experiment_parameters params) {
	struct workload<in1, in2, out> output;

	output.params = params;

	uint32_t quotient  = params.array_size / params.task_size_distribution.size();
	uint32_t remainder = params.array_size % params.task_size_distribution.size();

	for (uint32_t i = 0; i < params.task_size_distribution.size(); i++) {
		for (uint32_t j = 0; j < quotient; j++) {
			output.input1.push_back(params.task_size_distribution.at(i));
		}
	}

	for (uint32_t i = 0; i < remainder; i++) {
		output.input1.push_back(params.task_size_distribution.back());
	}

	output.input2.push_back(1);

	switch (params.user_function) {
		case Collatz:
			output.userFunction = collatz;

			break;
	}

	return output;
}

int main(int argc, char *argv[]) {
	// Retrieve run parameters from given config file.
    struct run_parameters params = translate_run_parameters(read_config_file(argc, argv));

    std::cout << params;

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

			metrics_finalise();

		  	metrics_calc();

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
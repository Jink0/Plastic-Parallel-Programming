#include <sequential_test.hpp>

#include <string>
#include <stdint.h>
#include <deque>

#include "utils.hpp"
#include "config_files_utils.hpp"
#include "workloads.hpp"
#include "metrics.hpp"



#ifdef DETAILED_METRICS
#define DM( x ) x
#else
#define DM( x )
#endif



int main(int argc, char *argv[]) {

    // Retrieve run parameters from given config file.
    struct run_parameters params = translate_run_parameters(read_config_file(argc, argv));

    // Print run parameters.
    print(params);

    // Move to output folder and copy our config to it.
    moveAndCopy(argv[1], "sequential_test");

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
            metrics_thread_start(0);

            // Sequential section start
            for (uint32_t k = 0; k < work.input1.size(); k++) {

                DM(metrics_starting_work(0));

                // Do work.
                output.at(k) = collatz(work.input1.at(k), work.input2);

                DM(metrics_finishing_work(0));
            }

            metrics_thread_finished(0);
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
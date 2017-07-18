#include <map_array_test.hpp>

#include <string>
#include <stdint.h>
#include <deque>

#include "map_array.hpp"

#include "utils.hpp"
#include "config_files_utils.hpp"
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
    moveAndCopy(argv[1], "map_array_test");

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

            // Start mapArray.
            map_array<int, int, int>(work, output);

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
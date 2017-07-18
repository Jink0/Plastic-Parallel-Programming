#include <map_array_test.hpp>

#include <string>
#include <stdint.h>
#include <deque>

#include "map_array.hpp"

#include "utils.hpp"
#include "config_files_utils.hpp"
#include "workloads.hpp"
#include "metrics.hpp"



#ifdef DETAILED_METRICS
  #define DM( x ) x
#else
  #define DM( x ) 
#endif





// // Structure to hold parameters for each experiment.
// struct eParameters {
//     // MapArray parameters.
//     struct parameters params;

//     // Size of array to use.
//     uint32_t array_size;

//     // Output filename.
//     string output_filename;
// };



// /*
//  * Prints the parameters of each experiment in the experiment deque.
//  */

// void printExperimentParameters(deque<eParameters> exParamsVector) 
// {
//     for (uint32_t i = 0; i < exParamsVector.size(); i++)
//     {
//         print(exParamsVector[i].output_filename, ":\n",
//               "\n\tNumber of threads: ", exParamsVector[i].params.thread_pinnings.size(),
//               "\n\tTask distribution: ", exParamsVector[i].params.task_dist,
//               "\n\tSchedule:          ");

//         if (exParamsVector[i].params.schedule == 0) 
//         {
//             print("Static\n\n\n");
//         }
//         else if (exParamsVector[i].params.schedule == 1)
//         {
//             print("Dynamic_chunks\n\n\n");
//         }
//         else if (exParamsVector[i].params.schedule == 2)
//         {
//             print("Dynamic_individual\n\n\n");
//         }
//         else if (exParamsVector[i].params.schedule == 3)
//         {
//             print("Tapered\n\n\n");
//         }
//         else if (exParamsVector[i].params.schedule == 4)
//         {
//             print("Auto\n\n\n");
//         }
//     }
// }



// /* 
//  * Reads the given config file and generates all of our experiment parameters.
//  */

// deque<eParameters> processConfig(char *argv[])
// {
//     /*
//      * Read experiment parameters.
//      */

//     // Setting property_tree namespace.
//     namespace pt = boost::property_tree;

//     // Create property tree and read our config file into it.
//     pt::ptree propTree;
//     pt::ini_parser::read_ini(argv[1], propTree);

//     // Retreive number of experiments from property tree.
//     uint32_t num_experiments = propTree.get<uint32_t>(pt::ptree::path_type("PARAMS/numExperiments", '/'));

//     // Get the number of repeats to do.
//     uint32_t repeats = propTree.get<uint32_t>(pt::ptree::path_type("PARAMS/repeats", '/'));

//     // Default experiment parameters container.
//     struct eParameters defaultParams;

//     int num_threads = propTree.get<int>(pt::ptree::path_type(     "DEFAULTS/numThreads", '/'));

//     deque<int> thread_pinnings;

//     for (int i = 0; i < num_threads; i++)
//     {
//         thread_pinnings.push_back(i);
//     }

//     // Read values from property tree.
//     defaultParams.params.thread_pinnings = thread_pinnings;
//     defaultParams.params.task_dist   = propTree.get<int>(pt::ptree::path_type(     "DEFAULTS/taskDistribution", '/'));
//     defaultParams.array_size         = propTree.get<uint32_t>(pt::ptree::path_type("DEFAULTS/arraySize", '/'));

//     // Reading schedule is more complex, as we need an enum value, so we switch on the string read.
//     string sched = propTree.get<string>(pt::ptree::path_type("DEFAULTS/schedule", '/'));

//     if (sched.compare("Static") == 0)
//     {
//         defaultParams.params.schedule = Static;
//     }
//     else if (sched.compare("Dynamic_chunks") == 0)
//     {
//         defaultParams.params.schedule = Dynamic_chunks;
//     }
//     else if (sched.compare("Dynamic_individual") == 0)
//     {
//         defaultParams.params.schedule = Dynamic_individual;
//     }
//     else if (sched.compare("Tapered") == 0)
//     {
//         defaultParams.params.schedule = Tapered;
//     }
//     else if (sched.compare("Auto") == 0)
//     {
//         defaultParams.params.schedule = Auto;
//     }
//     else
//     {
//         print("\nUnrecognised default schedule: ", sched, "\n\n");
//         exit(EXIT_FAILURE);
//     }



//     /*
//      * Generate deque of experiment parameters.
//      */

//     // deque of experiment parameters.
//     deque<eParameters> exParamsVector;

//     for (uint32_t i = 0; i < num_experiments; i++)
//     {
//         // Construct paths to parameter values.
//         string path1 = to_string(i + 1) + "/numThreads";
//         string path2 = to_string(i + 1) + "/taskDistribution";
//         string path3 = to_string(i + 1) + "/schedule";
//         string path4 = to_string(i + 1) + "/arraySize";

//         // Retrieving said values.
//         boost::optional<int> n_threads   = propTree.get_optional<int>(pt::ptree::path_type(path1, '/'));
//         boost::optional<int> t_dist      = propTree.get_optional<int>(pt::ptree::path_type(path2, '/'));
//         boost::optional<string> sch = propTree.get_optional<string>(pt::ptree::path_type(path3, '/'));
//         boost::optional<uint32_t> a_size = propTree.get_optional<uint32_t>(pt::ptree::path_type(path4, '/'));

//         // Current experiment parameters.
//         struct eParameters current;

//         // Since we are returning optional values, we must check they exist before assigning them. If they do not, we 
//         // use the default value. NOTE - SHOULD CHANGE SO WE DETECT IF NO NEW VALS TO CALCULATE num_experiments PROGRAMATICALLY
//         if (n_threads)
//         {
//             // current.params.num_threads = static_cast<int>(*n_threads);

//             deque<int> thread_pinnings;

//             for (int i = 0; i < static_cast<int>(*n_threads); i++)
//             {
//                 thread_pinnings.push_back(i);
//             }

//             current.params.thread_pinnings = thread_pinnings;
//         }
//         else
//         {
//             current.params.thread_pinnings = defaultParams.params.thread_pinnings;
//         }

//         if (t_dist)
//         {
//             current.params.task_dist = static_cast<int>(*t_dist);
//         }
//         else
//         {
//             current.params.task_dist = defaultParams.params.task_dist;
//         }

//         if (sch)
//         {
//             // Same deal as earlier with the schedule parameter.
//             sched = static_cast<string>(*sch);

//             if (sched.compare("Static") == 0)
//             {
//                 current.params.schedule = Static;
//             }
//             else if (sched.compare("Dynamic_chunks") == 0)
//             {
//                 current.params.schedule = Dynamic_chunks;
//             }
//             else if (sched.compare("Dynamic_individual") == 0)
//             {
//                 current.params.schedule = Dynamic_individual;
//             }
//             else
//             {
//                 print("\nUnrecognised default schedule: ", sched, "\n\n");
//                 exit(EXIT_FAILURE);
//             }
//         }
//         else
//         {
//             current.params.schedule = defaultParams.params.schedule;
//         }

//         if (a_size)
//         {
//             current.array_size = static_cast<uint32_t>(*a_size);
//         }
//         else
//         {
//             current.array_size = defaultParams.array_size;
//         }

//         // For however many repeats we want;
//         for (uint32_t r = 0; r < repeats; r++) 
//         {
//             // Set current output file
//             current.output_filename = ("Experiment" + to_string(i + 1) + "_Repeat" + to_string(r));

//             // Store current parameters in output deque.
//             exParamsVector.push_back(current);
//         }
//     }

//     // Return complete experiment deque.
//     return exParamsVector;
// }



// /*
//  * Creates a folder with the next valid name (the next run number), and moves into it.
//  */

// void createFolderAndMove()
// {
//     // Create runs directory if it doesn't exist.
//     path p("runs");
//     create_directory(p);

//     // Move into the runs directory.
//     current_path("runs");

//     // Directory name to start at.
//     int i = 1;
    
//     // Root directory word.
//     string root_dir_name = "run";

//     // Find what the next run number should be.
//     while (is_directory(root_dir_name + to_string(i).c_str()))
//     {
//         i++;
//     }

//     // Create our run directory.
//     path p2(root_dir_name + to_string(i).c_str());
//     create_directory(p2);

//     // Move into our run directory.
//     current_path(root_dir_name + to_string(i).c_str());
// }



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
    // for (uint32_t j = 0; j < params.number_of_repeats; j++) {

        // Generate workload.
        struct workload<int, int, int> work = generate_workload<int, int, int>(params.experiments.at(i));

        // Create output deque.
        deque<int> output(work.input1.size(), 0);

        metrics_repeat_start(params.experiments.at(i).number_of_threads);

        // Start mapArray.
        map_array<int, int, int>(work, output, params.experiments.at(0), "TEST");
        // map_array<int, int, int>(work);

      // metrics_thread_start(0);

      // // Sequential section start
      // for (uint32_t k = 0; k < work.input1.size(); k++) {

      //   DM(metrics_starting_work(0));

      //   // Do work.
      //   output.at(k) = collatz(work.input1.at(k), work.input2);

      //   DM(metrics_finishing_work(0));
      // }

      // metrics_thread_finished(0);
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
  // }







    // Run each experiment.
    // for (uint32_t i = 0; i < exParamsVector.size(); i++)
    // {
    //     uint32_t as = exParamsVector[i].array_size;

    //     // Experiment input vectors.
    //     deque<int> input1(as);
    //     deque<int> input2(as);

    //     // Generate data for vectors.
    //     for (uint32_t i = 0; i < as; i++) 
    //     {
    //         // input1[i] = (double) 1. / i;
    //         // input2[i] = (double) 1. / i;
    //         // input1[i] = (int) i + 1;
    //         // input2[i] = (int) i + 1;
    //         input1[i] = 5000000;
    //         input2[i] = 87736;
    //     }

    //     // Output deque.
    //     deque<int> output(as);

    //     // Start mapArray.
    //     map_array(input1, input2, collatz, output, exParamsVector[i].output_filename, exParamsVector[i].params);

    //     for (uint32_t i = 0; i < as; i++) 
    //     {
    //         //print(output[i]);
    //     }

    //     print("\n\n");
    // }

    //sleep(10);
}
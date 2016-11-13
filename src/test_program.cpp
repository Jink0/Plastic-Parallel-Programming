#include <stdlib.h>
#include <stdio.h>
#include <time.h>
// For string functions
#include <string.h>

// For parsing config file
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

// Map array library for testing
#include "map_array.h"



// Structure to hold parameters for each experiment.
struct eParameters {
    // MapArray parameters.
    struct parameters params;

    // Size of array to use.
    uint32_t array_size;

    // Output filename.
    std::string output_filename;
};



/*
 * Prints the parameters of each experiment in the experiment vector.
 */

void printExperimentParameters(std::vector<eParameters> exParamsVector) 
{
    for (uint32_t i = 0; i < exParamsVector.size(); i++)
    {
        print(exParamsVector[i].output_filename, ":\n",
              "\n\tNumber of threads: ", exParamsVector[i].params.num_threads,
              "\n\tTask distribution: ", exParamsVector[i].params.task_dist,
              "\n\tSchedule:          ");

        if (exParamsVector[i].params.schedule == 0) 
        {
            print("Static\n\n\n");
        }
        else if (exParamsVector[i].params.schedule == 1)
        {
            print("Dynamic_chunks\n\n\n");
        }
        else if (exParamsVector[i].params.schedule == 2)
        {
            print("Dynamic_individual\n\n\n");
        }
    }
}



/* 
 * Reads the given config file and generates all of our experiment parameters.
 */

std::vector<eParameters> processConfig(char *argv[])
{
    /*
     * Read experiment parameters.
     */

    // Setting property_tree namespace.
    namespace pt = boost::property_tree;

    // Create property tree and read our config file into it.
    pt::ptree propTree;
    pt::ini_parser::read_ini(argv[1], propTree);

    // Retreive number of experiments from property tree.
    uint32_t num_experiments = propTree.get<uint32_t>(pt::ptree::path_type("PARAMS/numExperiments", '/'));

    // Get the number of repeats to do.
    uint32_t repeats = propTree.get<uint32_t>(pt::ptree::path_type("PARAMS/repeats", '/'));

    // Default experiment parameters container.
    struct eParameters defaultParams;

    // Read values from property tree.
    defaultParams.params.num_threads = propTree.get<int>(pt::ptree::path_type(     "DEFAULTS/numThreads", '/'));
    defaultParams.params.task_dist   = propTree.get<int>(pt::ptree::path_type(     "DEFAULTS/taskDistribution", '/'));
    defaultParams.array_size         = propTree.get<uint32_t>(pt::ptree::path_type("DEFAULTS/arraySize", '/'));

    // Reading schedule is more complex, as we need an enum value, so we switch on the string read.
    std::string sched = propTree.get<std::string>(pt::ptree::path_type("DEFAULTS/schedule", '/'));

    if (sched.compare("Static") == 0)
    {
        defaultParams.params.schedule = Static;
    }
    else if (sched.compare("Dynamic_chunks") == 0)
    {
        defaultParams.params.schedule = Dynamic_chunks;
    }
    else if (sched.compare("Dynamic_individual") == 0)
    {
        defaultParams.params.schedule = Dynamic_individual;
    }
    else
    {
        print("\nUnrecognised default schedule: ", sched, "\n\n");
        exit(EXIT_FAILURE);
    }



    /*
     * Generate vector of experiment parameters.
     */

    // Vector of experiment parameters.
    std::vector<eParameters> exParamsVector;

    for (uint32_t i = 0; i < num_experiments; i++)
    {
        // Construct paths to parameter values.
        std::string path1 = std::to_string(i + 1) + "/numThreads";
        std::string path2 = std::to_string(i + 1) + "/taskDistribution";
        std::string path3 = std::to_string(i + 1) + "/schedule";
        std::string path4 = std::to_string(i + 1) + "/arraySize";

        // Retrieving said values.
        boost::optional<int> n_threads   = propTree.get_optional<int>(pt::ptree::path_type(path1, '/'));
        boost::optional<int> t_dist      = propTree.get_optional<int>(pt::ptree::path_type(path2, '/'));
        boost::optional<std::string> sch = propTree.get_optional<std::string>(pt::ptree::path_type(path3, '/'));
        boost::optional<uint32_t> a_size = propTree.get_optional<uint32_t>(pt::ptree::path_type(path4, '/'));

        // Current experiment parameters.
        struct eParameters current;

        // Since we are returning optional values, we must check they exist before assigning them. If they do not, we 
        // use the default value. NOTE - SHOULD CHANGE SO WE DETECT IF NO NEW VALS TO CALCULATE num_experiments PROGRAMATICALLY
        if (n_threads)
        {
            current.params.num_threads = static_cast<int>(*n_threads);
        }
        else
        {
            current.params.num_threads = defaultParams.params.num_threads;
        }

        if (t_dist)
        {
            current.params.task_dist = static_cast<int>(*t_dist);
        }
        else
        {
            current.params.task_dist = defaultParams.params.task_dist;
        }

        if (sch)
        {
            // Same deal as earlier with the schedule parameter.
            sched = static_cast<std::string>(*sch);

            if (sched.compare("Static") == 0)
            {
                current.params.schedule = Static;
            }
            else if (sched.compare("Dynamic_chunks") == 0)
            {
                current.params.schedule = Dynamic_chunks;
            }
            else if (sched.compare("Dynamic_individual") == 0)
            {
                current.params.schedule = Dynamic_individual;
            }
            else
            {
                print("\nUnrecognised default schedule: ", sched, "\n\n");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            current.params.schedule = defaultParams.params.schedule;
        }

        if (a_size)
        {
            current.array_size = static_cast<uint32_t>(*a_size);
        }
        else
        {
            current.array_size = defaultParams.array_size;
        }

        // For however many repeats we want;
        for (uint32_t r = 0; r < repeats; r++) 
        {
            // Set current output file
            current.output_filename = ("Experiment" + std::to_string(i + 1) + "_Repeat" + std::to_string(r));

            // Store current parameters in output vector
            exParamsVector.push_back(current);
        }
    }

    // Return complete experiment vector
    return exParamsVector;
}



/*
 *  Test user function
 */

double userFunction(double in1, vector<double> in2)
{
    return in1 + in2[0];
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        print("\nInvalid command line arguemnts!\n\n");
        exit(EXIT_FAILURE);
    }

    // Process given configuration file.
    std::vector<eParameters> exParamsVector = processConfig(argv);

    // Print our experiment parameters.
    printExperimentParameters(exParamsVector);

    // Run each experiment.
    for (uint32_t i = 0; i < exParamsVector.size(); i++)
    {
        // Experiment input vectors.
        vector<double> input1(exParamsVector[i].array_size);
        vector<double> input2(exParamsVector[i].array_size);

        // Generate data for vectors.
        for (uint32_t i = 0; i < exParamsVector[i].array_size; i++) 
        {
            input1[i] = (double) 1. / i;
            input2[i] = (double) 1. / i;
        }

        // Output vector.
        vector<double> output(exParamsVector[i].array_size);

        // Start mapArray
        map_array(input1, input2, userFunction, output, exParamsVector[i].output_filename, exParamsVector[i].params);
    }

    return 0;
}

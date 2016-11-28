#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <unistd.h>

// For string functions
#include <string.h>

// For parsing config file
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

// Map array library for testing
#include "map_array.h"

#include <sys/stat.h>



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

            // Store current parameters in output vector.
            exParamsVector.push_back(current);
        }
    }

    // Return complete experiment vector.
    return exParamsVector;
}



void createFolderAndMove()
{
    // Buffer for stat().
    struct stat buf;

    // Directory name to start at.
    int i = 1;
    
    // Root directory word.
    std::string root_dir_name = "run";

    // While a directory exists with the name rootWord + i, iterate i. This finds what we should call the new directory.
    while (stat((root_dir_name + to_string(i)).c_str(), &buf) == 0 && S_ISDIR(buf.st_mode))
    {
        i++;
    }

    // Create our new directory.
    mkdir((root_dir_name + to_string(i)).c_str(), 0775);

    // Move into it to store our output files here.
    chdir((root_dir_name + to_string(i)).c_str());
}



/*
 *  Test user function
 */

double userFunction(double in1, vector<double> in2)
{
    return in1 + in2[0];
}



/*
 * Return number of iterations of Collatz function
 * necessary to reach 1. This assumes that the Collatz
 * conjecture is true, so only goes through a finite number
 * of calls (this has been tested up to 2^60)
 */

int collatz(int start, vector<int> temp) 
{
    if (start < 1) 
    {
        fprintf(stderr,"Error, cannot start collatz with %d\n", start);
        return -1;
    }

    int count = 0;
    while (start != 1) 
    {
        count++;
        if (start % 2) 
        {
            start = 3 * start + 1;
        } 
        else 
        {
            start = start/2;
        }
    }

    return count;
}



/*
 * Iterative version of collatz, could potentially cause stack
 * overflow if particularly long sequence!
 */
int collatz_iter(int start, vector<int> temp) 
{
    if (start < 1) 
    {
        fprintf(stderr,"Error, cannot start collatz with %d\n", start);
        return -1;
    }

    if (start == 1) 
    {
        return 0;
    } 
    else if (start % 2) 
    {
        return 1 + collatz(3 * start + 1, temp);
    } 
    else 
    {
        return 1 + collatz( start / 2, temp);
    }
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

    // Create a folder for this run's output logs, and change the current working directory to it.
    createFolderAndMove();

    // Run each experiment.
    for (uint32_t i = 0; i < exParamsVector.size(); i++)
    {
        uint32_t as = exParamsVector[i].array_size;

        // Experiment input vectors.
        vector<int> input1(as);
        vector<int> input2(as);

        // Generate data for vectors.
        for (uint32_t i = 0; i < as; i++) 
        {
            // input1[i] = (double) 1. / i;
            // input2[i] = (double) 1. / i;
            input1[i] = (int) i + 1;
            input2[i] = (int) i + 1;
        }

        // Output vector.
        vector<int> output(as);

        // Start mapArray.
        map_array(input1, input2, collatz, output, exParamsVector[i].output_filename, exParamsVector[i].params);
    }

    return 0;
}
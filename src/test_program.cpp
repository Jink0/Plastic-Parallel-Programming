#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "map_array.h"



void printParameters(struct parameters params, int array_size) 
{
    print("Experiment parameters: \n\nArray size: ", array_size, 
        "\nNumber of threads: ", params.num_threads, 
        "\nTask distribution: ", params.task_dist,
        "\nSchedule: ");

    if (params.schedule == 0) 
    {
        print("Static\n\n");
    }
    else if (params.schedule == 1)
    {
        print("Dynamic_chunks\n\n");
    }
    else if (params.schedule == 2)
    {
        print("Dynamic_individual\n\n");
    }
}



/*
 *  Test user function
 */

double userFunction(double in1, vector<double> in2)
{
    return in1 + in2[0];
}



// Experiment parameters
struct eParameters {
    // MapArray parameters
    struct parameters params;

    // Size of array to use
    uint32_t array_size;

    // Output filename
    char *output_file;
};



void processConfig(struct eParameters* exParams, char *argv[])
{
    namespace pt = boost::property_tree;

    pt::ptree propTree;
    pt::ini_parser::read_ini(argv[1], propTree);

    struct eParameters defaultParams;

    defaultParams.params.num_threads = propTree.get<int>(pt::ptree::path_type("DEFAULTS/numThreads", '/'));
    defaultParams.params.task_dist   = propTree.get<int>(pt::ptree::path_type("DEFAULTS/taskDistribution", '/'));

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

    defaultParams.array_size  = propTree.get<uint32_t>(pt::ptree::path_type("DEFAULTS/arraySize", '/'));
    defaultParams.output_file = const_cast<char*>(propTree.get<std::string>(pt::ptree::path_type("DEFAULTS/outputFilesRootWord", '/')).c_str());

    int num_experiments = propTree.get<int>(pt::ptree::path_type("DEFAULTS/numExperiments", '/'));

    exParams = new eParameters[num_experiments];

    for (int i = 0; i < num_experiments; i++)
    {
        std::string path1 = std::to_string(i + 1) + "/numThreads";
        std::string path2 = std::to_string(i + 1) + "/taskDistribution";
        std::string path3 = std::to_string(i + 1) + "/schedule";
        std::string path4 = std::to_string(i + 1) + "/arraySize";

        boost::optional<int> n_threads   = propTree.get_optional<int>(pt::ptree::path_type(path1, '/'));
        boost::optional<int> t_dist      = propTree.get_optional<int>(pt::ptree::path_type(path2, '/'));
        boost::optional<std::string> sch = propTree.get_optional<std::string>(pt::ptree::path_type(path3, '/'));
        boost::optional<uint32_t> a_size = propTree.get_optional<uint32_t>(pt::ptree::path_type(path4, '/'));

        if (n_threads)
        {
            exParams[i].params.num_threads = static_cast<int>(*n_threads);
        }
        else
        {
            exParams[i].params.num_threads = defaultParams.params.num_threads;
        }

        if (t_dist)
        {
            exParams[i].params.task_dist = static_cast<int>(*t_dist);
        }
        else
        {
            exParams[i].params.task_dist = defaultParams.params.task_dist;
        }

        if (sch)
        {
            sched = static_cast<std::string>(*sch);

            if (sched.compare("Static") == 0)
            {
                exParams[i].params.schedule = Static;
            }
            else if (sched.compare("Dynamic_chunks") == 0)
            {
                exParams[i].params.schedule = Dynamic_chunks;
            }
            else if (sched.compare("Dynamic_individual") == 0)
            {
                exParams[i].params.schedule = Dynamic_individual;
            }
            else
            {
                print("\nUnrecognised default schedule: ", sched, "\n\n");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            exParams[i].params.schedule = defaultParams.params.schedule;
        }

        if (a_size)
        {
            exParams[i].array_size = static_cast<uint32_t>(*a_size);
        }
        else
        {
            exParams[i].array_size = defaultParams.array_size;
        }

        exParams[i].output_file = defaultParams.output_file;
    }
}



int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        print("\nInvalid command line arguemnts!\n\n");
        exit(EXIT_FAILURE);
    }

    namespace pt = boost::property_tree;

    pt::ptree propTree;
    pt::ini_parser::read_ini(argv[1], propTree);

    struct eParameters defaultParams;

    defaultParams.params.num_threads = propTree.get<int>(pt::ptree::path_type("DEFAULTS/numThreads", '/'));
    defaultParams.params.task_dist   = propTree.get<int>(pt::ptree::path_type("DEFAULTS/taskDistribution", '/'));

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

    defaultParams.array_size  = propTree.get<uint32_t>(pt::ptree::path_type("DEFAULTS/arraySize", '/'));
    defaultParams.output_file = const_cast<char*>(propTree.get<std::string>(pt::ptree::path_type("DEFAULTS/outputFilesRootWord", '/')).c_str());

    int num_experiments = propTree.get<int>(pt::ptree::path_type("DEFAULTS/numExperiments", '/'));

    eParameters *exParams = new eParameters[num_experiments];

    for (int i = 0; i < num_experiments; i++)
    {
        std::string path1 = std::to_string(i + 1) + "/numThreads";
        std::string path2 = std::to_string(i + 1) + "/taskDistribution";
        std::string path3 = std::to_string(i + 1) + "/schedule";
        std::string path4 = std::to_string(i + 1) + "/arraySize";

        boost::optional<int> n_threads   = propTree.get_optional<int>(pt::ptree::path_type(path1, '/'));
        boost::optional<int> t_dist      = propTree.get_optional<int>(pt::ptree::path_type(path2, '/'));
        boost::optional<std::string> sch = propTree.get_optional<std::string>(pt::ptree::path_type(path3, '/'));
        boost::optional<uint32_t> a_size = propTree.get_optional<uint32_t>(pt::ptree::path_type(path4, '/'));

        if (n_threads)
        {
            exParams[i].params.num_threads = static_cast<int>(*n_threads);
        }
        else
        {
            exParams[i].params.num_threads = defaultParams.params.num_threads;
        }

        if (t_dist)
        {
            exParams[i].params.task_dist = static_cast<int>(*t_dist);
        }
        else
        {
            exParams[i].params.task_dist = defaultParams.params.task_dist;
        }

        if (sch)
        {
            sched = static_cast<std::string>(*sch);

            if (sched.compare("Static") == 0)
            {
                exParams[i].params.schedule = Static;
            }
            else if (sched.compare("Dynamic_chunks") == 0)
            {
                exParams[i].params.schedule = Dynamic_chunks;
            }
            else if (sched.compare("Dynamic_individual") == 0)
            {
                exParams[i].params.schedule = Dynamic_individual;
            }
            else
            {
                print("\nUnrecognised default schedule: ", sched, "\n\n");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            exParams[i].params.schedule = defaultParams.params.schedule;
        }

        if (a_size)
        {
            exParams[i].array_size = static_cast<uint32_t>(*a_size);
        }
        else
        {
            exParams[i].array_size = defaultParams.array_size;
        }

        exParams[i].output_file = defaultParams.output_file;
    }

    for (int i = 0; i < num_experiments; i++)
    {
        // Experiment input vectors
        vector<double> input1(exParams[i].array_size);
        vector<double> input2(exParams[i].array_size);

        // Generate data for vectors
        for (int i = 0; i < exParams[i].array_size; i++) 
        {
            input1[i] = (double) 1. / i;
            input2[i] = (double) 1. / i;
        }

        // Output vector
        vector<double> output(exParams[i].array_size);

        char exp_num[(int)((ceil(log10(i + 1))+1)*sizeof(char))];
        sprintf(exp_num, "%d", i + 1);

        strcat(exParams[i].output_file, exp_num);
        cout << exParams[i].output_file << endl;

        // Start mapArray
        map_array(input1, input2, userFunction, output, exParams[i].params, exParams[i].output_file);
    }

    return 0;
}

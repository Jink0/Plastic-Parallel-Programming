#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "map_array.h"

enum TaskSizeDist {Uniform};

enum Schedule {Static, Dynamic_chunks, Dynamic_individual};

// Experiment parameters with default values
struct parameters {
    // Size of our array to work on
    double array_size           = 100;

    // Number of threads to use
    int num_threads             = 8;

    // Minimum possible task size 
    double min_task_size        = 0;

    // Maximum possible task size
    double max_task_size        = 0;

    // Distribution of the tasks
    TaskSizeDist task_size_dist = Uniform;

    // Schedule to use, Static - Give each thread equal portions
    //                  Dynamic_chunks - Threads dynamically retrive a chunk of the taskswhen they can
    //                  Dynamic_individual - Threads retrieve a single task when they can
    Schedule schedule           = Static;
} params;



void processCommandLineArgs(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "-as") 
        {
            i++;
            params.array_size = stod(argv[i]);
        }
        else if (arg == "-t")
        {
            i++;
            params.num_threads = stoi(argv[i]);
        }
        else if (arg == "-mints")
        {
            i++;
            params.min_task_size = stod(argv[i]);
        }
        else if (arg == "-maxts")
        {
            i++;
            params.max_task_size = stod(argv[i]);
        }
        else if (arg == "-tdist")
        {
            i++;

            if (argv[i] == "Uniform")
            {   
                params.task_size_dist = Uniform;
            }
            else
            {
                print("Unrecognised argument: ", argv[i]);
                // exit(EXIT_FAILURE);
            }
        }
        else if (arg == "-s")
        {
            i++;

            if (strcmp(argv[i], "Static"))
            {
                params.schedule = Static;
            }
            else if (argv[i] == "Dynamic_chunks")
            {
                params.schedule = Dynamic_chunks;
            }
            else if (argv[i] == "Dynamic_individual")
            {   
                params.schedule = Dynamic_individual;
            }
            else
            {
                print("Unrecognised argument: ", argv[i]);
                // exit(EXIT_FAILURE);
            }
        }
        else 
        {
            print("Unrecognised argument: ", argv[i]);
            // exit(EXIT_FAILURE);
        }
    }
}

void printParameters() 
{
    print("Experiment parameters: \n\nArray size: ", params.array_size, 
        "\nNumber of threads: ", params.num_threads, 
        "\nMin task size: ", params.min_task_size, 
        "\nMax task size: ", params.max_task_size,
        "\nTask distribution: ", params.task_size_dist,
        "\nSchedule: ", params.schedule, "\n\n");
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
    processCommandLineArgs(argc, argv);

    printParameters();

    // Experiment input vectors
    vector<double> input1(params.array_size);
    vector<double> input2(params.array_size);

    // Generate data for vectors
    for (int i = 0; i < params.array_size; i++) 
    {
        input1[i] = (double) 1. / i;
        input2[i] = (double) 1. / i;
    }

    // Output vector
    vector<double> output(params.array_size);

    // Start mapArray
    map_array(input1, input2, userFunction, output);

    return 0;
}

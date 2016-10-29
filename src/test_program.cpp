#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "map_array.h"



void processCommandLineArgs(int argc, char *argv[], struct parameters &params, int &array_size, char *output_file)
{
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "-as") 
        {
            i++;
            array_size = stod(argv[i]);
            assert(array_size > 0);
        }
        else if (arg == "-t")
        {
            i++;
            params.num_threads = stoi(argv[i]);
            assert(params.num_threads >= 1);
        }
        else if (arg == "-tdist")
        {
            i++;
            params.task_dist= stoi(argv[i]);
            assert(params.task_dist >= 1 && params.task_dist <= 3);
        }
        else if (arg == "-s")
        {
            i++;

            if (strcmp(argv[i], "Static") == 0)
            {
                params.schedule = Static;
            }
            else if (strcmp(argv[i], "Dynamic_chunks") == 0)
            {
                params.schedule = Dynamic_chunks;
            }
            else if (strcmp(argv[i], "Dynamic_individual") == 0)
            {   
                params.schedule = Dynamic_individual;
            }
            else
            {
                print("Unrecognised argument: ", argv[i]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(argv[i], "-f") == 0)
        {
            i++;
            // strcpy(output_file, argv[i]);
        }
        else 
        {
            print("\nUnrecognised argument: ", argv[i], "\n\n");
            exit(EXIT_FAILURE);
        }
    }
}



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



int main(int argc, char *argv[])
{
    struct parameters params;

    int array_size = 1000;

    char *output_file = NULL;

    processCommandLineArgs(argc, argv, params, array_size, output_file);

    printParameters(params, array_size);

    // Experiment input vectors
    vector<double> input1(array_size);
    vector<double> input2(array_size);

    // Generate data for vectors
    for (int i = 0; i < array_size; i++) 
    {
        input1[i] = (double) 1. / i;
        input2[i] = (double) 1. / i;
    }

    // Output vector
    vector<double> output(array_size);

    // Start mapArray
    map_array(input1, input2, userFunction, output, params, output_file);

    return 0;
}

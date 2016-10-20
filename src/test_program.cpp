#include <stdlib.h>
#include <stdio.h>
#include <time.h>

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



template <typename T>
class EnumParser
{
    map <string, T> enumMap;
public:
    EnumParser(){};

    T ParseSomeEnum(const string &value)
    { 
        map <string, T>::const_iterator iValue = enumMap.find(value);

        if (iValue  == enumMap.end())
        {
            throw runtime_error("");
        }

        return iValue->second;
    }
};




EnumParser<TaskSizeDist>::EnumParser()
{
    enumMap["Uniform"] = Uniform;
};



EnumParser<Schedule>::EnumParser()
{
    enumMap["Static"]             = Static;
    enumMap["Dynamic_chunks"]     = Dynamic_chunks;
    enumMap["Dynamic_individual"] = Dynamic_individual;
};


void processCommandLineArgs(int argc, char *argv[])
{
    for (int i = 0; i < argc; i++)
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
            EnumParser<TaskSizeDist> parser1;

            i++;
            params.task_size_dist = parser1.ParseSomeEnum(argv[i]);
        }
        else if (arg == "-s")
        {
            EnumParser<Schedule> parser2;

            i++;
            params.schedule = parser2.ParseSomeEnum(argv[i]);
        }
        else 
        {
            print("Unrecognised argument: ", argv[i]);
        }
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
    processCommandLineArgs(argc, argv);

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

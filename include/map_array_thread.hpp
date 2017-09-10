#ifndef MAP_ARRAY_THREAD_HPP
#define MAP_ARRAY_THREAD_HPP

#include <iostream>         // cout
#include <vector>           // Vectors
#include <deque>            // Double ended queues
#include <mutex>            // mutexes
#include <pthread.h>        // Thread and mutex functions
#include <unistd.h>         // sleep()
#include <boost/thread.hpp> // boost::thread::hardware_concurrency();
#include <string>
#include <iostream>

#include <utils.hpp>
#include <config_files_utils.hpp>

#include "metrics.hpp"
#include <workloads.hpp>



#ifdef DETAILED_METRICS
#define DM( x ) x
#else
#define DM( x )
#endif



/*
 * This file contains the definition of the map array pattern. Note - all the definitions are in the header file, as
 * you cannot separate the definition of a template class from its declaration and put it inside a .cpp file.
 */

/*
 * Data structures
 */

// Thread control enum. threads either run (execute), change strategies (update), pause (sleep), or stop (terminate).
enum Thread_Control {Execute, Update, Sleep, Terminate};



// Structure to contain a group of tasks.
template <typename in1, typename in2, typename out>
struct tasks {

    // Start of input.
    typename std::deque<in1>::iterator in1Begin;

    // End of input.
    typename std::deque<in1>::iterator in1End;

    // Pointer to shared input2 deque.
    std::deque<in2>* input2;

    // User function pointer.
    out (*userFunction) (in1, std::deque<in2>);

    // Start of output.
    typename std::deque<out>::iterator outBegin;
};



/*
 * Classes
 */



// Bag of tasks class. Also contains shard variables for communicating with worker threads.
template <class in1, class in2, class out>
class BagOfTasks {
public:
    // Variables to control if threads terminate.
    std::deque<Thread_Control> thread_control;

    // Check for if bag is empty.
    bool empty = false;

    // Constructor using just a workload and an output deque.
    BagOfTasks(struct workload<in1, in2, out>& work,
               std::deque<out>& output) :

        in1Begin(work.input1.begin()),
        in1End(work.input1.end()),
        input2(&work.input2),
        userFunction(work.userFunction),
        outBegin(output.begin()), 
        task_iterators(1, std::pair<typename std::deque<in1>::iterator, typename std::deque<in1>::iterator>(work.input1.begin(), work.input1.end())),
        output_iterators(1, output.begin())
    {
        thread_control.assign(work.params.number_of_threads, Execute);
        num_tasks_in_bag = in1End - in1Begin;
    }

    // Constructor
    BagOfTasks(typename std::deque<in1>::iterator in1B,
               typename std::deque<in1>::iterator in1E,
               std::deque<in2>* in2p,
               out (*userF) (in1, std::deque<in2>),
               typename std::deque<out>::iterator outB,
               uint32_t num_threads) :

        in1Begin(in1B),
        in1End(in1E),
        input2(in2p),
        userFunction(userF),
        outBegin(outB),
        task_iterators(1, std::pair<typename std::deque<in1>::iterator, typename std::deque<in1>::iterator>(in1B, in1E)),
        output_iterators(1, outB)
    {
        thread_control.assign(num_threads, Execute);
        num_tasks_in_bag = in1End - in1Begin;
    }

    // Destructor
    ~BagOfTasks() {};

    // Overloads << operator for easy printing with streams. Used for development.
    friend std::ostream& operator<< (std::ostream &outS, BagOfTasks<in1, in2, out> &bot) {

        // Get mutex. When control leaves the scope in which the lock_guard object was created, the lock_guard is destroyed and the mutex is released.
        std::lock_guard<std::mutex> lock(bot.m);

        // Print all our important data.
        return outS << "Bag of tasks: " << std::endl
                    << "Data types - <" << type_name<in1>() << ", "
                    << type_name<in2>() << ", "
                    << type_name<out>() << ">" << std::endl
                    << "Number of tasks - " << bot.in1End - bot.in1Begin << std::endl << std::endl;
    };

    uint32_t numTasksRemaining() {

        // Get mutex.
        std::lock_guard<std::mutex> lock(m);

        // Return value.
        return in1End - in1Begin;
    }

    // // Returns tasks of the specified count or less.
    // tasks<in1, in2, out> getTasks(uint32_t num) {

    //     // Get mutex.
    //     std::lock_guard<std::mutex> lock(m);

    //     // Record where we should start in our task list.
    //     typename std::deque<in1>::iterator tasksBegin = in1Begin;

    //     uint32_t num_tasks;

    //     // Calculate number of tasks to return.
    //     if (num < in1End - in1Begin) {
    //         num_tasks = num;

    //     } else {
    //         num_tasks = in1End - in1Begin;
    //     }

    //     // Check if the bag is empty.
    //     if (num_tasks == 0) {
    //         empty = true;
    //     }

    //     // Advance our iterator so it now marks the end of our tasks.
    //     advance(in1Begin, num_tasks);

    //     // Create tasks data structure to return.
    //     struct tasks<in1, in2, out> output = {
    //         tasksBegin,
    //         in1Begin,
    //         input2,
    //         userFunction,
    //         outBegin
    //     };

    //     // Advance the output iterator to output in the correct place next time.
    //     advance(outBegin, num_tasks);

    //     return output;
    // };



    // Returns tasks of the specified count or less.
    std::deque<tasks<in1, in2, out>> get_tasks(uint32_t num) {

        // Get mutex.
        std::lock_guard<std::mutex> lock(m);

        std::deque<tasks<in1, in2, out>> output;

        // Check if we have any tasks left.
        if (num_tasks_in_bag == 0) {
            return output;
        }

        uint32_t num_tasks;

        // Calculate number of tasks to return.
        if (num < num_tasks_in_bag) {
            num_tasks = num;

        } else {
            num_tasks = num_tasks_in_bag;
        }

        uint32_t count = 0;
        uint32_t i = 0;

        // Calculate which iterator pairs to return.
        while (count < num_tasks && i < task_iterators.size()) {
            count += task_iterators.at(i).second - task_iterators.at(i).first;
            i++;
        }

        // Calculate remainder of last iterator pair.
        uint32_t remainder = (task_iterators.at(i - 1).second - task_iterators.at(i - 1).first) + num_tasks - count;

        // If we will be left with a remainder, split the last iterator pair.
        if (remainder < count) {

            // Record the start of the remainder.
            typename std::deque<in1>::iterator remainder_begin = task_iterators.at(i - 1).first;

            // Advance to the end of the remainder.
            advance(task_iterators.at(i - 1).first, remainder);

            // Insert new iterator pair to effectively split the last pair we need.
            task_iterators.insert(task_iterators.begin() + (i - 1), std::pair<typename std::deque<in1>::iterator, typename std::deque<in1>::iterator>(remainder_begin, task_iterators.at(i - 1).first));

            // Duplicate and advance relevant output iterator.
            output_iterators.insert(output_iterators.begin() + (i - 1), output_iterators.at(i - 1));

            advance(output_iterators.at(i), remainder);

        }

        // Add pairs to output.
        for (uint32_t j = 0; j < i; j++) {

            // Create tasks data structure to add to output.
            struct tasks<in1, in2, out> temp = {
                task_iterators.front().first,
                task_iterators.front().second,
                input2,
                userFunction,
                output_iterators.front()
            };

            // Pop iterators.
            task_iterators.pop_front();
            output_iterators.pop_front();

            // Add to output.
            output.push_back(temp);
        }

        // Update the number of tasks in the bag.
        num_tasks_in_bag -= num_tasks;

        return output;
    };



    // Returns tasks into the bag.
    void return_tasks(std::deque<tasks<in1, in2, out>> returned_tasks) {

        // Get mutex.
        std::lock_guard<std::mutex> lock(m);

        print("\nReturning ", returned_tasks.size(), " set(s) of tasks, ");

        // Add tasks to the front.
        while (returned_tasks.size() != 0) {

            // Add iterator pair to bag.
            task_iterators.push_front(std::pair<typename std::deque<in1>::iterator, typename std::deque<in1>::iterator>(returned_tasks.back().in1Begin, returned_tasks.back().in1End));

            // Add corresponding output iterators.
            output_iterators.push_front(returned_tasks.back().outBegin);

            // Update task total.
            num_tasks_in_bag += returned_tasks.back().in1End - returned_tasks.back().in1Begin;

            returned_tasks.pop_back();
        }

        print("bag contains ", task_iterators.size(), " set(s) with ", num_tasks_in_bag, " total tasks\n");
    };



    uint32_t read_num_tasks_in_bag() const { 

        // Get mutex. Necessary?
        // std::lock_guard<std::mutex> lock(m);

        return num_tasks_in_bag; 
    };



private:
    std::mutex m;

    typename std::deque<in1>::iterator in1Begin;
    typename std::deque<in1>::iterator in1End;

    std::deque<in2>* input2;

    out (*userFunction) (in1, std::deque<in2>);

    typename std::deque<out>::iterator outBegin;

    std::deque<std::pair<typename std::deque<in1>::iterator, typename std::deque<in1>::iterator>> task_iterators;
    std::deque<typename std::deque<in1>::iterator> output_iterators;

    uint32_t num_tasks_in_bag;
};




#define NUM_STATUSES 3

enum Status {Alive, Sleeping, Terminated};

const std::string statuses[NUM_STATUSES] = {"Alive", "Sleeping", "Terminated"};

const std::string bools[2] = {"False", "True"};



// Data struct to pass to each thread.
template <typename in1, typename in2, typename out>
struct thread_data {

    // Id of this thread.
    uint32_t threadId;

    // CPU for thread to run on.
    uint32_t cpu_affinity;

    // Starting chunk size of tasks to retrieve.
    uint32_t chunk_size;

    // Check for tapered schedule.
    bool tapered_schedule = false;

    // Pointer to the shared bag of tasks object.
    BagOfTasks<in1, in2, out> *bot;

    // Flag which main thread will set to indicate new instructions.
    bool check_for_new_instructions = false;

    Status status = Alive;
};

// // Overloads << operator for easy printing with streams. Used for development.
// template <typename in1, typename in2, typename out>
// std::ostream& operator<< (std::ostream &outS, thread_data<in1, in2, out> &td) {

//     // Print our data.
//     return outS << "Thread data for thread " << td.threadId << ":" << std::endl
//                 << "CPU affinity:     " << td.cpu_affinity << std::endl
//                 << "Chunk size:       " << td.chunk_size << std::endl
//                 << "Tapered_schedule: " << bools[td.tapered_schedule] << std::endl
//                 << "New insts check:  " << bools[td.check_for_new_instructions] << std::endl
//                 << "Status:           " << statuses[td.status] << std::endl << std::endl;
// };



/*
 * Functions
 */



std::deque<uint32_t> calc_chunks(experiment_parameters params, uint32_t num_tasks = 0) {

    if (num_tasks == 0) {
        num_tasks = num_tasks;
    }

    // Output deque of chunk sizes, one for each thread.
    std::deque<uint32_t> output(params.number_of_threads);

    switch (params.initial_schedule){
        case Static:
        {
            // Calculate info for data partitioning.
            uint32_t quotient  = num_tasks / params.number_of_threads;
            uint32_t remainder = num_tasks % params.number_of_threads;

            for (uint32_t i = 0; i < params.number_of_threads; i++) {

                // If we still have remainder tasks, add one to this thread.
                if (i < remainder) {
                    output.at(i) = quotient + 1;

                } else {
                    output.at(i) = quotient;
                }
            }
        }

        break;

        case Dynamic_chunks:
        {
            // If the chunk size is set to zero, guess at a reasonable chunk size.
            if (params.initial_chunk_size == 0) {
                params.initial_chunk_size = num_tasks / (params.number_of_threads * 10);
            }

            // Set chunk sizes.
            for (uint32_t i = 0; i < params.number_of_threads; i++) {
                output.at(i) = params.initial_chunk_size;
            }
        }

        break;

        case Tapered:
        {
            // Calculate info for data partitioning.
            uint32_t quotient = num_tasks / (params.number_of_threads * 2);

            // Set chunk sizes.
            for (uint32_t i = 0; i < params.number_of_threads; i++) {
                output.at(i) = quotient;
            }
        }

        break;

        case Auto:
        {
            print("\n\n\n*********************\n\nAuto schedule not implemented yet!\n\n*********************\n\n\n\n");
        }

        break;
    }

    return output;
}



template <typename in1, typename in2, typename out>
std::deque<thread_data<in1, in2, out>> calc_thread_data(BagOfTasks<in1, in2, out> &bot, experiment_parameters params) {

    // Calculate info for data partitioning.
    std::deque<uint32_t> chunks = calc_chunks(params, bot.numTasksRemaining());

    // Output deque of thread data
    std::deque<thread_data<in1, in2, out>> output;

    // Set thread data values.
    for (uint32_t i = 0; i < params.number_of_threads; i++) {

        struct thread_data<in1, in2, out> iter_data;

        iter_data.threadId     = i;
        iter_data.chunk_size   = chunks.at(i);
        iter_data.bot          = &bot;
        iter_data.cpu_affinity = params.thread_pinnings.at(i);

        if (params.initial_schedule == Tapered) {
            iter_data.tapered_schedule = true;
        }

        output.push_back(iter_data);
    }

    return output;
}



// // Function to start each thread of mapArray on.
// template <typename in1, typename in2, typename out>
// void mapArrayThread(thread_data<in1, in2, out> my_data) {

//     // Initialise metrics.
//     metrics_thread_start(my_data.cpu_affinity);

//     // Stick to our designated CPU.
//     stick_this_thread_to_cpu(my_data.cpu_affinity);

//     // Get tasks.
//     tasks<in1, in2, out> my_tasks = my_data.bot->getTasks(my_data.chunk_size);

//     // Print hello, and the number of tasks.
//     print("[Thread ", my_data.threadId, "] Hello! \n[Thread ", my_data.threadId, "] Initial chunk size: ", my_tasks.in1End - my_tasks.in1Begin, "\n");

//     // Calculate taper.
//     uint32_t tapered_chunk_size = my_data.chunk_size / 2;

//     // While we have tasks to do;
//     while (my_tasks.in1End - my_tasks.in1Begin > 0 ) {

//         // Run between iterator ranges, stepping through input1 and output vectors.
//         for (; my_tasks.in1Begin != my_tasks.in1End; ++my_tasks.in1Begin, ++my_tasks.outBegin) {

//             DM(metrics_starting_work(my_data.threadId));

//             // Run user function.
//             *(my_tasks.outBegin) = my_tasks.userFunction(*(my_tasks.in1Begin), *(my_tasks.input2));

//             DM(metrics_finishing_work(my_data.threadId));
//         }

//         // If we should still be executing, get more tasks!
//         if (my_data.bot->thread_control.at(my_data.threadId) == Execute) {

//             // Check for tapered schedule.
//             if (my_data.tapered_schedule) {

//                 my_tasks = my_data.bot->getTasks(tapered_chunk_size);

//                 print("[Thread ", my_data.threadId, "] Chunk size: ", tapered_chunk_size, "\n");

//                 // Recalculate taper.
//                 if (tapered_chunk_size > 1) {

//                     tapered_chunk_size = tapered_chunk_size / 2;
//                 }
//             } else {
//                 my_tasks = my_data.bot->getTasks(my_data.chunk_size);

//                 print("[Thread ", my_data.threadId, "] Chunk size: ", my_data.chunk_size, "\n");
//             }
//         }
//     }

//     metrics_thread_finished(my_data.cpu_affinity);

//     return;
// }



// Function to start each thread of mapArray on.
template <typename in1, typename in2, typename out>
void mapArrayThread2(thread_data<in1, in2, out> my_data) {

    // Initialise metrics.
    metrics_thread_start(my_data.cpu_affinity);

    // Stick to our designated CPU.
    stick_this_thread_to_cpu(my_data.cpu_affinity);

    // Get tasks.
    std::deque<tasks<in1, in2, out>> my_tasks = my_data.bot->get_tasks(my_data.chunk_size);

    print("\n[Thread ", my_data.threadId, "] Received ", my_tasks.size(), " sets of tasks with chunk size(s):\n");

    for (uint32_t i = 0; i < my_tasks.size(); i++) {
        print(my_tasks.at(i).in1End - my_tasks.at(i).in1Begin, "\n");
    }

    // Calculate taper.
    uint32_t tapered_chunk_size = my_data.chunk_size / 2;

    start:

    for (uint32_t i = 0; i < my_tasks.size(); i++) {

        // // While we have tasks to do;
        // while (my_tasks.at(i).in1End - my_tasks.at(i).in1Begin > i) {

        // Run between iterator ranges, stepping through input1 and output vectors.
        for (; my_tasks.at(i).in1Begin != my_tasks.at(i).in1End; ++my_tasks.at(i).in1Begin, ++my_tasks.at(i).outBegin) {

            DM(metrics_starting_work(my_data.threadId));

            // Run user function.
            *(my_tasks.at(i).outBegin) = my_tasks.at(i).userFunction(*(my_tasks.at(i).in1Begin), *(my_tasks.at(i).input2));

            DM(metrics_finishing_work(my_data.threadId));

            // Check if we should still be executing.
            if (my_data.bot->thread_control.at(my_data.threadId) != Execute) {

                my_data.bot->return_tasks(my_tasks);
                my_tasks.clear();

                switch (my_data.bot->thread_control.at(my_data.threadId)) {
                    case Update: {

                        break;
                    } 

                    case Sleep: {

                        break;
                    } 

                    case Terminate: {

                        goto end;

                        break;
                    }
                }
            }
        }
    }

    // Check for tapered schedule.
    if (my_data.tapered_schedule) {

        my_tasks.clear();

        my_tasks = my_data.bot->get_tasks(tapered_chunk_size);

        print("\n[Thread ", my_data.threadId, "] Received ", my_tasks.size(), " sets of tasks with chunk size(s):\n");

        for (uint32_t i = 0; i < my_tasks.size(); i++) {
            print(my_tasks.at(i).in1End - my_tasks.at(i).in1Begin, "\n");
        }

        // Recalculate taper.
        if (tapered_chunk_size > 1) {

            tapered_chunk_size = tapered_chunk_size / 2;
        }
    } else {

        my_tasks.clear();

        my_tasks = my_data.bot->get_tasks(my_data.chunk_size);

        print("\n[Thread ", my_data.threadId, "] Received ", my_tasks.size(), " sets of tasks with chunk size(s):\n");

        for (uint32_t i = 0; i < my_tasks.size(); i++) {
            print(my_tasks.at(i).in1End - my_tasks.at(i).in1Begin, "\n");
        }
    }

    if (my_tasks.size() != 0) {
        goto start;
    }

    end:

    metrics_thread_finished(my_data.cpu_affinity);

    return;
}

#endif // MAP_ARRAY_THREAD_HPP
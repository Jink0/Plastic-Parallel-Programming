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



#include <condition_variable> 



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

// Variables which define map_array_thread behavior. May be updated by controller.
struct work_data {

    // CPU for thread to run on.
    uint32_t cpu_affinity;

    // Starting chunk size of tasks to retrieve.
    uint32_t chunk_size;

    // Check for tapered schedule.
    bool tapered_schedule = false;
};

// Structure for complete thread control by the main thread.
template <typename in1, typename in2, typename out>
struct thread_control_struct {

    // Thread state variable.
    Thread_Control state = Execute;

    // Sleep condition variable.
    std::condition_variable cv;

    // Sleep mutex.
    std::mutex m;

    // Thread data for updates.
    struct work_data data;
};

// Structure to contain a set of tasks.
template <typename in1, typename in2, typename out>
struct task_set {

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
    // Variable to control threads and thread data.
    std::deque<thread_control_struct<in1, in2, out>> thread_control;



    // Constructor using just a workload and an output deque.
    BagOfTasks(struct workload<in1, in2, out>& work, std::deque<out>& output) :

        in1Begin(work.input1.begin()),
        in1End(work.input1.end()),
        input2(&work.input2),
        userFunction(work.userFunction),
        outBegin(output.begin()), 
        task_iterators(1, std::pair<typename std::deque<in1>::iterator, typename std::deque<in1>::iterator>(work.input1.begin(), work.input1.end())),
        output_iterators(1, output.begin())
    {
        thread_control.resize(work.params.number_of_threads);

        num_tasks_in_bag = in1End - in1Begin;
        num_tasks_uncompleted = num_tasks_in_bag;
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
        thread_control.resize(num_threads);

        num_tasks_in_bag = in1End - in1Begin;
        num_tasks_uncompleted = num_tasks_in_bag;
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
                    << "Number of tasks in bag: " << read_num_tasks_in_bag() << std::endl 
                    << "Number of tasks uncompleted: " << read_num_tasks_uncompleted() << std::endl << std::endl;
    };



    // Returns tasks of the specified count or less.
    std::deque<task_set<in1, in2, out>> get_tasks(uint32_t num) {

        // Get mutex.
        std::lock_guard<std::mutex> lock(m);

        std::deque<task_set<in1, in2, out>> output;

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
            struct task_set<in1, in2, out> temp = {
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
    void return_tasks(std::deque<task_set<in1, in2, out>> returned_tasks) {

        // Get mutex.
        std::lock_guard<std::mutex> lock(m);

        print("\nReturning ", returned_tasks.size(), " set(s) of tasks, ");

        // Add tasks to the front of the bag.
        while (returned_tasks.size() != 0) {

            // Add iterator pair to bag.
            task_iterators.push_front(std::pair<typename std::deque<in1>::iterator, typename std::deque<in1>::iterator>(returned_tasks.back().in1Begin, returned_tasks.back().in1End));

            // Add corresponding output iterator.
            output_iterators.push_front(returned_tasks.back().outBegin);

            // Update task total.
            num_tasks_in_bag += returned_tasks.back().in1End - returned_tasks.back().in1Begin;

            returned_tasks.pop_back();
        }

        print("bag contains ", task_iterators.size(), " set(s) with ", num_tasks_in_bag, " total tasks\n");
    };



    // Sets the last n threads to sleep.
    void sleep_n_threads(uint32_t n) {

        uint32_t count = n;

        for (uint32_t i = thread_control.size() - 1; i >= 0; i--) {

            if (count == 0) {
                break;
            }

            if (thread_control.at(i).state == Execute) {

                thread_control.at(i).state = Sleep;

                count--;
            }
        }

        if (count > 0) {
            print("\nCouldn't sleep ", n, " threads!\n\n");
            exit(1);
        }
    }



    // Wakes the next n threads (not including running threads.)
    void wake_n_threads(uint32_t n) {

        uint32_t count = n;

        for (uint32_t i = 0; i < thread_control.size(); i++) {

            if (count == 0) {
                break;
            }

            if (thread_control.at(i).state == Sleep) {

                // thread_control.at(i).state = Update;
                std::unique_lock<std::mutex> lk(thread_control.at(i).m);
                thread_control.at(i).cv.notify_all();

                count--;
            }
        }

        if (count > 0) {
            print("\nCouldn't wake ", n, " threads!\n\n");
            exit(1);
        }
    }



    // Returns a pair with the active/inactive thread counts.
    std::pair<uint32_t, uint32_t> num_active_and_inactive_threads() {

        std::pair<uint32_t, uint32_t> output(0, 0);

        for (uint32_t i = 0; i < thread_control.size(); i++) {

            if (thread_control.at(i).state == Execute) {
                output.first++;

            } else if (thread_control.at(i).state == Sleep) {
                output.second++;
            }
        }

        return output;
    }



    uint32_t read_num_tasks_in_bag() { 

        // Get mutex. Necessary?
        std::lock_guard<std::mutex> lock(m);

        return num_tasks_in_bag; 
    };



    uint32_t read_num_tasks_uncompleted() {

        // Get mutex.
        std::lock_guard<std::mutex> lock(num_tasks_uncompleted_m);

        return num_tasks_uncompleted;
    };



    void update_num_tasks_uncompleted(uint32_t subtract) {

        // Get mutex.
        std::lock_guard<std::mutex> lock(num_tasks_uncompleted_m);

        if (num_tasks_uncompleted >= subtract) {
            num_tasks_uncompleted = num_tasks_uncompleted - subtract;

        } else {

            print("ERROR - Told to subtract too much from num_tasks_uncompleted: \nSubtract value: ", subtract, "num_tasks_uncompleted", num_tasks_uncompleted);
            exit(1);
        }
    }



private:
    std::mutex m;
    std::mutex num_tasks_uncompleted_m;

    typename std::deque<in1>::iterator in1Begin;
    typename std::deque<in1>::iterator in1End;

    std::deque<in2>* input2;

    out (*userFunction) (in1, std::deque<in2>);

    typename std::deque<out>::iterator outBegin;

    std::deque<std::pair<typename std::deque<in1>::iterator, typename std::deque<in1>::iterator>> task_iterators;
    std::deque<typename std::deque<in1>::iterator> output_iterators;

    uint32_t num_tasks_in_bag;
    uint32_t num_tasks_uncompleted;
};




#define NUM_STATUSES 3

enum Status {Alive, Sleeping, Terminated};

const std::string statuses[NUM_STATUSES] = {"Alive", "Sleeping", "Terminated"};

const std::string bools[2] = {"False", "True"};



// Data struct to pass to each thread.
template <typename in1, typename in2, typename out>
struct thread_init_data {

    // Id of this thread.
    uint32_t threadId;

    // Pointer to the shared bag of tasks object.
    BagOfTasks<in1, in2, out> *bot;
};



/*
 * Functions
 */



// Returns a deque of appropriate chunk sizes.
std::deque<uint32_t> calc_chunks(experiment_parameters params, uint32_t num_tasks) {

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
            // uint32_t quotient = num_tasks / (params.number_of_threads * 2);

            // Set chunk sizes.
            for (uint32_t i = 0; i < params.number_of_threads; i++) {
                // output.at(i) = quotient;
                output.at(i) = params.initial_chunk_size;
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



// Returns a deque of complete work data for every thread.
template <typename in1, typename in2, typename out>
std::deque<work_data> calc_work_data(BagOfTasks<in1, in2, out> &bot, experiment_parameters params) {

    // Calculate info for data partitioning.
    std::deque<uint32_t> chunks = calc_chunks(params, bot.read_num_tasks_uncompleted());

    // Output deque of thread data
    std::deque<work_data> output;

    // Set thread data values.
    for (uint32_t i = 0; i < params.number_of_threads; i++) {

        struct work_data iter_data;

        iter_data.chunk_size   = chunks.at(i);
        iter_data.cpu_affinity = params.thread_pinnings.at(i);

        if (params.initial_schedule == Tapered) {
            iter_data.tapered_schedule = true;
        }

        output.push_back(iter_data);
    }

    return output;
}



// Calculates the total number of tasks in a group (deque) of task sets.
template <typename in1, typename in2, typename out>
uint32_t calc_num_tasks(std::deque<task_set<in1, in2, out>> tasks) {

    uint32_t total = 0;

    // For each set of tasks.
    for (uint32_t i = 0; i < tasks.size(); i++) {
        total += (tasks.at(i).in1End - tasks.at(i).in1Begin);
    }

    return total;
}



// Function to start each thread of mapArray on.
template <typename in1, typename in2, typename out>
void mapArrayThread(thread_init_data<in1, in2, out> my_data) {

    // Initialise metrics.
    metrics_thread_start(my_data.threadId);

    print("\n\n\n\n\n\nHello from thread ", my_data.threadId, "!\n\n\n\n");

    work_data work_data = {my_data.bot->thread_control.at(my_data.threadId).data.cpu_affinity,
                                          my_data.bot->thread_control.at(my_data.threadId).data.chunk_size,
                                          my_data.bot->thread_control.at(my_data.threadId).data.tapered_schedule};

    restart:

    // Stick to our designated CPU.
    stick_this_thread_to_cpu(work_data.cpu_affinity);

    // Get tasks.
    std::deque<task_set<in1, in2, out>> my_tasks = my_data.bot->get_tasks(work_data.chunk_size);

    print("\n[Thread ", my_data.threadId, "] Received ", my_tasks.size(), " sets of tasks with chunk size(s):\n");

    for (uint32_t i = 0; i < my_tasks.size(); i++) {
        print(my_tasks.at(i).in1End - my_tasks.at(i).in1Begin, "\n");
    }

    uint32_t total_num_tasks = calc_num_tasks(my_tasks);

    // While we have work to do:
    while (total_num_tasks != 0) {

        // Run through each set of tasks.
        for (uint32_t i = 0; i < my_tasks.size(); i++) {

            // Run each task in set, stepping through input1 and output vectors.
            for (; my_tasks.at(i).in1Begin != my_tasks.at(i).in1End; ++my_tasks.at(i).in1Begin, ++my_tasks.at(i).outBegin) {

                DM(metrics_starting_work(my_data.threadId));

                // Run user function.
                *(my_tasks.at(i).outBegin) = my_tasks.at(i).userFunction(*(my_tasks.at(i).in1Begin), *(my_tasks.at(i).input2));

                DM(metrics_finishing_work(my_data.threadId));

                // Check if we should still be executing.
                if (my_data.bot->thread_control.at(my_data.threadId).state != Execute) {

                    // Update completed tasks.
                    my_data.bot->update_num_tasks_uncompleted(total_num_tasks - calc_num_tasks(my_tasks));

                    // Return our tasks.
                    my_data.bot->return_tasks(my_tasks);
                    my_tasks.clear();

                    check:

                    // Perform relevant action.
                    switch (my_data.bot->thread_control.at(my_data.threadId).state) {

                        // Update our parameters.
                        case Update: {

                            work_data.cpu_affinity     = my_data.bot->thread_control.at(my_data.threadId).data.cpu_affinity;
                            work_data.chunk_size       = my_data.bot->thread_control.at(my_data.threadId).data.chunk_size;
                            work_data.tapered_schedule = my_data.bot->thread_control.at(my_data.threadId).data.tapered_schedule;

                            my_data.bot->thread_control.at(my_data.threadId).state = Execute;

                            goto restart;

                            break;
                        } 

                        // Go to sleep.
                        case Sleep: {

                            // Wait until we are woken.
                            std::unique_lock<std::mutex> lk(my_data.bot->thread_control.at(my_data.threadId).m);
                            my_data.bot->thread_control.at(my_data.threadId).cv.wait(lk);

                            print("\n[Thread ", my_data.threadId, "] Woken up!\n");

                            // my_data.bot->thread_control.at(my_data.threadId).state = Execute;

                            goto check;

                            break;
                        } 

                        // Terminate our computation.
                        case Terminate: {

                            goto end;

                            break;
                        }
                    }
                }
            }
        }

        // Update completed tasks.
        my_data.bot->update_num_tasks_uncompleted(total_num_tasks);

        // Check for tapered schedule.
        if (work_data.tapered_schedule && work_data.chunk_size > 1) {
            work_data.chunk_size = work_data.chunk_size / 2;
        }

        my_tasks.clear();

        my_tasks = my_data.bot->get_tasks(work_data.chunk_size);

        total_num_tasks = calc_num_tasks(my_tasks);

        print("\n[Thread ", my_data.threadId, "] Received ", my_tasks.size(), " sets of tasks with chunk size(s):\n");

        for (uint32_t j = 0; j < my_tasks.size(); j++) {
            print(my_tasks.at(j).in1End - my_tasks.at(j).in1Begin, "\n");
        }
    }

    end:

    metrics_thread_finished(my_data.threadId);

    return;
}

#endif // MAP_ARRAY_THREAD_HPP
#ifndef MAP_ARRAY_HPP
#define MAP_ARRAY_HPP

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
#include <comms.hpp>

#include <map_array_thread.hpp>

#include <workloads.hpp>



/*
 * This file contains the definition of the map array pattern. Note - all the definitions are in the header file, as
 * you cannot separate the definition of a template class from its declaration and put it inside a .cpp file.
 */

/*
 *  Implementation of the mapArray parallel programming pattern. Currently uses all available cores and splits tasks
 *  evenly. If the output deque is not big enough, it will be resized.
 *
 *  deque<in1>& input1                              - First input deque to be iterated over.
 *  deque<in2>& input2                              - Second input deque to be passed to user function.
 *  out          (*user_function) (in1, deque<in2>) - User function pointer to a function which takes .
 *                                                     (in1, deque<in2>) and returns an out type.
 *  deque<out>& output                              - deque to store output in.
 */

template <typename in1, typename in2, typename out>
void map_array(struct workload<in1, in2, out>& work, std::deque<out>& output) {

    // Print the number of processors we will use.
    print("[Main] Using ", work.params.number_of_threads, " processors\n");

    // Create bag of tasks.
    BagOfTasks<in1, in2, out> bot(work, output);

    // Calculate info for data partitioning.
    std::deque<thread_data<in1, in2, out>> thread_data_deque = calc_thread_data(bot, work.params);

    // Variables for creating and managing threads.
    std::deque<pthread_t> threads(work.params.number_of_threads);

    // Create all our needed threads.
    for (uint32_t i = 0; i < work.params.number_of_threads; i++) {

        print("[Main] Creating thread ", i , "\n");

        int rc = pthread_create(&threads.at(i), NULL, mapArrayThread<in1, in2, out>, (void *) &thread_data_deque.at(i));

        // Create thread name.
        char thread_name[16];
        sprintf(thread_name, "MA Thread %u", i);

        // Set thread name to something recognizable.
        pthread_setname_np(threads.at(i), thread_name);

        if (rc) {
            // If we couldn't create a new thread, throw an error and exit.
            print("[Main] ERROR; return code from pthread_create() is ", rc, "\n");
            exit(-1);
        }
    }

    // Get our PID to send to the controller.
    uint32_t pid = pthread_self();

    //  Prepare our context and socket
    zmq::context_t context(1);
    zmq::socket_t  port_socket(context, ZMQ_REQ);

    port_socket.connect("tcp://localhost:5555");

    print("\n[Main] Attempting registration with controller...\n\n");

    struct message rgstr;

    rgstr.header = APP_P_REQ;
    rgstr.pid    = pid;

    m_send(port_socket, rgstr);

    uint32_t port = uint32_t_recv(port_socket);

    port_socket.close();

    print("\n[Main] Received new port from controller: " + std::to_string(port) + "\n\n");

    zmq::socket_t socket(context, ZMQ_PAIR);

    socket.connect("tcp://localhost:" + std::to_string(port));

    rgstr.header = APP_INIT;

    rgstr.parameters.number_of_threads = work.params.number_of_threads;
    rgstr.parameters.schedule          = work.params.initial_schedule;
    rgstr.parameters.chunk_size        = work.params.initial_chunk_size;
    rgstr.parameters.array_size        = work.params.array_size;

    m_send(socket, rgstr);

    for (uint32_t i = 0; i < work.params.number_of_threads; i++) {
        uint32_t_send(socket, work.params.thread_pinnings.at(i));
    }

    while (bot.empty == false) {
        usleep(5);

        // Poll socket for a reply, with timeout
        zmq::pollitem_t items[] = { { socket, 0, ZMQ_POLLIN, 0 } };
        zmq::poll(&items[0], 1, 1000);

        // If we got a reply, process it
        if (items[0].revents & ZMQ_POLLIN) {

            struct message msg = m_recv(socket);

            std::deque<uint32_t> thread_pinnings;
            std::stringstream thread_pinnings_stringstream;

            for (uint32_t i = 0; i < msg.parameters.number_of_threads; i++) {

                uint32_t item = uint32_t_recv(socket);

                thread_pinnings.push_back(item);
                thread_pinnings_stringstream << item << " ";
            }

            if (msg.header == CON_UPDT) {

                print("\n[Main] New parameters received!",
                      "\n[Main] Changing schedule to: ", schedules[msg.parameters.schedule],
                      "\n[Main] With thread pinnings: ", thread_pinnings_stringstream.str(),
                      "\n\n");

                //     // Calculate new number of threads.
                //     // uint32_t new_num_threads = MAX_NUM_THREADS - count(begin(msg.parameters.thread_pinnings), end(msg.parameters.thread_pinnings), -1);

                //     // If we have a surplus of threads;
                //     if (work.params.number_of_threads > msg.parameters.number_of_threads) {

                //         uint32_t iterator = bot.thread_control.size() - 1;

                //         // Set excess threads to terminate.
                //         while(iterator > msg.parameters.number_of_threads - 1) {

                //             bot.thread_control.at(iterator) = Terminate;
                //         }

                //         // Join with terminating threads.
                //         join_with_threads(threads, work.params.number_of_threads - msg.parameters.number_of_threads);

                //         // Cleanup thread control vars.
                //         bot.thread_control.resize(msg.parameters.number_of_threads);
                //     }

                //     if (work.params.number_of_threads < msg.parameters.number_of_threads) {

                //     }

                // Terminating threads.
                bot.thread_control.assign(work.params.number_of_threads, Terminate);

                // Joining with threads.
                join_with_threads(threads, work.params.number_of_threads);

                // Update parameters.
                work.params.number_of_threads  = msg.parameters.number_of_threads;
                work.params.initial_schedule   = msg.parameters.schedule;
                work.params.initial_chunk_size = msg.parameters.chunk_size;

                // Clear previous thread pinnings.
                // work.params.thread_pinnings.clear();

                // Copy new thread pinnings.
                work.params.thread_pinnings = thread_pinnings;

        
        
                // Restart map_array:

                // Reset terminate variables.
                bot.thread_control.assign(work.params.number_of_threads, Execute);

                thread_data_deque.clear();

                // Recalculate info for data partitioning.
                std::deque<thread_data<in1, in2, out>> thread_data_deque2 = calc_thread_data(bot, work.params);

                // Reset thread ids.
                threads.clear();
                threads.resize(work.params.number_of_threads);

                // Create all our needed threads.
                for (uint32_t i = 0; i < work.params.number_of_threads; i++) {
                    print("[Main] Creating thread ", i , "\n");

                    int rc = pthread_create(&threads.at(i), NULL, mapArrayThread<in1, in2, out>, (void *) &thread_data_deque2.at(i));

                    if (rc) {
                        // If we couldn't create a new thread, throw an error and exit.
                        print("[Main] ERROR; return code from pthread_create() is ", rc, "\n");
                        exit(-1);
                    }
                }
            } else {
                print("MALFORMED MESSAGE!");
                exit(EXIT_FAILURE);
            }
        }
    }

    struct message term;
    term.header = APP_TERM;
    term.pid = pid;

    m_send(socket, term);

    socket.close();

    join_with_threads(threads, work.params.number_of_threads);

    return;
}

#endif // MAP_ARRAY_HPP
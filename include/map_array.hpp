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



#include <thread>





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
    std::deque<work_data> work_data_deque = calc_work_data<in1, in2, out>(bot, work.params);

    for (uint32_t m = 0; m < work_data_deque.size(); m++) {
        bot.thread_control.at(m).data.cpu_affinity = work_data_deque.at(m).cpu_affinity;
        bot.thread_control.at(m).data.chunk_size = work_data_deque.at(m).chunk_size;
        bot.thread_control.at(m).data.tapered_schedule = work_data_deque.at(m).tapered_schedule;
    }

    // Variables for creating and managing threads.
    std::deque<std::thread> threads(work.params.number_of_threads);

    // Create all our needed threads.
    for (uint32_t i = 0; i < work.params.number_of_threads; i++) {

        print("[Main] Creating thread ", i , "\n");

        // Thread number and bag pointer for the new thread.
        thread_init_data<in1, in2, out> init_data = {i, &bot};

        threads.at(i) = std::thread(mapArrayThread<in1, in2, out>, init_data);
    }

    // Get our PID to send to the controller.
    uint32_t pid = pthread_self();

    //  Prepare our context and socket
    zmq::context_t context(1);
    zmq::socket_t  port_socket(context, ZMQ_REQ);

    port_socket.connect("tcp://localhost:5555");

    print("\n[Main] Attempting registration with controller...\n");

    struct message rgstr;

    rgstr.header = APP_P_REQ;
    rgstr.pid    = pid;

    m_send(port_socket, rgstr);

    uint32_t port = uint32_t_recv(port_socket);

    port_socket.close();

    print("\n[Main] Received new port from controller: " + std::to_string(port) + "\n");

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

    while (bot.read_num_tasks_uncompleted() != 0) {
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
                      "\n[Main] With a chunk size of: ", msg.parameters.chunk_size,
                      "\n[Main] With thread pinnings: ", thread_pinnings_stringstream.str(),
                      "\n");

                std::pair<uint32_t, uint32_t> thread_state_counts = bot.num_active_and_inactive_threads();

                uint32_t current_num_threads = thread_state_counts.first + thread_state_counts.second;

                // Calculate difference in thread counts.
                int thread_num_diff = msg.parameters.number_of_threads - current_num_threads;

                // If we have a excess of threads;
                if (thread_num_diff < 0) {

                    // Sleep excess threads.
                    bot.sleep_n_threads(abs(thread_num_diff));
                } 

                // If we have a shortage of threads.
                if (thread_num_diff > 0) {

                    // Expand thread control data for our new threads.
                    bot.thread_control.resize(msg.parameters.number_of_threads);
                }

                // Update parameters.
                work.params.number_of_threads  = msg.parameters.number_of_threads;
                work.params.initial_schedule   = msg.parameters.schedule;
                work.params.initial_chunk_size = msg.parameters.chunk_size;

                work.params.thread_pinnings.clear();

                for (uint32_t i = 0; i < thread_pinnings.size(); i++) {
                    work.params.thread_pinnings.push_back(thread_pinnings.at(i));
                }

                // Recalculate info for data partitioning.
                std::deque<work_data> thread_data_deque2 = calc_work_data<in1, in2, out>(bot, work.params);

                // Copy updated data.
                for (uint32_t m = 0; m < work.params.number_of_threads; m++) {

                    bot.thread_control.at(m).data.cpu_affinity     = thread_data_deque2.at(m).cpu_affinity;
                    bot.thread_control.at(m).data.chunk_size       = thread_data_deque2.at(m).chunk_size;
                    bot.thread_control.at(m).data.tapered_schedule = thread_data_deque2.at(m).tapered_schedule;

                    // std::lock_guard<std::mutex> lk(bot.thread_control.at(m).m);

                    // bot.thread_control.at(m).cv.notify_all();

                    bot.thread_control.at(m).state = Update;
                }

                // if (thread_state_counts.second != 0) {
                    
                // }

                // If we need more threads;
                if (thread_num_diff > 0) {

                    // Expand metrics for the new threads.
                    metrics_expand_threads(thread_num_diff);

                    // Expand threads.
                    threads.resize(work.params.number_of_threads);

                    // Create all our needed threads.
                    for (uint32_t i = current_num_threads; i < work.params.number_of_threads; i++) {

                        print("[Main] Creating thread ", i , "\n");

                        // Thread number and bag pointer for the new thread.
                        thread_init_data<in1, in2, out> init_data = {i, &bot};

                        threads.at(i) = std::thread(mapArrayThread<in1, in2, out>, init_data);
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

    // Get thread state counts.
    std::pair<uint32_t, uint32_t> thread_state_counts = bot.num_active_and_inactive_threads();

    // Wake sleeping threads so they can return.
    bot.wake_n_threads(thread_state_counts.second);

    for (uint32_t i = 0; i < threads.size(); i++) {
        threads.at(i).join();
    }

    return;
}

#endif // MAP_ARRAY_HPP
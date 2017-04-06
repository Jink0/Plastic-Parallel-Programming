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

using namespace std;

#ifdef METRICS
  #define Ms( x ) x
#else
  #define Ms( x ) 
#endif

#ifdef METRICS
  #include <metrics.hpp> // Our metrics library
#endif

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
void map_array(deque<in1>& input1, deque<in2>& input2, out (*user_function) (in1, deque<in2>), deque<out>& output, 
               string output_filename = "", parameters params = parameters())
{
  Ms(print("[Main] Metrics on!\n\n"));

  // Initialise metrics.
  Ms(metrics_init(4, output_filename + ".csv"));

  // Print the number of processors we can detect.
  print("[Main] Found ", params.thread_pinnings.size(), " processors\n");

  BagOfTasks<in1, in2, out> bot(input1.begin(), input1.end(), &input2, user_function, output.begin());

  bot.thread_control.assign(params.thread_pinnings.size(), Execute);

  // Calculate info for data partitioning.
  deque<thread_data<in1, in2, out>> thread_data_deque = calc_thread_data(bot.numTasksRemaining(), bot, params);

  // Variables for creating and managing threads.
  deque<pthread_t> threads(params.thread_pinnings.size());

  // Create all our needed threads.
  for (uint32_t i = 0; i < params.thread_pinnings.size(); i++)
  {
    print("[Main] Creating thread ", i , "\n");

    int rc = pthread_create(&threads.at(i), NULL, mapArrayThread<in1, in2, out>, (void *) &thread_data_deque.at(i));

    // Create thread name.
    char thread_name[16];
    sprintf(thread_name, "MA Thread %u", i);

    // Set thread name to something recognizable.
    pthread_setname_np(threads.at(i), thread_name);

    if (rc)
    {
      // If we couldn't create a new thread, throw an error and exit.
      print("[Main] ERROR; return code from pthread_create() is ", rc, "\n");
      exit(-1);
    }
  }

  // Get our PID to send to the controller.
  uint32_t pid = pthread_self();

  using namespace zmq;

  //  Prepare our context and socket
  context_t context(1);
  socket_t  socket(context, ZMQ_PAIR);

  socket.connect("tcp://localhost:5555");

  print("\n[Main] Registering with controller...\n\n");
        
  struct message rgstr;

  rgstr.header                   = APP_REG;
  rgstr.pid                      = pid;
  rgstr.settings.schedule        = params.schedule;

  fill_n(rgstr.settings.thread_pinnings, MAX_NUM_THREADS, -1);

  copy(params.thread_pinnings.begin(), params.thread_pinnings.end(), rgstr.settings.thread_pinnings);

  m_send(socket, rgstr);

  while (bot.empty == false)
  {
    usleep(5);

    // Get message from controller.
    struct message msg = m_no_block_recv(socket);

    if (msg.header != -1)
    {
      print("\n[Main] Received new parameters from controller!\n\n");

      // Calculate new number of threads.
      /*uint32_t new_num_threads = MAX_NUM_THREADS - count(begin(msg.settings.thread_pinnings), end(msg.settings.thread_pinnings), -1);

      // If we have a surplus of threads;
      if (params.thread_pinnings.size() > new_num_threads)
      {
        uint32_t iterator = bot.thread_control.size() - 1;

        // Set excess threads to terminate.
        while(iterator > new_num_threads - 1)
        {
          bot.thread_control.at(iterator) = Terminate;
        }

        // Join with terminating threads.
        join_with_threads(threads, params.thread_pinnings.size() - new_num_threads);

        // Cleanup thread control vars.
        bot.thread_control.resize(new_num_threads);
      }
        
      if (params.thread_pinnings.size() < new_num_threads)
      {

      }*/

      // Terminating threads.
      bot.thread_control.assign(params.thread_pinnings.size(), Terminate);

      // Joining with threads.
      join_with_threads(threads, params.thread_pinnings.size());

      // Update parameters.
      params.schedule = msg.settings.schedule;

      // Clear previous thread pinnings.
      params.thread_pinnings.clear();

      uint32_t i_w = 0;
      stringstream thread_pinnings_stringstream;

      while (msg.settings.thread_pinnings[i_w] != -1) 
      {
        // Record and update thread pinnings.
        thread_pinnings_stringstream << msg.settings.thread_pinnings[i_w] << " ";
        params.thread_pinnings.push_back(msg.settings.thread_pinnings[i_w]);
        i_w++;
      }

      print("\n[Main] New schedule received!",
            "\n[Main] Changing schedule to: ", Schedules[msg.settings.schedule], 
            "\n[Main] With thread pinnings: ", thread_pinnings_stringstream.str(),
            "\n\n");

      // Restart map_array:

      // Reset terminate variables.
      bot.thread_control.assign(params.thread_pinnings.size(), Execute);
        
      // Recalculate info for data partitioning.
      thread_data_deque = calc_thread_data(bot.numTasksRemaining(), bot, params);

      // Reset thread ids.
      threads.clear();
      threads.resize(params.thread_pinnings.size());

      // Create all our needed threads.
      for (uint32_t i = 0; i < params.thread_pinnings.size(); i++)
      {
        print("[Main] Creating thread ", i , "\n");

        int rc = pthread_create(&threads.at(i), NULL, mapArrayThread<in1, in2, out>, (void *) &thread_data_deque.at(i));

        if (rc)
        {
          // If we couldn't create a new thread, throw an error and exit.
          print("[Main] ERROR; return code from pthread_create() is ", rc, "\n");
          exit(-1);
        }
      }
    }
  }

  struct message term;
  term.header = APP_TERM;
  term.pid = pid;

  m_send(socket, term);

  socket.close();

  join_with_threads(threads, params.thread_pinnings.size());

  Ms(metrics_finalise());

  Ms(metrics_calc());

  Ms(metrics_exit());

  return;
}

#endif // MAP_ARRAY_HPP
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
#include <comms.hpp>

using namespace std;

#ifdef METRICS
  #define Ms( x ) x
#else
  #define Ms( x ) 
#endif

#ifdef METRICS
  #include <metrics.hpp> // Our metrics library
#endif

#include <map_array_utils.cpp>

/*
 * This file contains the definition of the map array pattern. Note - all the definitions are in the header file, as 
 * you cannot separate the definition of a template class from its declaration and put it inside a .cpp file.
 */

// Function to start each thread of mapArray on.
template <typename in1, typename in2, typename out>
void *mapArrayThread(void *threadarg)
{
  // Pointer to store personal data
  struct thread_data<in1, in2, out> *my_data;
  my_data = (struct thread_data<in1, in2, out> *) threadarg;

  stick_this_thread_to_cpu(my_data->cpu_affinity);

  // Initialise metrics
  Ms(metrics_thread_start(my_data->threadId));

  // Print starting parameters
  print("[Thread ", my_data->threadId, "] Hello! \n");

  // Get tasks
  tasks<in1, in2, out> my_tasks = (*my_data->bot).getTasks(my_data->chunk_size);

  uint32_t tapered_chunk_size = my_data->chunk_size / 2;

  // While we have tasks to do;
  while (my_tasks.in1End - my_tasks.in1Begin > 0 )
  {
    // Run between iterator ranges, stepping through input1 and output vectors
    for (; my_tasks.in1Begin != my_tasks.in1End; ++my_tasks.in1Begin, ++my_tasks.outBegin)
    {
      Ms(metrics_starting_work(my_data->threadId));
    
      // Run user function
      *(my_tasks.outBegin) = my_tasks.userFunction(*(my_tasks.in1Begin), *(my_tasks.input2));

      Ms(metrics_finishing_work(my_data->threadId));
    }

    // If we should still be executing, get more tasks!
    if ((*my_data->bot).thread_control.at(my_data->threadId) == Execute)
    {
      if (my_data->tapered_schedule)
      {
        my_tasks = (*my_data->bot).getTasks(tapered_chunk_size);

        print("[Thread ", my_data->threadId, "] Chunk size: ", tapered_chunk_size, "\n");

        if (tapered_chunk_size > 1)
        {
          tapered_chunk_size = tapered_chunk_size / 2;
        }
      }
      else
      {
        my_tasks = (*my_data->bot).getTasks(my_data->chunk_size);

        print("[Thread ", my_data->threadId, "] Chunk size: ", my_data->chunk_size, "\n");
      }
    }

  }

  Ms(metrics_thread_finished(my_data->threadId));

  pthread_exit(NULL);
}

#endif // MAP_ARRAY_THREAD_HPP
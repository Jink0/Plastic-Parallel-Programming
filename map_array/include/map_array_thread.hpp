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

/*
 * This file contains the definition of the map array pattern. Note - all the definitions are in the header file, as 
 * you cannot separate the definition of a template class from its declaration and put it inside a .cpp file.
 */



// Parameters with default values.
struct parameters 
{
    parameters(): task_dist(1), schedule(Tapered) 
    { 
      // Retrieve the number of CPUs using the boost library.
      uint32_t num_threads = boost::thread::hardware_concurrency();

      for (uint32_t i = 0; i < num_threads; i++)
      {
        thread_pinnings.push_back(i);
      }
    }

    // How many threads to pin where.
    deque<int> thread_pinnings;

    // Distribution of the tasks.
    int task_dist;

    // Schedule to use.
    Schedule schedule;
};



deque<uint32_t> calc_schedules(uint32_t num_tasks, uint32_t num_threads, Schedule sched, uint32_t chunk_size = 0)
{
  deque<uint32_t> output(num_threads);

  switch (sched)
  {
    case Static:
      {
        // Calculate info for data partitioning.
        uint32_t quotient  = num_tasks / num_threads;
        uint32_t remainder = num_tasks % num_threads;

        for (uint32_t i = 0; i < num_threads; i++)
        {
          // If we still have remainder tasks, add one to this thread.
          if (i < remainder) 
          { 
            output.at(i) = quotient + 1;
          }
          else
          {
            output.at(i) = quotient;
          }
        }
      }

      break;

    case Dynamic_chunks:
      {
        if (chunk_size == 0)
        {
          chunk_size = num_tasks / (num_threads * 10);
        }

        for (uint32_t i = 0; i < num_threads; i++)
        {
          output.at(i) = chunk_size;
        }
      }

      break;

    case Dynamic_individual:
      {
        for (uint32_t i = 0; i < num_threads; i++)
        {
          output.at(i) = 1;
        }
      }

      break;

    case Tapered:
      {
        // Calculate info for data partitioning.
        uint32_t quotient  = num_tasks / (num_threads * 2);

        for (uint32_t i = 0; i < num_threads; i++)
        {
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



// Thread control enum. threads either run (execute), change strategies (update), or stop (terminate).
enum Thread_Control {Execute, Update, Terminate};



// Structure to contain a group of tasks.
template <typename in1, typename in2, typename out>
struct tasks
{
  // Start of input.
  typename deque<in1>::iterator in1Begin;

  // End of input.
  typename deque<in1>::iterator in1End;

  // Pointer to shared input2 deque.
  deque<in2>* input2;

  // User function pointer.
  out (*userFunction) (in1, deque<in2>);

  // Start of output.
  typename deque<out>::iterator outBegin;
};



// Bag of tasks class. Also contains shard variables for communicating with worker threads.
template <class in1, class in2, class out>
class BagOfTasks {
  public:
    // Variables to control if threads terminate.
    deque<Thread_Control> thread_control;

    // Check for if bag is empty.
    bool empty = false;

    // Constructor
    BagOfTasks(typename deque<in1>::iterator in1B, 
               typename deque<in1>::iterator in1E, 
               deque<in2>* in2p, 
               out (*userF) (in1, deque<in2>), 
               typename deque<out>::iterator outB) :
              
               in1Begin(in1B),
               in1End(in1E),
               input2(in2p),
               userFunction(userF),
               outBegin(outB)
      {}

    // Destructor
    ~BagOfTasks() {};

    // Overloads << operator for easy printing with streams.
    friend ostream& operator<< (ostream &outS, BagOfTasks<in1, in2, out> &bot)
    {
      // Get mutex. When control leaves the scope in which the lock_guard object was created, the lock_guard is 
      // destroyed and the mutex is released. 
      lock_guard<mutex> lock(bot.m);

      // Print all our important data.
      return outS << "Bag of tasks: " << endl
                  << "Data types - <" << type_name<in1>() << ", " 
                                      << type_name<in2>() << ", " 
                                      << type_name<out>() << ">" << endl
                  << "Number of tasks - " << bot.in1End - bot.in1Begin << endl << endl;
    };

    uint32_t numTasksRemaining()
    {
      // Get mutex. 
      lock_guard<mutex> lock(m);

      // Return value.
      return in1End - in1Begin;
    }

    // Returns tasks of the specified number or less. 
    tasks<in1, in2, out> getTasks(uint32_t num)
    {
      // Get mutex. 
      lock_guard<mutex> lock(m);

      // Record where we should start in our task list.
      typename deque<in1>::iterator tasksBegin = in1Begin;

      uint32_t num_tasks;

      // Calculate number of tasks to return.
      if (num < in1End - in1Begin)
      {
        num_tasks = num;
      }
      else
      {
        num_tasks = in1End - in1Begin;
      }

      if (num_tasks == 0)
      {
        empty = true;
      }

      // Advance our iterator so it now marks the end of our tasks.
      advance(in1Begin, num_tasks);

      // Create tasks data structure to return.
      struct tasks<in1, in2, out> output = {
        tasksBegin, 
        in1Begin,
        input2,
        userFunction,
        outBegin
      };

      // Advance the output iterator to output in the correct place next time.
      advance(outBegin, num_tasks);

      return output;
    };

  private:
    mutex m;

    typename deque<in1>::iterator in1Begin;
    typename deque<in1>::iterator in1End;

    deque<in2>* input2;

    out (*userFunction) (in1, deque<in2>);

    typename deque<out>::iterator outBegin;
};



enum Status {Alive, Sleeping, Terminated};

// Data struct to pass to each thread.
template <typename in1, typename in2, typename out>
struct thread_data
{
  // Id of this thread.
  int threadId;

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



template <typename in1, typename in2, typename out>
deque<thread_data<in1, in2, out>> calc_thread_data(uint32_t input1_size, BagOfTasks<in1, in2, out> &bot, parameters params) 
{
  // Calculate info for data partitioning.
  deque<uint32_t> schedules = calc_schedules(input1_size, params.thread_pinnings.size(), params.schedule);

  // Output thread data
  deque<thread_data<in1, in2, out>> output;

  // Set thread data values.
  for (uint32_t i = 0; i < params.thread_pinnings.size(); i++)
  {
    struct thread_data<in1, in2, out> iter_data;

    iter_data.threadId     = i;
    iter_data.chunk_size   = schedules.at(i);
    iter_data.bot          = &bot;
    iter_data.cpu_affinity = params.thread_pinnings.at(i);

    if (params.schedule == Tapered)
    {
      iter_data.tapered_schedule = true;
    }

    output.push_front(iter_data);
  }

  return output;
}



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
#ifndef MAP_ARRAY_H
#define MAP_ARRAY_H

#include <iostream>         // cout
#include <vector>           // Vectors
#include <mutex>            // mutexes
#include <pthread.h>        // Thread and mutex functions
#include <unistd.h>         // sleep()
#include <cxxabi.h>         // Demangling typenames (gcc only)
#include <boost/thread.hpp> // boost::thread::hardware_concurrency();

#include <zmq.hpp>
#include <string>
#include <iostream>

#include <comms.h>

using namespace std;

#ifdef METRICS
  #define Ms( x ) x
#else
  #define Ms( x ) 
#endif

#ifdef METRICS
  #include <metrics.h> // Our metrics library
#endif

/*
 * This file contrains the definition of the map array pattern. Note - all the definitions are in the header file, as 
 * you cannot seperate the definition of a template class from its declaration and put it inside a .cpp file.
 */



/*
 * Mutexed print function:
 */

// With nothing to add to the output stream, just return the stream.
ostream&
print_rec(ostream& outS)
{
    return outS;
}

// Recursively print arguments.
template <class A0, class ...Args>
ostream&
print_rec(ostream& outS, const A0& a0, const Args& ...args)
{
    outS << a0;
    return print_rec(outS, args...);
}

// Case when given an output stream.
template <class ...Args>
ostream&
print(ostream& outS, const Args& ...args)
{
    return print_rec(outS, args...);
}

// Retrieve the mutex. Must be done using this non-templated function, so that we have one mutex across all 
// instantiations of the templated function.
mutex&
get_cout_mutex()
{
    static mutex m;
    return m;
}

// The main print function defaults to the cout output stream.
template <class ...Args>
ostream&
print(const Args& ...args)
{
    lock_guard<mutex> _(get_cout_mutex());
    return print(cout, args...);
}



// Parameters with default values.
struct parameters 
{
    parameters(): task_dist(1), schedule(Tapered) 
    { 
      // Retreive the number of CPUs using the boost library.
      uint32_t num_threads = boost::thread::hardware_concurrency();

      for (uint32_t i = 0; i < num_threads; i++)
      {
        thread_pinnings.push_back(i);
      }
    }

    // How many threads to pin where.
    vector<uint32_t> thread_pinnings;

    // Distribution of the tasks.
    int task_dist;

    // Schedule to use.
    Schedule schedule;
};



// Returns the typename given to the template. Example usage: typename<int>() returns "int".
template<typename T>
string type_name()
{
    // Variable to store status.
    int status;

    // Mangled name of type T.
    string mName = typeid(T).name();

    // Attempt to demangle 
    char *dmName = abi::__cxa_demangle(mName.c_str(), NULL, NULL, &status);

    // If successful,
    if(status == 0) 
    {
        // Record name.
        mName = dmName;

        // Free memory.
        free(dmName);
    }   

    return mName;
}



vector<uint32_t> calc_schedules(uint32_t num_tasks, uint32_t num_threads, Schedule sched, uint32_t chunk_size = 0)
{
  vector<uint32_t> output(num_threads);

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
            output[i] = quotient + 1;
          }
          else
          {
            output[i] = quotient;
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
          output[i] = chunk_size;
        }
      }

      break;

    case Dynamic_individual:
      {
        for (uint32_t i = 0; i < num_threads; i++)
        {
          output[i] = 1;
        }
      }

      break;

    case Tapered:
      {
        // Calculate info for data partitioning.
        uint32_t quotient  = num_tasks / (num_threads * 2);

        for (uint32_t i = 0; i < num_threads; i++)
        {
          output[i] = quotient;
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



// Structure to contain a group of tasks.
template <typename in1, typename in2, typename out>
struct tasks
{
  // Start of input.
  typename vector<in1>::iterator in1Begin;

  // End of input.
  typename vector<in1>::iterator in1End;

  // Pointer to shared input2 vector.
  vector<in2>* input2;

  // User function pointer.
  out (*userFunction) (in1, vector<in2>);

  // Start of output.
  typename vector<out>::iterator outBegin;
};



// Bag of tasks class.
template <class in1, class in2, class out>
class BagOfTasks {
  public:
    // Constructor
    BagOfTasks(typename vector<in1>::iterator in1B, 
               typename vector<in1>::iterator in1E, 
               vector<in2>* in2p, 
               out (*userF) (in1, vector<in2>), 
               typename vector<out>::iterator outB) :
              
               in1Begin(in1B),
               in1End(in1E),
               input2(in2p),
               userFunction(userF),
               outBegin(outB)
      {}

    // Destructor
    ~BagOfTasks() {};

    bool threadTerminateVar;

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

    // Returns tasks of the specified number or less. 
    tasks<in1, in2, out> getTasks(uint32_t num)
    {
      // Get mutex. 
      lock_guard<mutex> lock(m);

      // Record where we should start in our task list.
      typename vector<in1>::iterator tasksBegin = in1Begin;

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

    typename vector<in1>::iterator in1Begin;
    typename vector<in1>::iterator in1End;

    vector<in2>* input2;

    out (*userFunction) (in1, vector<in2>);

    typename vector<out>::iterator outBegin;
};



int stick_this_thread_to_cpu(uint32_t core_id) {
   uint32_t num_cores = sysconf(_SC_NPROCESSORS_ONLN);

   if (core_id >= num_cores)
      return EINVAL;

   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(core_id, &cpuset);

   pthread_t current_thread = pthread_self();    

   return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}



// Data struct to pass to each thread.
template <typename in1, typename in2, typename out>
struct thread_data
{
  // Id of this thread.
  int threadId;

  // CPU for thread to run on.
  uint32_t cpu_affinity;

  // Starting chunk size of tasks to retreive.
  uint32_t chunk_size;

  // Check for tapered schedule.
  bool tapered_schedule = false;

  // Pointer to the shared bag of tasks object.
  BagOfTasks<in1, in2, out> *bot;

  // Flag which main thread will set to indicate new instructions.
  bool check_for_new_instructions = false;
};



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

    // If we should still be running (no new schedule), get more tasks!
    if ((*my_data->bot).threadTerminateVar == false) //my_data->threadId
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



/*
 *  Implementation of the mapArray parallel programming pattern. Currently uses all available cores and splits tasks 
 *  evenly. If the output vector is not big enough, it will be resized.
 *
 *  vector<in1>& input1                              - First input vector to be iterated over.
 *  vector<in2>& input2                              - Second input vector to be passed to user function.
 *  out          (*user_function) (in1, vector<in2>) - User function pointer to a function which takes .
 *                                                     (in1, vector<in2>) and returns an out type.
 *  vector<out>& output                              - Vector to store output in.
 */

template <typename in1, typename in2, typename out>
void map_array(vector<in1>& input1, vector<in2>& input2, out (*user_function) (in1, vector<in2>), vector<out>& output, 
               string output_filename = "", parameters params = parameters())
{
  Ms(print("Metrics on!\n\n"));

  // Initialise metrics.
  Ms(metrics_init(params.thread_pinnings.size(), output_filename + ".csv"));

  // Print the number of processors we can detect.
  print("[Main] Found ", params.thread_pinnings.size(), " processors\n");

  BagOfTasks<in1, in2, out> bot(input1.begin(), input1.end(), &input2, user_function, output.begin());

  bot.threadTerminateVar = false;

  struct thread_data<in1, in2, out> thread_data_array[params.thread_pinnings.size()];

  // Calculate info for data partitioning.
  vector<uint32_t> schedules = calc_schedules(input1.size(), params.thread_pinnings.size(), params.schedule);

  // Set thread data values.
  for (uint32_t i = 0; i < params.thread_pinnings.size(); i++)
  {
    thread_data_array[i].threadId     = i;
    thread_data_array[i].chunk_size   = schedules[i];
    thread_data_array[i].bot          = &bot;
    thread_data_array[i].cpu_affinity = params.thread_pinnings[i];

    if (params.schedule == Tapered)
    {
      thread_data_array[i].tapered_schedule = true;
    }
  }

  // Variables for creating and managing threads.
  pthread_t threads[params.thread_pinnings.size()];
  pthread_attr_t attr;
  int rc;

  // Initialize and set thread detached attribute to joinable.
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  // Create all our needed threads.
  for (uint32_t i = 0; i < params.thread_pinnings.size(); i++)
  {
    print("[Main] Creating thread ", i , "\n");

    rc = pthread_create(&threads[i], &attr, mapArrayThread<in1, in2, out>, (void *) &thread_data_array[i]);

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
  socket_t socket(context, ZMQ_REQ);

  print("Requesting socket from controller...\n");
  socket.connect("tcp://localhost:5555");

  struct message syn;

  syn.header            = APP_SYN;
  syn.pid               = pid;
  syn.settings.schedule = params.schedule;

  message_t msg (sizeof(syn));
  memcpy(msg.data(), &syn, sizeof(syn));

  print("Sending SYN with PID ", pid, "...\n");
  socket.send(msg);

  // Get the reply.
  message_t reply;
  socket.recv(&reply);

  struct message ack = *(static_cast<struct message*>(reply.data()));

  print("Received ACK from controller!\n");

  if (ack.settings.schedule != params.schedule) {
    print("New schedule received! Changing to ", Schedules[ack.settings.schedule], "\n");

    bot.threadTerminateVar = true;

    // Free attribute and join with the other threads.
    pthread_attr_destroy(&attr);

    for (uint32_t i = 0; i < params.thread_pinnings.size(); i++)
    {
      rc = pthread_join(threads[i], NULL);

      if (rc)
      {
        // If we couldn't create a new thread, throw an error and exit.
        print("[Main] ERROR; return code from pthread_join() is ", rc, "\n");
        exit(-1);
      }

      print("[Main] Joined with thread ", i, "\n");
    }

    // Update parameters
    params.schedule = ack.settings.schedule;

    // Restart map_array
    
    // Calculate info for data partitioning.
    schedules = calc_schedules(input1.size(), params.thread_pinnings.size(), params.schedule);

    // Set thread data values.
    for (uint32_t i = 0; i < params.thread_pinnings.size(); i++)
    {
      thread_data_array[i].threadId     = i;
      thread_data_array[i].chunk_size   = schedules[i];
      thread_data_array[i].bot          = &bot;
      thread_data_array[i].cpu_affinity = params.thread_pinnings[i];

      if (params.schedule == Tapered)
      {
        thread_data_array[i].tapered_schedule = true;
      }
    }

    // Variables for creating and managing threads.
    pthread_t threads[params.thread_pinnings.size()];
    pthread_attr_t attr;
    int rc;

    // Initialize and set thread detached attribute to joinable.
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // Create all our needed threads.
    for (uint32_t i = 0; i < params.thread_pinnings.size(); i++)
    {
      print("[Main] Creating thread ", i , "\n");

      rc = pthread_create(&threads[i], &attr, mapArrayThread<in1, in2, out>, (void *) &thread_data_array[i]);

      if (rc)
      {
        // If we couldn't create a new thread, throw an error and exit.
        print("[Main] ERROR; return code from pthread_create() is ", rc, "\n");
        exit(-1);
      }
    }
  }

  //print("SLEEPING\n");

  //sleep(20);

  




  socket.close();

  // Free attribute and join with the other threads.
  pthread_attr_destroy(&attr);

  for (uint32_t i = 0; i < params.thread_pinnings.size(); i++)
  {
    rc = pthread_join(threads[i], NULL);

    if (rc)
    {
      // If we couldn't create a new thread, throw an error and exit.
      print("[Main] ERROR; return code from pthread_join() is ", rc, "\n");
      exit(-1);
    }

    print("[Main] Joined with thread ", i, "\n");
  }

  Ms(metrics_finalise());

  Ms(metrics_calc());

  Ms(metrics_exit());
}

#endif /* MAP_ARRAY_H */
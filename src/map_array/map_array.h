#ifndef MAP_ARRAY_H
#define MAP_ARRAY_H

#include <iostream>  // cout
#include <vector>    // Vectors
#include <assert.h>  // Assert functions
#include <pthread.h> // Thread and mutex functions
#include <mutex>     // mutex
#include <unistd.h>  // For sleep()

#include <cxxabi.h>  // For demangling typenames (gcc only)

#include <boost/thread.hpp>

using namespace std;

#include <metrics.h>



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



/* Different possible schedules Static             - Give each thread equal portions.
                                Dynamic_chunks     - Threads dynamically retrive a chunk of the tasks when they can.
                                Dynamic_individual - Threads retrieve a single task when they can.
                                Tapered            - Chunk size starts off large and decreases to better handle load 
                                                     imbalance between iterations.
                                Auto               - Automatically try to figure out the best schedule. */
enum Schedule {Static, Dynamic_chunks, Dynamic_individual, Tapered, Auto};



// Parameters with default values.
struct parameters 
{
    parameters(): task_dist(1), schedule(Dynamic_chunks) 
    { 
      // Retreive the number of CPUs using the boost library.
      num_threads = boost::thread::hardware_concurrency();
    }

    // Number of threads to use.
    int num_threads;

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



// Data struct to pass to each thread.
template <typename in1, typename in2, typename out>
struct thread_data
{
  // Id of this thread.
  int        threadId;

  // Starting chunk size of tasks to retreive.
  uint32_t   chunkSize;

  // Pointer to the shared bag of tasks object.
  BagOfTasks<in1, in2, out> *bot;
};



// Function to start each thread of mapArray on.
template <typename in1, typename in2, typename out>
void *mapArrayThread(void *threadarg)
{
  // Pointer to store personal data
  struct thread_data<in1, in2, out> *my_data;
  my_data = (struct thread_data<in1, in2, out> *) threadarg;

  // Initialise metrics
  metrics_thread_start(my_data->threadId);

  // Print starting parameters
  print("[Thread ", my_data->threadId, "] Hello! \n");

  // Get tasks
  tasks<in1, in2, out> my_tasks = (*my_data->bot).getTasks(my_data->chunkSize);

  // Run between iterator ranges, stepping through input1 and output vectors
  for (; my_tasks.in1Begin != my_tasks.in1End; ++my_tasks.in1Begin, ++my_tasks.outBegin)
  {
    metrics_starting_work(my_data->threadId);
    
    // Run user function
    *(my_tasks.outBegin) = my_tasks.userFunction(*(my_tasks.in1Begin), *(my_tasks.input2));

     metrics_finishing_work(my_data->threadId);
  }

  metrics_thread_finished(my_data->threadId);

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
               string output_filename, parameters params = parameters())
{
  // Initialise metrics.
  metrics_init(params.num_threads, output_filename + ".csv");

  // Check input sizes.
  // if (input2.size() != output.size())
  // {
    // Resize output if needed
    // output.clear();
    // output.resize(input2.size());
  // }

  // Print the number of processors we can detect.
  print("[Main] Found ", params.num_threads, " processors\n");

  BagOfTasks<in1, in2, out> bot(input1.begin(), input1.end(), &input2, user_function, output.begin());

  struct  thread_data<in1, in2, out> thread_data_array[params.num_threads];

  // Calculate info for data partitioning.
  uint32_t length    = input1.size();
  uint32_t quotient  = length / params.num_threads;
  uint32_t remainder = length % params.num_threads;

  // Set thread data values.
  for (long i = 0; i < params.num_threads; i++)
  {
    thread_data_array[i].threadId   = i;
    thread_data_array[i].chunkSize  = quotient;
    thread_data_array[i].bot = &bot;

    // If we still have remainder tasks, add one to this thread.
    if (i < remainder) 
    { 
      thread_data_array[i].chunkSize++; 
    }
  }

  // Variables for creating and managing threads.
  pthread_t threads[params.num_threads];
  pthread_attr_t attr;
  int rc;

  // Initialize and set thread detached attribute to joinable.
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  // Create all our needed threads.
  for (long i = 0; i < params.num_threads; i++)
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

  // Free attribute and join with the other threads.
  pthread_attr_destroy(&attr);
  for (long i = 0; i < params.num_threads; i++)
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

  metrics_finalise();

  metrics_calc();

  metrics_exit();
}

#endif /* MAP_ARRAY_H */
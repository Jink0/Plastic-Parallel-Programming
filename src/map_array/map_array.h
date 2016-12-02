#ifndef MAP_ARRAY_H
#define MAP_ARRAY_H

#include <iostream>  // cout
#include <vector>    // Vectors
#include <assert.h>  // Assert functions
#include <pthread.h> // Thread and mutex functions
#include <mutex>     // mutex
#include <unistd.h>  // For sleep()
using namespace std;

#include <metrics.h>



/*
 * This file contrains the definition of the map array pattern. Note - all the definitions are in the header file, as 
 * you cannot seperate the definition of a template class from its declaration and put it inside a .cpp file.
 */



/*
 *  Mutexed print function.
 */

ostream&
print_one(ostream& os)
{
  return os;
}

template <class A0, class ...Args>
ostream&
print_one(ostream& os, const A0& a0, const Args& ...args)
{
  os << a0;
  return print_one(os, args...);
}

template <class ...Args>
ostream&
print(ostream& os, const Args& ...args)
{
  return print_one(os, args...);
}

template <class ...Args>
ostream&
print(const Args& ...args)
{
  static mutex m;
  lock_guard<mutex> _(m);
  return print(cout, args...);
}



template <typename in1, typename in2, typename out>
struct tasks
{
  typename vector<in1>::iterator in1Begin;
  typename vector<in1>::iterator in1End;

  vector<in2>* input2;

  out (*userFunction) (in1, vector<in2>);

  typename vector<out>::iterator outBegin;
};



/*template <typename in1, typename in2, typename out>
class BagOfTasks {
  public:
    // Constructor
    BagOfTasks(vector<in1>::iterator in1Begin, 
                           vector<in1>::iterator in1End, 
                           vector<in2>* input2, 
                           out (*userFunction) (in1, vector<in2>), 
                           vector<out>::iterator outBegin)
{
  in1Begin = in1Begin;
  in1End   = in1End;

  input2 = input2;

  userFunction = userFunction;

  outBegin = outBegin;
};

    // Destructor
    ~BagOfTasks() {};
    // Task retreival
    tasks<in1, in2, out> getTasks(uint32_t num);

  private:
    typename vector<in1>::iterator in1Begin;
    typename vector<in1>::iterator in1End;

    vector<in2>* input2;

    out (*userFunction) (in1, vector<in2>);

    typename vector<out>::iterator outBegin;
};*/



/*template <typename in1, typename in2, typename out>
BagOfTasks<in1, in2, out>::BagOfTasks(
                           vector<in1>::iterator in1Begin, 
                           vector<in1>::iterator in1End, 
                           vector<in2>* input2, 
                           out (*userFunction) (in1, vector<in2>), 
                           vector<out>::iterator outBegin)
{
  in1Begin = in1Begin;
  in1End   = in1End;

  input2 = input2;

  userFunction = userFunction;

  outBegin = outBegin;
}*/



/*template <typename in1, typename in2, typename out>
tasks<in1, in2, out> BagOfTasks<in1, in2, out>::getTasks(uint32_t num)
{
  typename vector<in1>::iterator tasksBegin = in1Begin;

  if (num < in1End - in1Begin)
  {
    advance(in1Begin, num);
  }
  else
  {
    advance(in1Begin, in1End - in1Begin);
  }

  struct tasks<in1, in2, out> output = {
    tasksBegin, 
    in1Begin,
    input2,
    userFunction,
    outBegin
  };

  return output;
}*/



// Different possible schedules Static             - Give each thread equal portions.
//                              Dynamic_chunks     - Threads dynamically retrive a chunk of the tasks when they can.
//                              Dynamic_individual - Threads retrieve a single task when they can.
//                              Tapered            - Chunk size starts off large and decreases to better handle load imbalance between iterations.
//                              Auto               - Automatically try to figure out the best schedule.
enum Schedule {Static, Dynamic_chunks, Dynamic_individual, Tapered, Auto};

// Parameters with default values.
struct parameters 
{
    parameters(): task_dist(1), schedule(Dynamic_chunks) 
    { 
      // Most portable method of retriving processor count.
      FILE * fp;
      char result[128];
      fp = popen("/bin/cat /proc/cpuinfo |grep -c '^processor'","r");
      fread(result, 1, sizeof(result)-1, fp);
      fclose(fp);
      num_threads = atoi(result);
    }

    // Number of threads to use.
    int num_threads;

    // Distribution of the tasks.
    int task_dist;

    // Schedule to use.
    Schedule schedule;
};



/*
 *  Data struct to pass to each thread.
 *
 *  int                            threadId                           - Integer ID of the thread.
 *  typename vector<in1>::iterator in1Begin                           - Where to begin in input1.
 *  typename vector<in1>::iterator in1End                             - Where to end in input1.
 *  vector<in2>                    input2                             - Pointer to vector input2.
 *  out                            (*userFunction) (in1, vector<in2>) - User function pointer to a function which takes 
 *                                                                      (in1, vector<in2>) and returns an out type.
 *  typename vector<out>::iterator outBegin                           - Where to start in output.
 */

template <typename in1, typename in2, typename out>
struct thread_data
{
  int  threadId;

  typename vector<in1>::iterator in1Begin;
  typename vector<in1>::iterator in1End;

  vector<in2> input2;

  out (*userFunction) (in1, vector<in2>);

  typename vector<out>::iterator outBegin;
};



/*
 *  Function to start each thread of mapArray on.
 *
 *  void *threadarg - Pointer to thread_data structure.
 */

template <typename in1, typename in2, typename out>
void *mapArrayThread(void *threadarg)
{
  // Pointer to store personal data
  struct thread_data<in1, in2, out> *my_data;
  my_data = (struct thread_data<in1, in2, out> *) threadarg;

  // Initialise metrics
  metrics_thread_start(my_data->threadId);

  // Print starting parameters
  print("[Thread ", my_data->threadId, "] Hello! I will process ", my_data->in1End - my_data->in1Begin, " tasks\n");

  // Run between iterator ranges, stepping through input1 and output vectors
  for (; my_data->in1Begin != my_data->in1End; ++my_data->in1Begin, ++my_data->outBegin)
  {
    metrics_starting_work(my_data->threadId);
    
    // Run user function
    *(my_data->outBegin) = my_data->userFunction(*(my_data->in1Begin), my_data->input2);

     metrics_finishing_work(my_data->threadId);
  }

  metrics_thread_finished(my_data->threadId);

  pthread_exit(NULL);
}



template <typename in1, typename in2, typename out>
class BagOfTasks {
  public:
    // Constructor
    BagOfTasks() {};

    BagOfTasks(vector<in1>::iterator in1Begin);

    // Destructor
    ~BagOfTasks() {};
    // Task retreival
    tasks<in1, in2, out> getTasks(uint32_t num);

  private:
    typename vector<in1>::iterator in1Begin;
    typename vector<in1>::iterator in1End;

    vector<in2>* input2;

    out (*userFunction) (in1, vector<in2>);

    typename vector<out>::iterator outBegin;
};



template <typename in1, typename in2, typename out>
BagOfTasks<in1, in2, out>::BagOfTasks(vector<in1>::iterator in1Begin)
: in1Begin(in1Begin)
{}



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

  struct BagOfTasks<in1, in2, out>* bagOfTasks = new BagOfTasks<in1, in2, out>(input1.begin());



  // Thread data struct array to store data structs for each thread.
  struct thread_data<in1, in2, out> thread_data_array[params.num_threads];

  // Calculate info for data partitioning.
  int length    = input1.size();
  int quotient  = length / params.num_threads;
  int remainder = length % params.num_threads;

  // Variables for iterators.
  typename vector<in1>::iterator in1Begin = input1.begin();
  typename vector<in1>::iterator in1End;
  typename vector<out>::iterator outBegin = output.begin();

  // Set thread data values.
  for (long i = 0; i < params.num_threads; i++)
  {
    thread_data_array[i].threadId     = i;
    thread_data_array[i].in1Begin     = in1Begin;

    advance(in1Begin, quotient);

    thread_data_array[i].in1End       = in1Begin;
    thread_data_array[i].input2       = input2;
    thread_data_array[i].userFunction = user_function;
    thread_data_array[i].outBegin     = outBegin;

    advance(outBegin, quotient);

    // If we still have remainder tasks, add one to this thread.
    if (i < remainder) 
    { 
      thread_data_array[i].in1End++; 
      in1Begin++;
      outBegin++;
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
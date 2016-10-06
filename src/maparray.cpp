#include <iostream>
#include <vector>
#include <stdlib.h>

#include <assert.h> // 

#include <pthread.h> // Thread and mutex functions
#include <mutex> // std::mutex

#include <unistd.h> // For sleep()
using namespace std;



/*
 *  Mutexed print function
 */

std::ostream&
print_one(std::ostream& os)
{
  return os;
}

template <class A0, class ...Args>
std::ostream&
print_one(std::ostream& os, const A0& a0, const Args& ...args)
{
  os << a0;
  return print_one(os, args...);
}

template <class ...Args>
std::ostream&
print(std::ostream& os, const Args& ...args)
{
  return print_one(os, args...);
}

template <class ...Args>
std::ostream&
print(const Args& ...args)
{
  static std::mutex m;
  std::lock_guard<std::mutex> _(m);
  return print(std::cout, args...);
}



/*
 *  Data struct to pass to each thread
 *
 *  int                            threadId                           - Integer ID of the thread
 *  typename vector<in1>::iterator in1Begin                           - Where to begin in input1
 *  typename vector<in1>::iterator in1End                             - Where to end in input1
 *  vector<in2>                    input2                             - Pointer to vector input2
 *  out                            (*userFunction) (in1, vector<in2>) - User function pointer to a function which takes 
 *                                                                      (in1, vector<in2>) and returns an out type
 *  typename vector<out>::iterator outBegin                           - Where to start in output
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
 *  Function to start each thread of mapArray on
 *
 *  void *threadarg - Pointer to thread_data structure
 */

template <typename in1, typename in2, typename out>
void *mapArrayThread(void *threadarg)
{
  // Sleeping for a second just creates nicer, ordered text output
  sleep(1);

  // Pointer to store personal data
  struct thread_data<in1, in2, out> *my_data;
  my_data = (struct thread_data<in1, in2, out> *) threadarg;

  // Print starting parameters
  print("[Thread ", my_data->threadId, "] Hello! I will process ", my_data->in1End - my_data->in1Begin, " tasks\n");

  // Run between iterator ranges, stepping through input1 and output vectors
  for (; my_data->in1Begin != my_data->in1End; ++my_data->in1Begin, ++my_data->outBegin)
  {
    // Run user function
    *(my_data->outBegin) = my_data->userFunction(*(my_data->in1Begin), my_data->input2);
  }

  pthread_exit(NULL);
}



/*
 *  Implementation of the mapArray parallel programming pattern. Currently uses all available cores and splits tasks 
 *  evenly. If the output vector is not big enough, it will be resized
 *
 *  vector<in1>& input1                              - First input vector to be iterated over
 *  vector<in2>& input2                              - Second input vector to be passed to user function
 *  out          (*user_function) (in1, vector<in2>) - User function pointer to a function which takes 
 *                                                     (in1, vector<in2>) and returns an out type
 *  vector<out>& output                              - Vector to store output in
 */

template <typename in1, typename in2, typename out>
void map_array(vector<in1>& input1, vector<in2>& input2, out (*user_function) (in1, vector<in2>), vector<out>& output)
{
  // Check input sizes
  if (input2.size() != output.size())
  {
    // Resize output if needed
    output.clear();
    output.resize(input2.size());
  }

  // Most portable method of retriving processor count
  FILE * fp;
  char result[128];
  fp = popen("/bin/cat /proc/cpuinfo |grep -c '^processor'","r");
  fread(result, 1, sizeof(result)-1, fp);
  fclose(fp);
  int num_threads = atoi(result);

  // Print the number of processors we can detect
  print("[Main] Found ", num_threads, " processors\n");
  
  // Thread data struct array to store data structs for each thread
  struct thread_data<in1, in2, out> thread_data_array[num_threads];

  // Calculate info for data partitioning
  int length    = input1.size();
  int quotient  = length / num_threads;
  int remainder = length % num_threads;

  // Variables for iterators
  typename vector<in1>::iterator in1Begin = input1.begin();
  typename vector<in1>::iterator in1End;
  typename vector<out>::iterator outBegin = output.begin();

  // Set thread data values
  for (long i = 0; i < num_threads; i++)
  {
    thread_data_array[i].threadId     = i;
    thread_data_array[i].in1Begin     = in1Begin;

    advance(in1Begin, quotient);

    thread_data_array[i].in1End       = in1Begin;
    thread_data_array[i].input2       = input2;
    thread_data_array[i].userFunction = user_function;
    thread_data_array[i].outBegin     = outBegin;

    advance(outBegin, quotient);

    // If we still have remainder tasks, add one to this thread
    if (i < remainder) 
    { 
      thread_data_array[i].in1End++; 
      in1Begin++;
      outBegin++;
    }
  }

  // Variables for creating and managing threads
  pthread_t threads[num_threads];
  pthread_attr_t attr;
  int rc;

  // Initialize and set thread detached attribute to joinable
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  // Create all our needed threads
  for (long i = 0; i < num_threads; i++)
  {
    print("[Main] Creating thread ", i , "\n");

    rc = pthread_create(&threads[i], &attr, mapArrayThread<in1, in2, out>, (void *) &thread_data_array[i]);

    if (rc)
    {
      // If we couldn't create a new thread, throw an error and exit
      print("[Main] ERROR; return code from pthread_create() is ", rc, "\n");
      exit(-1);
    }
  }

  // Free attribute and join with the other threads
  pthread_attr_destroy(&attr);
  for (long i = 0; i < num_threads; i++)
  {
    rc = pthread_join(threads[i], NULL);

    if (rc)
    {
      // If we couldn't create a new thread, throw an error and exit
      print("[Main] ERROR; return code from pthread_join() is ", rc, "\n");
      exit(-1);
    }

    print("[Main] Joined with thread ", i, "\n");
  }
}



/* 
 *  Test user function, very simple. Creates a perfectly balanced workload
 */

int userFunction(int in1, vector<int> in2)
{
  return in1 + 10;
}



int main()
{
  // Test input vectors
  vector<int> input1;
  vector<int> input2;

  // Push values onto the vectors
  for (int i = 0; i < 20; i++) {
    input1.push_back(i);
    input2.push_back(i * 2);
  }

  // Create output vector
  vector<int> output(input1.size());

  // Start mapArray
  map_array(input1, input2, userFunction, output);

  print("\n\nVector 1: \n");

  // Print input/output vectors
  for (vector<int>::const_iterator i = input1.begin(); i != input1.end(); ++i)
    print(*i, ' ');

  print("\n\nVector 2: \n");

  for (vector<int>::const_iterator i = input2.begin(); i != input2.end(); ++i)
    print(*i, ' ');

  print("\n\nOutput vector: \n");

  for (vector<int>::const_iterator i = output.begin(); i != output.end(); ++i)
    print(*i, ' ');

  print("\n\n");

  return 0;
}
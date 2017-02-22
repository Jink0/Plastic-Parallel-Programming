#ifndef MAP_ARRAY_H
#define MAP_ARRAY_H

#include <iostream>         // cout
#include <vector>           // Vectors
#include <deque>            // Double queues
#include <mutex>            // mutexes
#include <pthread.h>        // Thread and mutex functions
#include <unistd.h>         // sleep()
#include <cxxabi.h>         // Demangling typenames (gcc only)
#include <boost/thread.hpp> // boost::thread::hardware_concurrency();
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
    deque<int> thread_pinnings;

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

    // If successful;
    if(status == 0) 
    {
        // Record name.
        mName = dmName;

        // Free memory.
        free(dmName);
    }   

    return mName;
}



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



// Thread control enum. threads either run (execute), change strategies (update), or stop (terminte).
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

enum Status {Alive, Sleeping, Terminated};

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



void join_with_threads(deque<pthread_t> threads, uint32_t num_threads_to_join) 
{
  int inital_threads_max_index = threads.size() - 1;

  for (uint32_t i = 0; i < num_threads_to_join; i++)
  {
    int rc = pthread_join(threads.back(), NULL);

    threads.pop_back();

    if (rc)
    {
      // If we couldn't create a new thread, throw an error and exit.
      print("[Main] ERROR; return code from pthread_join() is ", rc, "\n");
      exit(-1);
    }

    print("[Main] Joined with thread ", inital_threads_max_index - i, "\n");
  }
}



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
  Ms(metrics_init(params.thread_pinnings.size(), output_filename + ".csv"));

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

  //zmq_setsockopt(socket, ZMQ_RCVTIMEO, 0);

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

      //print(msg.settings.schedule, "\n\n\n");

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

  m_send(socket, term);

  socket.close();

  join_with_threads(threads, params.thread_pinnings.size());

  Ms(metrics_finalise());

  Ms(metrics_calc());

  Ms(metrics_exit());
}

#endif /* MAP_ARRAY_H */
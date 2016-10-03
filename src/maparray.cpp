#include <iostream>
#include <vector>
#include <stdlib.h>

#include <assert.h> // 

#include <pthread.h> // Thread and mutex functions
#include <mutex> // std::mutex

#include <unistd.h> // For sleep()
using namespace std;

#define DEBUG
#define NUM_THREADS 8



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
 *  Macro definition for user functions. The definition expands as a struct which can be used when
 *  creating new patterns. Note - \ lets you continue a statement onto the next line, so we can
 *  write #define statements on multiple lines like this.
 */

#define USER_FUNCTION(name, inType1, in1, inType2, in2, outType, func)\
struct name\
{\
    typedef inType1 IT1;\
    typedef inType2 IT2;\
    typedef outType OT;\
    \
    inline outType function(inType1 in1, vector<inType2>& in2)\
    {\
        func\
    }\
};



/*
 *  A quick test function for map array. Returns sum of the product of val with each number in vect,
 *  produces a perfectly balanced workload.
 */

USER_FUNCTION(testFunction, int, i1, int, i2, int,

              int output = 2;

              // for (vector<int>::const_iterator i = i2.begin(); i != i2.end(); ++i)
              // output += i1 * *i;

              return output;)


template <typename Input1Iterator, typename T, typename OutputIterator>
struct thread_data
{
  int  threadId;

  Input1Iterator input1Begin;
  Input1Iterator input1End;
  // T *input2;
  OutputIterator outputBegin;
};


template <typename Input1Iterator, typename T, typename OutputIterator>
void *mapArrayThread(void *threadarg)
{
  struct thread_data<Input1Iterator, T, OutputIterator> *my_data;

  sleep(1);

  my_data = (struct thread_data<Input1Iterator, T, OutputIterator> *) threadarg;

  print("[Thread ", my_data->threadId, "] Hello!\n");

  // *outputBegin = m_mapArrayFunc->function(*input1Begin,input2);

  pthread_exit(NULL);
}



/*  A class representing the MapArray skeleton.
 *
 *  This class implements the mapArray pattern. MapArray is a variant of Map. It produces a result
 *  vector from two input vectors where each element of the result is a function of the
 *  corresponding element of the first input, and any number of elements from the second input.
 *  This means that at each call to the user defined function, which is done for each element in
 *  input one, all elements from input two can be accessed.
 */

template <typename MapArrayFunc>
class MapArray
{
private:
  MapArrayFunc* m_mapArrayFunc;

public:
  MapArray(MapArrayFunc* mapArrayFunc)
  {
    m_mapArrayFunc = mapArrayFunc;
  };

  ~MapArray()
  {
    delete m_mapArrayFunc;
  };

  template <typename T>
  void execute(vector<T>& input1, vector<T>& input2, vector<T>& output)
  {
    if (input2.size() != output.size())
    {
      output.clear();
      output.resize(input2.size());
    }

    execute(input1.begin(), input1.end(), input2, output.begin());
  };

  template <typename Input1Iterator, typename T, typename OutputIterator>
  void execute(Input1Iterator input1Begin, Input1Iterator input1End, vector<T>& input2, OutputIterator outputBegin)
  {
    pthread_t threads[NUM_THREADS];
    pthread_attr_t attr;
    int rc;
    void *status;

    struct thread_data<Input1Iterator, T, OutputIterator> thread_data_array[NUM_THREADS];

    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for (long i = 0; i < NUM_THREADS; i++)
    {
      thread_data_array[i].threadId = i;
      thread_data_array[i].input1Begin = input1Begin;
      thread_data_array[i].input1End = input1End;
      // thread_data_array[i].input2 = *input2;
      thread_data_array[i].outputBegin = outputBegin;

      print("Creating thread ", i , "\n");
      rc = pthread_create(&threads[i], &attr, mapArrayThread<Input1Iterator, T, OutputIterator>, (void *) &thread_data_array[i]);

      if (rc)
      {
        print("ERROR; return code from pthread_create() is ", rc, "\n");
        exit(-1);
      }
    }

    /* Free attribute and wait for the other threads */
    pthread_attr_destroy(&attr);
    for (long i = 0; i < NUM_THREADS; i++)
    {
      rc = pthread_join(threads[i], &status);
      if (rc)
      {
        print("ERROR; return code from pthread_join() is ", rc, "\n");
        exit(-1);
      }
      print("Main: completed join with thread ", i, "\n");
    }

  }
};



int main()
{
  // Test input vectors
  vector<int> input1;
  vector<int> input2;

  // Push values onto the vectors
  for (int i = 0; i < 13; i++) {
    input1.push_back(i);
    input2.push_back(i);
  }

  // Create output vector
  vector<int> output(input1.size());

  // Run mapArray
  MapArray<testFunction> testMapArray(new testFunction);

  testMapArray.execute(input1, input2, output);

  // Print input/output vectors
  for (vector<int>::const_iterator i = input1.begin(); i != input1.end(); ++i)
    print(*i, ' ');

  print("\n");

  for (vector<int>::const_iterator i = input2.begin(); i != input2.end(); ++i)
    print(*i, ' ');

  print("\n");

  for (vector<int>::const_iterator i = output.begin(); i != output.end(); ++i)
    print(*i, ' ');

  return 0;
}
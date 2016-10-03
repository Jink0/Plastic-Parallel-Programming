#include <iostream>
#include <vector>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
using namespace std;

#define DEBUG
#define NUM_THREADS 8



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
   // struct thread_data<Input1Iterator, T, OutputIterator> *my_data;

   // sleep(1);

   // my_data = (struct thread_data<Input1Iterator, T, OutputIterator> *) threadarg;

   // cout << "[Thread " << my_data->threadId << "] Hello!" << endl;

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
      if(input2.size() != output.size())
      {  
        output.clear();
        output.resize(input2.size());
      }

      execute(input1.begin(), input1.end(), input2, output.begin());
    };

   template <typename Input1Iterator, typename T, typename OutputIterator>
   void execute(Input1Iterator input1Begin, Input1Iterator input1End, vector<T>& input2, OutputIterator outputBegin)
    {
      int rc;
      pthread_t threads[NUM_THREADS];
      struct thread_data<Input1Iterator, T, OutputIterator> thread_data_array[NUM_THREADS];

      for (int i = 0; i < NUM_THREADS; i++) 
      {
        thread_data_array[i].threadId = i;
        thread_data_array[i].input1Begin = input1Begin;
        thread_data_array[i].input1End = input1End;
        // thread_data_array[i].input2 = *input2;
        thread_data_array[i].outputBegin = outputBegin;
      // }

      // for (; input1Begin != input1End; ++input1Begin, ++outputBegin)
      // {
        cout << "Creating thread " << i << endl;
        rc = pthread_create(&threads[i], NULL, mapArrayThread<Input1Iterator, T, OutputIterator>, (void *) &thread_data_array[i]);

        // *outputBegin = m_mapArrayFunc->function(*input1Begin,input2);

        // if (rc)
        // {
        //   printf("ERROR; return code from pthread_create() is %d\n", rc);
        //   exit(-1);
        // }
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
      cout << *i << ' ';

   cout << endl;

   for (vector<int>::const_iterator i = input2.begin(); i != input2.end(); ++i)
      cout << *i << ' ';

   cout << endl;

   for (vector<int>::const_iterator i = output.begin(); i != output.end(); ++i)
      cout << *i << ' ';

   return 0;
}








// template <typename T1, typename T2, typename T3>
// struct threadDataStruct {
//    int threadId;

//    int startIndex;
//    int numTasks;
//    it1 test;

//    vector<it1> *input1;
//    vector<IT2> *input2;
//    vector<OT> *output;
// };






// template <typename T1, typename T2, typename T3>
// void *mapArrayThread(void *threadData)
// {
//    // struct threadDataStruct<T1, T2, T3> *data = (struct threadDataStruct<T1, T2, T3> *) threadData;
//    struct threadDataStruct *data = (struct threadDataStruct *) threadData;

//    cout << "[User Thread " << data->threadId << "]: " << endl << "startIndex: " << data->startIndex << endl << "numTasks: " << data->numTasks << endl;

//    for (int i = 0; i < data->numTasks; i++) {
//       // data->output[data->startIndex + i] = testFunction(data->input1[i], data->input2);
//    }

//    pthread_exit(NULL);
// }

// template <typename T1, typename T2, typename T3>
// void mapArray(vector<T1>& input1, vector<T2>& input2, T3 (function)(T1, vector<T2>&), vector<T3>& output)
// {
//    assert(input1.size() == output.size() && "Input1 and output array sizes must be equal!");

//    int base  = input1.size() / NUM_THREADS;
//    int extra = input1.size() % NUM_THREADS;
//    int baseTotal = 0;

//    pthread_t threads[NUM_THREADS];
//    // struct threadDataStruct<T1, T2, T3> threadData[NUM_THREADS];
//    struct threadDataStruct threadData[NUM_THREADS];

//    for (int i = 0; i < NUM_THREADS; i++ ) {
//       // Writing data to pass to thread
//       threadData[i].threadId   = i;
//       threadData[i].startIndex = baseTotal;

//       if (extra > 0) {
//          threadData[i].numTasks = base + 1;
//          extra--;
//       } else {
//          threadData[i].numTasks = base;
//       }

//       baseTotal += threadData[i].numTasks;

//       // threadData[i].input1     = &input1;

//       // int test = input1[0];
      
//       // threadData[i].input2     = &input2;
//       // threadData[i].function   = function;
//       // threadData[i].output     = &output;

//       cout << "[User Program_]: Creating thread " << i << endl;

//       // Create our thread, and pass a pointer to it's threadDataStruct
//       int err = pthread_create(&threads[i], NULL, mapArrayThread<T1, T2, T3>, (void *)&threadData[i]);

//       // Checking for errors creating the thread
//       if (err) {
//          cout << "Error:unable to create thread, " << err << endl;
//          exit(-1);
//       }
//    }
//    pthread_exit(NULL);
// }
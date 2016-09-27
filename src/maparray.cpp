#include <iostream>
#include <vector>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
using namespace std;

#define DEBUG
#define NUM_THREADS 5

// Test function for map array. Returns sum of the product of val with each number in vect, produces a perfectly balanced workload
double testFunction(int val, vector<int>& vect)
{
   int output = 0;

   for (vector<int>::const_iterator i = vect.begin(); i != vect.end(); ++i)
      output += val * *i;

   return output;
}

template <typename T1, typename T2, typename T3>
struct threadDataStruct {
   int threadId;

   int startIndex;
   int numTasks;

   vector<T1> *input1;
   vector<T2> *input2;
   T3 (*function)(T1, vector<T2>&);
   vector<T3> *output;
};

template <typename T1, typename T2, typename T3>
void *mapArrayThread(void *threadData)
{
   struct threadDataStruct<T1, T2, T3> *data = (struct threadDataStruct<T1, T2, T3> *) threadData;

   cout << "[User Thread " << data->threadId << "]: " << endl << "startIndex: " << data->startIndex << endl << "numTasks: " << data->numTasks << endl;

   for (int i = 0; i < data->numTasks; i++) {
      // data->output[data->startIndex + i] = testFunction(data->input1[i], data->input2);
   }

   pthread_exit(NULL);
}

template <typename T1, typename T2, typename T3>
void mapArray(vector<T1>& input1, vector<T2>& input2, T3 (function)(T1, vector<T2>&), vector<T3>& output)
{
   assert(input1.size() == output.size() && "Input1 and output array sizes must be equal!");

   int base  = input1.size() / NUM_THREADS;
   int extra = input1.size() % NUM_THREADS;
   int baseTotal = 0;

   pthread_t threads[NUM_THREADS];
   struct threadDataStruct<T1, T2, T3> threadData[NUM_THREADS];

   for (int i = 0; i < NUM_THREADS; i++ ) {
      // Writing data to pass to thread
      threadData[i].threadId   = i;
      threadData[i].startIndex = baseTotal;

      if (extra > 0) {
         threadData[i].numTasks = base + 1;
         extra--;
      } else {
         threadData[i].numTasks = base;
      }

      baseTotal += threadData[i].numTasks;

      threadData[i].input1     = &input1;
      cout << "HEY " << threadData[i]->input1 << endl;
      threadData[i].input2     = &input2;
      threadData[i].function   = function;
      threadData[i].output     = &output;

      cout << "[User Program_]: Creating thread " << i << endl;

      // Create our thread, and pass a pointer to it's threadDataStruct
      int err = pthread_create(&threads[i], NULL, mapArrayThread<T1, T2, T3>, (void *)&threadData[i]);

      // Checking for errors creating the thread
      if (err) {
         cout << "Error:unable to create thread, " << err << endl;
         exit(-1);
      }
   }
   pthread_exit(NULL);
}

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
   vector<double> output(input1.size());

   // Run mapArray
   mapArray(input1, input2, testFunction, output);

   // Print input/output vectors
   for (vector<int>::const_iterator i = input1.begin(); i != input1.end(); ++i)
      cout << *i << ' ';

   cout << endl;

   for (vector<int>::const_iterator i = input2.begin(); i != input2.end(); ++i)
      cout << *i << ' ';

   cout << endl;

   for (vector<double>::const_iterator i = output.begin(); i != output.end(); ++i)
      cout << *i << ' ';

   return 0;
}
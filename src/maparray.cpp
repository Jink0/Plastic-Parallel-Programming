#include <iostream>
#include <vector>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
using namespace std;

#define DEBUG 
#define NUM_THREADS 1

template <typename T1, typename T2, typename T3>
struct threadDataStruct {
   int threadId;

   int startIndex;
   int numTasks;

   vector<T1>& input1;
   vector<T2>& input2;
   vector<T3>& output;
   T3 (*function)(T1, vector<T2>&);
};

template <typename T1, typename T2, typename T3>
class threadDataClass {
   private:
      int threadId;

      int startIndex;
      int numTasks;

      vector<T1>& input1;
      vector<T2>& input2;
      vector<T3>& output;
      T3 (*function)(T1, T2);

   public:
      threadDataClass(int id, int start, int tasks, vector<T1>& in1, vector<T2>& in2, vector<T3>& out, T3 (*func)(T1, T2))
         : threadId(id)
         , startIndex(start)
         , numTasks(tasks)
         , input1(in1)
         , input2(in2)
         , output(out)
         , function(func)
      { }
};

// Test function for map array. Returns sum of the product of val with each number in vect
double testFunction(int val, vector<int>& vect)
{
   int output = 0;

   for (vector<int>::const_iterator i = vect.begin(); i != vect.end(); ++i)
      output += val * *i;

   return output;
}

template <typename T1, typename T2, typename T3>
// void mapArrayThreadTemplate(T3 (function)(T1, vector<T2>&), vector<T1>& input1, vector<T2>& input2, vector<T3>& output, int startIndex, int numTasks)
void *mapArrayThread(void *threadarg)
{
   struct threadDataStruct<T1, T2, T3> *myData;

   myData = (struct threadDataStruct<T1, T2, T3> *) threadarg;

   cout << "Thread data:\n" << myData->threadId << endl << myData->startIndex << endl << myData->numTasks << endl << myData->input1[3] << endl << myData->input2[3] << endl << myData->output[3] << endl;

   for (int i = 0; i < myData->numTasks; i++) {
      myData->output[myData->startIndex + i] = function(myData->input1[i], myData->input2);
   }
}

template <typename T1, typename T2, typename T3>
void mapArray(T3 (function)(T1, vector<T2>&), vector<T1>& input1, vector<T2>& input2, vector<T3>& output)
{
   assert(input1.size() == output.size() && "input1 and output array sizes must be equal!");

   int base  = input1.size() / NUM_THREADS;
   int extra = input1.size() % NUM_THREADS;

   pthread_t threads[NUM_THREADS];
   struct threadDataStruct<T1, T2, T3> *threadData[NUM_THREADS];
   int rc;

   for(int i = 0; i < NUM_THREADS; i++) {
      cout <<"main() : creating thread, " << i << endl;
      threadData[i]->threadId = i;
      threadData[i]->startIndex = 0;
      threadData[i]->numTasks = input1.size();
      threadData[i]->input1 = input1;
      threadData[i]->input2 = input2;
      threadData[i]->output = output;
      threadData[i]->function = function;

      rc = pthread_create(&threads[i], NULL, mapArrayThread, threadData[i]);
      if (rc){
         cout << "Error:unable to create thread," << rc << endl;
         exit(-1);
      }
   }

   for (int i = 0; i < input1.size(); i++) {
      output[i] = function(input1[i], input2);
   }
}
 
int main()
{
   // Test input vectors
   vector<int> input1;
   vector<int> input2;

   int i;

   // Push 5 values into the vectors
   for(i = 0; i < 10; i++){
      input1.push_back(i);
      input2.push_back(i);
   }

   vector<double> output(input1.size());

   mapArray(testFunction, input1, input2, output);

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
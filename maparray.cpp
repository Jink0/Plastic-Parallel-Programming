#include <iostream>
#include <vector>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
using namespace std;

#define DEBUG 
#define NUM_THREADS 8

// void mapArrayThreadTemplate(T3 (function)(T1, vector<T2>&), vector<T1>& input1, vector<T2>& input2, vector<T3>& output, int startIndex, int numTasks)

template <typename T1, typename T2, typename T3>
struct threadDataStruct {
   int threadId;
   int startIndex;
   int numTasks;

   T3 (function)(T1, vector<T2>&);
   vector<T1>& input1;
   vector<T2>& input2;
   vector<T3>& output;
};

// Test function for map array. Returns sum of the product of val with each number in vect
double testFunction(int val, vector<int>& vect)
{
   int output = 0;

   for (vector<int>::const_iterator i = vect.begin(); i != vect.end(); ++i)
      output += val * *i;

   return output;
}

void mapArrayThread(int (function)(int, vector<int>&), int startIndex, int numTasks, vector<int>& input1, vector<int>& input2, vector<int>& output)
{
   for (int i = 0; i < numTasks; i++) {
      output[startIndex + i] = function(input1[i], input2);
   }
}

void mapArray(int (function)(int, vector<int>&), vector<int>& input1, vector<int>& input2, vector<int>& output)
{
   // assert(input1.size() == output.size() && "input1 and output array sizes must be equal!");

   // int base  = input1.size() / NUM_THREADS;
   // int extra = input1.size() % NUM_THREADS;

   // //mapArrayThread(function, 0, input1.size(), input1, input2, output);

   // pthread_t threads[NUM_THREADS];
   // struct thread_data td[NUM_THREADS];
   // int rc;
   // int i;

   // for(i=0; i < NUM_THREADS; i++ ){
   //    cout <<"main() : creating thread, " << i << endl;
   //    td[i].thread_id = i;
   //    td[i].message = "This is message";
   //    rc = pthread_create(&threads[i], NULL,
   //                        PrintHello, (void *)&td[i]);
   //    if (rc){
   //       cout << "Error:unable to create thread," << rc << endl;
   //       exit(-1);
   //    }
   // }
}

// template <typename T1, typename T2, typename T3>
// void mapArrayThreadTemplate(T3 (function)(T1, vector<T2>&), vector<T1>& input1, vector<T2>& input2, vector<T3>& output, int startIndex, int numTasks)
void *mapArrayThreadTemplate(void *threadarg)
{
   struct threadDataStruct *myData;

   myData = (struct threadDataStruct *) threadarg;

   for (int i = 0; i < myData->numTasks; i++) {
      output[startIndex + i] = function(input1[i], input2);
   }
}

template <typename T1, typename T2, typename T3>
void mapArrayTemplate(T3 (function)(T1, vector<T2>&), vector<T1>& input1, vector<T2>& input2, vector<T3>& output)
{
   assert(input1.size() == output.size() && "input1 and output array sizes must be equal!");

   int base  = input1.size() / NUM_THREADS;
   int extra = input1.size() % NUM_THREADS;

   pthread_t threads[NUM_THREADS];
   struct threadDataStruct threadData[NUM_THREADS];
   int rc;

   for(int i = 0; i < NUM_THREADS; i++) {
      cout <<"main() : creating thread, " << i << endl;
      threadData[i].threadId = i;

      rc = pthread_create(&threads[i], NULL,
                          mapArrayThreadTemplate, (void *)&threadData[i]);
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

   mapArrayTemplate(testFunction, input1, input2, output);

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
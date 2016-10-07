#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "map_array.h"



/*
 * Simulate different types of array access, reading/writing,
 * sequential vs random. Items are summed are result returned
 * so compiler does not optimise away calls.
 * All arrays passed in as double. Indices accessed are
 * min <= index < max
 */



/*
 * Generate a random number N, such that min <= N < max
 */

static int get_rand(unsigned int *seed, int min, int max) 
{
    if (max <= min) 
    {
        fprintf(stderr, "Error: invalid values min:%d  max:%d, need min<max\n", min, max);
        exit(EXIT_FAILURE);
    }

    return min + (rand_r(seed) % (max - min));
}



/*
 * Read random array elements. Use re-entrant PRNG as may be used by multiple threads at once
 */

double read_random(double *arr, int min, int max, int num) 
{
    unsigned seed;
    seed = (unsigned) time(NULL);

    double sum = 0;
    int i, index;

    for (i = 0; i < num; i++) 
    {
        index = get_rand(&seed, min, max);
        sum += arr[index];
    }

    return sum;
}



/*
 * Sequentially read array
 */

double read_sequential(double *arr, int min, int max) 
{
    double sum = 0.;
    int i;

    for (i = min; i < max; i++) 
    {
        sum += arr[i];
    }

    return sum;
}



/*
 * Sequantially read array, however get random numbers to mimic times for random access
 */

double read_sequential_adjusted(double *arr, int min, int max) 
{
    double sum = 0.;
    int i;

    volatile int index; // stop compiler from optimising away unused index
    unsigned seed;
    seed = (unsigned) time(NULL);

    for (i = min; i < max; i++) 
    {
        index = get_rand(&seed, min, max);
        sum += arr[i];
    }

    return sum;
}



/*
 * Write to num array elements min<=N<max
 */

double write_random(double *arr, int min, int max, int num) 
{
    int i, index;
    unsigned seed;
    seed = (unsigned) time(NULL);

    for (i = 0; i < num; i++) 
    {
        index = get_rand(&seed, min, max);
        arr[index] = (double) i;
    }

    return (double) i;
}



/*
 * Write sequentially to array entries.
 */

double write_sequential(double *arr, int min, int max) 
{
    int i;

    for (i = min; i < max; i++) 
    {
        arr[i] = (double) i;
    }

    return (double) i;
}



/*
 * Write sequentially to array entries, adjusted to call RNG
 */

double write_sequential_adjusted(double *arr, int min, int max) 
{
    int i;
    volatile int index; // stop compiler from optimising away unused index
    unsigned seed;
    seed = (unsigned) time(NULL);

    for (i = min; i < max; i++) 
    {
        index = get_rand(&seed, min, max);
        arr[i] = (double) i;
    }

    return (double) i;
}



/*
 * Read and write from random array elements, each time read and write from same element
 */

double read_write_random_same(double *arr, int min, int max, int num) 
{
    double sum = 0;
    int i, index;
    unsigned seed;
    seed = (unsigned) time(NULL);

    for (i = 0; i < num; i++) 
    {
        index = get_rand(&seed, min, max);
        sum += arr[index];
        arr[index] = (double) i;
    }

    return sum;
}



/*
 * Read and write from random array elements, each time read and write from different elements
 */

double read_write_random_diff(double *arr, int min, int max, int num) 
{
    double sum = 0;
    int i, index;
    unsigned seed;
    seed = (unsigned) time(NULL);

    for (i = 0; i < num; i++) 
    {
        index = get_rand(&seed, min, max);
        sum += arr[index];
        index = get_rand(&seed, min, max);
        arr[index] = (double) i;
    }

    return sum;
}



/*
 * Read and write to sequential array elements
 */

double read_write_sequential(double *arr, int min, int max) 
{
    int i;
    double sum = 0;

    for (i = min; i < max; i++) 
    {
        sum += arr[i];
        arr[i] = i;
    }

    return sum;
}



/*
 * Read and write to sequential array elements. Adjusted to use RNG
 */

double read_write_sequential_adjusted(double *arr, int min, int max) 
{
    int i;
    double sum = 0;

    volatile int index; // stop compiler from optimising away unused index
    unsigned seed;
    seed = (unsigned) time(NULL);

    for (i = min; i < max; i++) 
    {
        index = get_rand(&seed, min, max);
        sum += arr[i];
        arr[i] = i;
    }

    return sum;
}



/* 
 *  Test user function, very simple. Creates a perfectly balanced workload
 */

double userFunction(double in1, vector<double> in2)
{
  return in1 + 10;
}



int main()
{
  // Test input vectors
  vector<double> input1;
  vector<double> input2;

  // Push values onto the vectors
  for (int i = 0; i < 100000; i++) {
    input1.push_back(i);
    input2.push_back(i * 2);
  }

  // Create output vector
  vector<double> output(input1.size());

  // // Start mapArray
  // map_array(input1, input2, userFunction, output);

  // int arr_size = 2000000; // size of array to work on
  //   //int arr_size = 1000; // size of array to work on
  //   int work_num = 5000; // number of tasks (each of which works on full array)


  //   double *data = calloc(arr_size, sizeof(double));

  //   int i;
  //   for (i=0; i<arr_size; i++) {
  //       data[i] = (double) 1./i;
  //   }



  //   // Each task puts output into array
  //   double *output_arr = calloc(work_num, sizeof(double));


  //   tf_context_t *tf_context = tf_init(port, output_file);

  //   struct task_struct work_data[work_num];

  //   // Fill out work data structure
  //   for (i=0; i<work_num; i++) {
  //       work_data[i].data = data;
  //       work_data[i].size = arr_size;
  //       work_data[i].output = &output_arr[i];
    // }






  // print("\n\nVector 1: \n");

  // // Print input/output vectors
  // for (vector<int>::const_iterator i = input1.begin(); i != input1.end(); ++i)
  //   print(*i, ' ');

  // print("\n\nVector 2: \n");

  // for (vector<int>::const_iterator i = input2.begin(); i != input2.end(); ++i)
  //   print(*i, ' ');

  // print("\n\nOutput vector: \n");

  // for (vector<int>::const_iterator i = output.begin(); i != output.end(); ++i)
  //   print(*i, ' ');

  // print("\n\n");

  return 0;
}
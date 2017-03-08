#include <parallel_test.hpp>

// Workloads for workload generation
#include "workloads.hpp"

#include <stdint.h>
#include <omp.h>
#include <iostream>

#define CHUNKSIZE 10

int main(int argc, char *argv[])
{
	uint32_t as = 10000;

	/*// Experiment input vectors.
    deque<int> input1(as);
    deque<int> input2(as);

    // Generate data for vectors.
    for (uint32_t i = 0; i < as; i++) 
    {
        // input1[i] = (double) 1. / i;
        // input2[i] = (double) 1. / i;
        // input1[i] = (int) i + 1;
        // input2[i] = (int) i + 1;
        input1[i] = 10000;
        input2[i] = 87736;
    }

    // Output deque.
    deque<int> output(as);

    // Run between iterator ranges, stepping through input1 and output vectors
    for (uint32_t i = 0; i < as; i++)
    { 
      // Run user function
      output.at(i) = collatz(input1.at(i), input2);
    }*/

    int nthreads, tid, i, chunk;
	float a[as], b[as], c[as];

	/* Some initializations */
	for (i=0; i < as; i++)
	  a[i] = b[i] = i * 1.0;
	chunk = CHUNKSIZE;

	#pragma omp parallel shared(a,b,c,nthreads,chunk) private(i,tid)
	  {
	  tid = omp_get_thread_num();
	  if (tid == 0)
	    {
	    nthreads = omp_get_num_threads();
	    std::cout << "Number of threads = " << nthreads << std::endl;
	    }
	  std::cout << "Thread " << tid << " starting...\n";

	  #pragma omp for schedule(dynamic,chunk)
	  for (i=0; i<as; i++)
	    {
	    c[i] = a[i] + b[i];
	    std::cout << "Thread " << tid << ": c[" << i << "]= " << c[i] << std::endl;
	    }

  	}  /* end of parallel section */
}
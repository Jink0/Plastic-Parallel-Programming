#include <sequential_test.hpp>

// Workloads for workload generation
#include "workloads.hpp"

#include <stdint.h>

int main(int argc, char *argv[])
{
	uint32_t as = 10000;

	// Experiment input vectors.
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
    }
}
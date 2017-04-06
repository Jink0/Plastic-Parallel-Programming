#include <sequential_test.hpp>

#include "workloads.hpp" // Workloads for workload generation
#include "metrics.hpp"
#include "utils.hpp"
#include <stdint.h>



int main(int argc, char *argv[])
{
	// Initialise metrics.
  	metrics_init(1, string(argv[1]) + ".csv");
  	metrics_thread_start(0);

	uint32_t as = 10000;

	// Experiment input vectors.
    deque<int> input1(as);
    deque<int> input2(as);

    // Generate data for vectors.
    for (uint32_t i = 0; i < as; i++) 
    {
        input1[i] = 10000;
        input2[i] = 87736;
    }

    // Output deque.
    deque<int> output(as);

    for (uint32_t i = 0; i < as; i++)
    { 
      metrics_starting_work(0);
  
      // Run user function
      output.at(i) = collatz(input1.at(i), input2);

      metrics_finishing_work(0);
    }

    metrics_thread_finished(0);

    // for (uint32_t i = 0; i < as; i++) 
    // {
        // print(output.at(i));
    // }

    metrics_finalise();

  	metrics_calc();

  	metrics_exit();
}
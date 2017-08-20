#ifndef WORKLOADS_HPP
#define WORKLOADS_HPP

#include <deque>            // Double ended queues

#include "config_files_utils.hpp" // For experiment_parameters



// Structure to contain our workload.
template <typename in1, typename in2, typename out>
struct workload {
	// First input deque.
	std::deque<in1> input1;

	// Second input deque.
	std::deque<in2> input2;

	// User function pointer.
	out (*userFunction) (in1, std::deque<in2>);

	struct experiment_parameters params;
};

/*
 * Test user function
 */

int returnOne(int in1, std::deque<int> in2);



/*
 * Test user function
 */

int oneTouch(int in1, std::deque<int> in2);



/*
 * Test user function
 */

int collatz(int weight, std::deque<int> seeds);

//
template <typename in1, typename in2, typename out>
workload<in1, in2, out> generate_workload(struct experiment_parameters params) {
	struct workload<in1, in2, out> output;

	output.params = params;

	uint32_t quotient  = params.array_size / params.task_size_distribution.size();
	uint32_t remainder = params.array_size % params.task_size_distribution.size();

	for (uint32_t i = 0; i < params.task_size_distribution.size(); i++) {
		for (uint32_t j = 0; j < quotient; j++) {
			output.input1.push_back(params.task_size_distribution.at(i));
		}
	}

	for (uint32_t i = 0; i < remainder; i++) {
		output.input1.push_back(params.task_size_distribution.back());
	}

	output.input2.push_back(1);

	switch (params.user_function) {
	case Collatz:
		output.userFunction = collatz;

		break;
	}

	return output;
}

#endif // WORKLOADS_HPP
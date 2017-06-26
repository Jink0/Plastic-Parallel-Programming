#include <parallel_test.hpp>

// Workloads for workload generation
#include "workloads.hpp"

#include "metrics.hpp"
#include "utils.hpp"

#include <stdint.h>
#include <omp.h>
#include <iostream>

#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <boost/filesystem.hpp>

#define CHUNKSIZE 500
#define N_THREADS 4
#define REPEATS   5

#include <deque>

enum Schedule {Static, Dynamic_chunks, Tapered, Auto};

struct experiment_parameters {
	uint32_t number_of_threads;
	Schedule inital_schedule;
	uint32_t inital_chunk_size;
	uint32_t array_size;
	std::deque<uint32_t> task_size_distribution;
};

struct run_parameters {
	uint32_t number_of_repeats     = 0;

	std::deque<experiment_parameters> experiments;
};

std::ostream& print(std::ostream &o, const run_parameters& params) {
	o << "Number of experiments: " << params.experiments.size() << std::endl <<
         "Number of repeats: " << params.number_of_repeats << std::endl << std::endl;

    o << "TODO: Complete printout\n\n";

    for (uint32_t i = 0; i < params.experiments.size(); i++) {
    	o << "Experiment " << i + 1 << ": " << std::endl;
    }

    return o;
}

std::ostream& operator<< (std::ostream &o, const run_parameters &params){
  return print(o, params);
}



boost::property_tree::ptree read_config_file(int argc, char *argv[]) {
	std::cout << std::endl;

	// Check arguments
	if (argc != 2) {
		std::cout << "Incorrect command line arguments!" << std::endl << std::endl
				  << "usage: parallel_test config_file.xml" << std::endl << std::endl;

		exit(EXIT_FAILURE);
	}
 
	// Create an empty property tree object
    boost::property_tree::ptree pt;

    // Check that config file exists
    if (!boost::filesystem::exists(argv[1])) {
  		std::cout << "Cannot find config file: " << argv[1] << std::endl << std::endl;

  		exit(EXIT_FAILURE);
	}

    // Read config file to property tree
    read_xml(argv[1], pt, boost::property_tree::xml_parser::trim_whitespace);

    // Return property tree
    return pt;
}

#include <boost/algorithm/string/trim.hpp>

void dump(boost::property_tree::ptree const& pt, std::string const& indent = "") {
    for (auto& node : pt) {
        std::cout << indent << node.first;

        auto value = boost::trim_copy(node.second.get_value(""));

        if (!value.empty())
            std::cout << ": '" << value << "'";

        std::cout << "\n";

        dump(node.second, indent + "    ");
    }
}

experiment_parameters translate_experiment_parameters(boost::property_tree::ptree pt) {
	struct experiment_parameters params;

	for (auto& node : pt) {
        if (node.first.compare("number_of_threads") == 0) {
        	params.number_of_threads = node.second.get_value<uint32_t>();

        } else if (node.first.compare("inital_schedule") == 0) {
        	std::string sched = node.second.get_value<string>();

        	if (sched.compare("Static") == 0) {
        		params.inital_schedule = Static;

        	} else if (sched.compare("Dynamic_chunks") == 0) {
        		params.inital_schedule = Dynamic_chunks;

        	} else if (sched.compare("Tapered") == 0) {
        		params.inital_schedule = Tapered;

        	} else if (sched.compare("Auto") == 0) {
        		params.inital_schedule = Auto;
        	}

        } else if (node.first.compare("inital_chunk_size") == 0) {
        	params.inital_chunk_size = node.second.get_value<uint32_t>();

        } else if (node.first.compare("array_size") == 0) {
        	params.array_size = node.second.get_value<uint32_t>();

        } else if (node.first.compare("task_size_distribution") == 0) {

        } else {
	        std::cout << "Unrecognised node in config: " << node.first << std::endl << std::endl;

	        // exit(EXIT_FAILURE);
	    }
    }

	return params;
}

run_parameters translate_property_tree(boost::property_tree::ptree pt) {
	struct run_parameters params;
	struct experiment_parameters defaults;

    // dump(pt);

    for (auto& node : pt.get_child("parameters")) {
        if (node.first.compare("number_of_repeats") == 0) {
        	params.number_of_repeats = node.second.get_value<uint32_t>();

        } else if (node.first.compare("defaults") == 0) {
       		defaults = translate_experiment_parameters(node.second);

        } else if (node.first.compare("experiments") == 0) {
        	// params.experiments.push_back(translate_experiment_parameters(node.second));

        } else {
	        std::cout << "Unrecognised node in config: " << node.first << std::endl << std::endl;

	        exit(EXIT_FAILURE);
	    }
    }

	return params;
}

void print_run_parameters(struct run_parameters params) {
	std::cout << "Number of repeats: " << params.number_of_repeats << std::endl;
}

int main(int argc, char *argv[])
{
	// Create an empty property tree object
    boost::property_tree::ptree pt;

    // populate tree structure pt
	pt = read_config_file(argc, argv);

    // writing the unchanged ptree in file2.xml
    // write_xml("file2.xml", pt, std::locale(), boost::property_tree::xml_writer_make_settings<std::string>('\t', 1));

    // write_xml(std::cout, pt, boost::property_tree::xml_writer_make_settings<std::string>('\t', 1));

    struct run_parameters test;

    std::cout << test << std::endl;

    struct run_parameters test2 = translate_property_tree(pt);

    std::cout << test2;

	// for (uint32_t r = 0; r < REPEATS; r++) {
	// 	// Initialise metrics.
	//   	metrics_init(N_THREADS, string(argv[1]) + "_Repeat" + std::to_string(r) + ".csv");

	// 	uint32_t as = 200000;

	// 	// Experiment input vectors.
	//     deque<int> input1(as);
	//     deque<int> input2(as);

	//     // Generate data for vectors.
	//     for (uint32_t i = 0; i < as; i++) 
	//     {
	//         // input1[i] = (double) 1. / i;
	//         // input2[i] = (double) 1. / i;
	//         // input1[i] = (int) i + 1;
	//         // input2[i] = (int) i + 1;
	//         input1[i] = 10000;
	//         input2[i] = 87736;
	//     }

	//     // Output deque.
	//     deque<int> output(as);

	    

	//     uint32_t nthreads, tid, i;

	// 	uint32_t chunk = CHUNKSIZE;

	// 	omp_set_dynamic(0);     // Explicitly disable dynamic teams
	// 	omp_set_num_threads(N_THREADS); // Use 4 threads for all consecutive parallel regions

	// 	#pragma omp parallel shared(input1, input2, output, nthreads, chunk) private(i,tid) 
	// 	{
	// 	  tid = omp_get_thread_num();
	// 	  metrics_thread_start(tid);

	// 	  if (tid == 0) {
	// 	    nthreads = omp_get_num_threads();
	// 	    print("Number of threads = ", nthreads, "\n");
	// 	  }

	// 	  print("Thread ", tid, " starting...\n");

	// 	  #pragma omp for schedule(dynamic,chunk)
	// 	  for (i = 0; i < as; i++) {
	// 	  	metrics_starting_work(tid);
	// 	    output.at(i) = collatz(input1.at(i), input2);
	// 	    metrics_finishing_work(tid);

	// 	  }

	// 	  metrics_thread_finished(tid);
	//   	}  /* end of parallel section */

	// 	metrics_finalise();

	//   	metrics_calc();

	//   	metrics_exit();

	// 	for (i = 0; i < as; i++) 
	//     {
	//         //print(output.at(i));
	//     }
	// }
}
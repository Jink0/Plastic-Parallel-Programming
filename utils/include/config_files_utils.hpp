#ifndef CONFIG_FILES_UTILS_HPP
#define CONFIG_FILES_UTILS_HPP

#include <stdint.h>
#include <iostream>

#include <string>
#include <deque>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>



/*
 * Data structures
 */

// Possible schedules.
enum Schedule {Static, Dynamic_chunks, Tapered, Auto};

// Individual experiment parameters.
struct experiment_parameters {
	// Number of threads to use.
	uint32_t number_of_threads;

	// Schedule to start with.
	Schedule initial_schedule;

	// Chunk size to start with.
	uint32_t initial_chunk_size;

	// Array size to use.
	uint32_t array_size;

	// Relative distribution of tasks, e.g. 1 1 1 4 means last 1/4 of the array has tasks 4x as large.
	std::deque<uint32_t> task_size_distribution;
};

// Run of experiments parameters.
struct run_parameters {
	// Number of times to repeat experiments.
	uint32_t number_of_repeats = 0;

	// deque of individual experiment parameters.
	std::deque<experiment_parameters> experiments;
};



/*
 * Functions.
 */

// Dump property tree to ostream o, indented by given indent.
std::ostream& dump(std::ostream &o, boost::property_tree::ptree const &pt, std::string const &indent = "");

// Dump run parameters to ostream o, indented by given indent.
std::ostream& dump(std::ostream &o, const run_parameters &params, std::string const &indent = "");

// Dump experiment parameters to ostream o, indented by given indent.
std::ostream& dump(std::ostream &o, const experiment_parameters &params, std::string const &indent = "");



// Overloads ostream operator << for property trees.
std::ostream& operator<< (std::ostream &o, const boost::property_tree::ptree &pt);

// Overloads ostream operator << for struct run_parameters.
std::ostream& operator<< (std::ostream &o, const run_parameters &params);

// Overloads ostream operator << for struct experiment_parameters.
std::ostream& operator<< (std::ostream &o, const experiment_parameters &params);



// Attempts to read the given config file.
boost::property_tree::ptree read_config_file(int argc, char *argv[]);



// Translates the parameters of an individual experiment from a property tree to our experiment_parameters struct. 
// Writes the parameters it finds to the given experiment_parameters struct, possibly overwriting existing values.
void translate_experiment_parameters(boost::property_tree::ptree pt, struct experiment_parameters &params);

// Translates the parameters of an entire run from a property tree to our run_parameters struct. 
run_parameters translate_run_parameters(boost::property_tree::ptree pt);



#endif // CONFIG_FILES_UTILS_HPP
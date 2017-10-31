#ifndef CONFIG_FILES_UTILS_HPP
#define CONFIG_FILES_UTILS_HPP

#include <string>
#include <fstream>
#include <map>
#include <sstream>
#include <limits.h>
#include <unistd.h>
#include <utils.hpp>
#include <sys/stat.h>
#include <thread>



extern uint32_t num_runs, grid_size, num_stages, use_set_num_repeats;
extern std::vector<uint32_t> num_workers, num_iterations, set_pin_bool, strip_size;
extern std::vector<std::vector<uint32_t>> kernels, kernel_durations, kernel_repeats;
extern std::vector<std::vector<std::vector<uint32_t>>> pinnings;



// Returns the currrent working directory
std::string get_current_working_dir();

// Creates and moves into relevant working directory. Also copies given config file
void move_and_copy(std::string prog_dir_name, std::string config_filename);

// Returns a map of key-value pairsfrom the conifuration file
std::map<std::string, std::string> parse_config(std::string filename);

// Checks that the given iterators are not equal, prints relevant error message
void check_iterator(std::map<std::string, std::string>::iterator it, std::map<std::string, std::string>::iterator end);

// Reads config file, and returns s_exp_parameters
void read_config(std::map<std::string, std::string> config);

// Print experiment parameters
void print_params();

#endif // CONFIG_FILES_UTILS_HPP
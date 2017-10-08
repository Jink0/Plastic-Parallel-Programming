#define _REENTRANT

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/times.h>
#include <limits.h>

#include <iostream>
#include <vector>
#include <thread>

#include <map>

#include <sstream>

#include <utils.hpp>

void Worker(long long my_id, uint32_t stage);
void InitializeGrids();
void Barrier(uint32_t stage);

void moveAndCopy(std::string prog_dir_name, std::string config_filename);

// Returns a map of key-value pairsfrom the conifuration file
std::map<std::string, std::string> parse_config(std::string filename);

// Reads config file, and returns s_exp_parameters
void read_config(std::map<std::string, std::string> config);




struct tms buffer;        // Used for timing
clock_t start, end;

pthread_mutex_t barrier;  // Mutex semaphore for the barrier
pthread_cond_t go;        // Condition variable for leaving
uint32_t num_arrived = 0; // Count of the number who have arrived

uint32_t num_runs, grid_size, num_stages;

std::vector<uint32_t> num_workers, num_iterations, set_pin_bool, strip_size;
std::vector<std::vector<std::vector<uint32_t>>> pinnings;

std::vector<std::vector<double>> max_difference_global;
std::vector<std::vector<double>> grid1, grid2;

std::vector<std::string> options = {"Each worker has all cores", "Each worker has one corresponding core (max workers = num cores)", "Custom"};

// struct s_stage_parameters {
// 	uint32_t num_workers;
// 	uint32_t num_iterations;
// 	uint32_t set_pin_bool;
// 	uint32_t pinnings;
// };

// struct s_exp_parameters {
// 	uint32_t num_runs;
// 	uint32_t grid_size;
// 	uint32_t num_stages;

// 	std::vector<s_stage_parameters> stage_parameters;
// };

#include <algorithm>

int main(int argc, char *argv[]) {

	// Parse config
	std::map<std::string, std::string> config = parse_config(std::string(argv[1]));

	// Read config
	read_config(config);
	
	//  Calculate strip sizes
	for (uint32_t i = 0; i < num_stages; i++) {
		strip_size.push_back(grid_size / num_workers.at(i));
	}

	moveAndCopy("jacobi", argv[1]);

	// Attempt to open/create output file
	FILE *output_stream = fopen((char*) "output", "w");

	if (output_stream == NULL) {
        // If we couldn't open the file, throw an error
        perror("Error, metric could not open file");
        exit(EXIT_FAILURE);
    }

	// Print intro
	print("\nNumber of runs:    ", num_runs, "\n",
		  "Grid size:         ", grid_size, "\n",
		  "Number of stages:  ", num_stages, "\n");

	for (uint32_t i = 0; i < num_stages; i++) {
		print("\n\nStage ", i + 1, ":\n\n",
			  "Number of workers:    ", num_workers.at(i), "\n",
			  "Number of iterations: ", num_iterations.at(i), "\n",
			  "Set-pinning:          ", options.at(set_pin_bool.at(i)), "\n");

		if (set_pin_bool.at(i) == 2) {
			for (uint32_t j = 0; j < pinnings.at(i).size(); j++) {
		    	print("Worker ", j, ": ");

		    	for (uint32_t k = 0; k < pinnings.at(i).at(j).size(); k++) {
		    		print(pinnings.at(i).at(j).at(k), " ");
		    	}

		    	print("\n");
		    }
		}
	}

	fputs("\n\n", output_stream);

	// Calculate the max number of workers we will need
	uint32_t max_num_workers = *max_element(std::begin(num_workers), std::end(num_workers));

  	// Thread handles
	std::vector<std::thread> threads(max_num_workers);

	// WORK ON THIS!!!!!
	max_difference_global.resize(num_stages);

	for (uint32_t i = 0; i < num_stages; i++) {
		max_difference_global.at(i).resize(num_workers.at(i));
	}

	// Initialize mutex and condition variable
	pthread_mutex_init(&barrier, NULL);
	pthread_cond_init(&go, NULL);

	for (uint32_t r = 1; r < num_runs + 1; r++) {

		// Print arguments
		print("\n");

		InitializeGrids();

		// Record start time
		start = times(&buffer);

		for (uint32_t stage = 0; stage < num_stages; stage++) {
			// Create workers
			for (uint32_t i = 0; i < num_workers.at(stage); i++) {
				threads.at(i) = std::thread(Worker, i, stage);
			}

			// Join with workers
			for (uint32_t i = 0; i < num_workers.at(stage); i++) {
				threads.at(i).join();
			}
		}

		// Record end time
		end = times(&buffer);

		double max_diff = 0.0;

		// Find maximum difference
		for (uint32_t i = 0; i < num_workers.back(); i++) {
			if (max_diff < max_difference_global.back().at(i)) {
		  		max_diff = max_difference_global.back().at(i);
			}
		}

		print("\nRun ", r, "\n",
			  "Elapsed time: ", end - start, "\n");

		fputs(("\nRun: ," + std::to_string(r) + "\n" +
	           "Elapsed time: ," + std::to_string(end - start) + "\n").c_str(), output_stream);
	}
}

#include <sched.h>
#include <unistd.h>
#include <errno.h>

// Each Worker computes values in one strip of the grids. The main worker loop does two computations to avoid copying from one grid to the other.
void Worker(long long my_id, uint32_t stage) {

	// std::vector<uint32_t> core_ids;

	// if (set_pin_bool.at(stage) != 0) {
	// 	for (uint32_t i = 0; i < pinnings.at(stage); i++) {
	// 		core_ids.push_back(i);
	// 	}

	// } else {
	// 	core_ids.push_back(my_id);
	// }

	// print("\n\n", pinnings.size(),  "\n\n\n");

    

	force_affinity_set(pinnings.at(stage).at(my_id));








    // print("\n\n\n\n\nThread ", my_id, " cpu set size: ", check_affinity_set_size(), "\n\n");


	// Determine first and last rows of my strip of the grids
	uint32_t first = my_id * strip_size.at(stage) + 1;
	uint32_t last = first + strip_size.at(stage) - 1;

	for (uint32_t iter = 1; iter <= num_iterations.at(stage); iter++) {

		// Update my points
		for (uint32_t i = first; i <= last; i++) {
			for (uint32_t j = 1; j <= grid_size; j++) {
				grid2[i][j] = (grid1[i - 1][j] + grid1[i + 1][j] + grid1[i][j - 1] + grid1[i][j + 1]) * 0.25;
			}
		}

		Barrier(stage);

		// Update my points again
		for (uint32_t i = first; i <= last; i++) {
			for (uint32_t j = 1; j <= grid_size; j++) {
				grid1[i][j] = (grid2[i - 1][j] + grid2[i + 1][j] + grid2[i][j - 1] + grid2[i][j + 1]) * 0.25;
			}
		}

		Barrier(stage);

		// Simulate a convergence test.
		// Compute the maximum difference in my strip and set global variable
	  	double max_diff = 0.0;

		for (uint32_t i = first; i <= last; i++) {
			for (uint32_t j = 1; j <= grid_size; j++) {
				uint32_t temp = grid1[i][j]-grid2[i][j];

				if (temp < 0) {
					temp = -temp;
				}

				if (max_diff < temp) {
					max_diff = temp;
				}
			}
		}

		max_difference_global[stage][my_id] = max_diff;
	}
}



// Initialize the grids (grid1 and grid2), set boundaries to 1.0 and interior points to 0.0
void InitializeGrids() {

	grid1.resize(grid_size + 2);
	grid2.resize(grid_size + 2);
 
 	// Set all points to 0.0
	for (uint32_t i = 0; i <= grid_size + 1; i++) {
		grid1[i].resize(grid_size + 2);
		grid2[i].resize(grid_size + 2);

		for (uint32_t j = 0; j <= grid_size + 1; j++) {
		    grid1[i][j] = 0.0;
		    grid2[i][j] = 0.0;
		}
	}

	// Set edges to 1.0
	for (uint32_t i = 0; i <= grid_size + 1; i++) {
		grid1[i][0] = 1.0;
	    grid1[i][grid_size + 1] = 1.0;
	    grid2[i][0] = 1.0;
	    grid2[i][grid_size + 1] = 1.0;
	}

  	for (uint32_t j = 0; j <= grid_size + 1; j++) {
	    grid1[0][j] = 1.0;
	    grid2[0][j] = 1.0;
	    grid1[grid_size + 1][j] = 1.0;
	    grid2[grid_size + 1][j] = 1.0;
  	}
}



// Reusable counter barrier. Not sense reversing?
void Barrier(uint32_t stage) {

	pthread_mutex_lock(&barrier);

  	num_arrived++;

  	if (num_arrived == num_workers.at(stage)) {

    	num_arrived = 0;
    	pthread_cond_broadcast(&go);

  	} else {

    	pthread_cond_wait(&go, &barrier);
	}

  	pthread_mutex_unlock(&barrier);
}



#include <string>
#include <limits.h>
#include <unistd.h>

std::string get_working_dir() {

  char result[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);

  return std::string(result, (count > 0) ? count : 0);
}



#include <sys/stat.h>
#include <fstream>

void moveAndCopy(std::string prog_dir_name, std::string config_filename) {

    // Record filepath of the config file before we move so we can copy it later
    std::string working_dir = get_working_dir();

 	auto pos = working_dir.rfind('/');
	if (pos != std::string::npos) {
	    working_dir.erase(pos);
	}

    std::ifstream  src(working_dir + "/../" + config_filename, std::ios::binary);

    // Create runs directory if it doesn't exist
    mkdir("runs", S_IRWXU | S_IRWXG | S_IRWXO);

    // Move into the runs directory
    chdir("runs");

    // Create program directory if it doesn't exist
    mkdir("jacobi", S_IRWXU | S_IRWXG | S_IRWXO);

    // Move into the program directory
    chdir("jacobi");

    // Directory name to start at
    uint32_t i = 1;
    
    // Root directory word
    std::string root_dir_name = "run";

    struct stat sb;

    // Find what the next run number should be
    while (stat((root_dir_name + std::to_string(i)).c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
    	i++;
    }

	// Create run directory
    mkdir((root_dir_name + std::to_string(i)).c_str(), S_IRWXU | S_IRWXG | S_IRWXO);

	// Move into run directory
    chdir((root_dir_name + std::to_string(i)).c_str());

    print("\n\n\nHere: ", working_dir + "/../runs/jacobi/run" + std::to_string(i) + config_filename, "\n\n\n\n");

    // std::ofstream  dst(working_dir + config_filename, std::ios::binary);

    // dst << src.rdbuf();
}


#include <fstream>


// Returns a map of key-value pairsfrom the conifuration file
std::map<std::string, std::string> parse_config(std::string filename) {

    std::ifstream input(filename); // Input stream

    if (!input.is_open()) {
    	print("ERROR - Cannot open config file: ", filename);
    	exit(1);
    }

    std::map<std::string, std::string> output; // Output map of key-value pairs

    // While we have input to process;
    while(input) {

        std::string key;
        std::string value;

        std::getline(input, key, ':');    // Read up to the : delimiter, store in key
        std::getline(input, value, '\n'); // Read up to the newline, store in value

        std::string::size_type p1 = value.find_first_of("\""); // Find the first quote in the value
        std::string::size_type p2 = value.find_last_of("\"");  // Find the last quote in the value

        // Check if the found positions are all valid
        if(p1 != std::string::npos && p2 != std::string::npos && p2 > p1) {

            value = value.substr(p1 + 1, p2 - p1 - 1); // Take a substring of the part between the quotes

            output[key] = value.c_str();         // Store result
        }
    }

    input.close(); // Close the file stream

    return output; // Return the result
}



void check_iterator(std::map<std::string, std::string>::iterator it, std::map<std::string, std::string>::iterator end) {
	if (it == end) {
		print("Malformed config file!");
		exit(1);
	}
}


#include <sstream>
#include <fstream>
#include <iostream>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>


// Reads config file, and returns s_exp_parameters
void read_config(std::map<std::string, std::string> config) {

	std::map<std::string, std::string>::iterator it = config.find("num_runs");
	check_iterator(it, config.end());
	num_runs = atoi(it->second.c_str());

	it = config.find("grid_size");
	check_iterator(it, config.end());
	grid_size = atoi(it->second.c_str());

	it = config.find("num_stages");
	check_iterator(it, config.end());
	num_stages = atoi(it->second.c_str());

	for (uint32_t i = 0; i < num_stages; i++) {

		it = config.find("num_workers_" + std::to_string(i));
		check_iterator(it, config.end());
		num_workers.push_back(atoi(it->second.c_str()));

		it = config.find("num_iterations_" + std::to_string(i));
		check_iterator(it, config.end());
		num_iterations.push_back(atoi(it->second.c_str()));

		it = config.find("set_pin_bool_" + std::to_string(i));
		check_iterator(it, config.end());
		set_pin_bool.push_back(atoi(it->second.c_str()));

		std::vector<std::vector<uint32_t>> temp(num_workers.back());

		switch (set_pin_bool.back()) {
			case 0: {
				uint32_t hw_concurrency = std::thread::hardware_concurrency();

				for (uint32_t i = 0; i < num_workers.back(); i++) {

					for (uint32_t j = 0; j < hw_concurrency; j++) {
						
						temp.at(i).push_back(j);
					}
				}

				pinnings.push_back(temp);
			}

			case 1: {
				for (uint32_t i = 0; i < num_workers.back(); i++) {
						
					temp.at(i).push_back(i);
				}

				pinnings.push_back(temp);
			}

			case 2: {
				it = config.find("pinnings_" + std::to_string(i));
				check_iterator(it, config.end());

			    std::stringstream ss(it->second);

			    std::string token;
			    uint32_t worker = 0;

			    while (ss >> token) {

			    	uint32_t prev_dot = 0;
			    	uint32_t double_dot = 0;

			    	std::stringstream ss2(token);

			    	while(ss2.good()) {

					    std::string substr;
					    getline(ss2, substr, '.');

					    if (substr == "") {
					    	double_dot = 1;

					    } else {
					    	if (double_dot == 0) {
					    		temp.at(worker).push_back(std::stoi(substr));

					    	} else {
					    		for (uint32_t j = temp.at(worker).back() + 1; j <= std::stoi(substr); j++) {
					    			temp.at(worker).push_back(j);
					    		}
					    		double_dot = 0;
					    	}
					    }
					}
					worker++;
			    }

				pinnings.push_back(temp);
			}
		}
	}
}
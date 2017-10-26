#include "config_file_utils.hpp"



// Returns the currrent working directory
std::string get_current_working_dir() {

	char buff[FILENAME_MAX];

	char *ptr = getcwd(buff, FILENAME_MAX);

	if (ptr == NULL) {
		print("ERROR: Cannot get current working directory");
    	exit(1);
	}

	std::string current_working_dir(buff);

	return current_working_dir;
}



// Creates and moves into relevant working directory. Also copies given config file
void move_and_copy(std::string prog_dir_name, std::string config_filename) {

    // Open the config file before we move so we can copy it later
    std::ifstream src(get_current_working_dir() + "/" + config_filename, std::ios::binary);

    // print("\nConfig file found at:\n", get_current_working_dir() + "/" + config_filename, "\n");

    int res;

    // Create runs directory if it doesn't exist
    mkdir("runs", S_IRWXU | S_IRWXG | S_IRWXO);

    // Move into the runs directory
    res = chdir("runs");

    if (res != 0) {
    	print("ERROR: Cannot move into runs directory");
    	exit(1);
    }

    // Create program directory if it doesn't exist
    mkdir("jacobi", S_IRWXU | S_IRWXG | S_IRWXO);

    // Move into the program directory
    res = chdir("jacobi");

    if (res != 0) {
    	print("ERROR: Cannot move into jacobi directory");
    	exit(1);
    }

    // Directory name to start at
    uint32_t i = 1;
    
    // Root directory word
    std::string root_dir_name = "test";

    struct stat sb;

    // Find what the next test number should be
    while (stat((root_dir_name + std::to_string(i)).c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
    	i++;
    }

	// Create test directory
    mkdir((root_dir_name + std::to_string(i)).c_str(), S_IRWXU | S_IRWXG | S_IRWXO);

	// Move into test directory
    res = chdir((root_dir_name + std::to_string(i)).c_str());

    if (res != 0) {
    	print("ERROR: Cannot move into test", i, " directory");
    	exit(1);
    }

    // Create new config path
    std::string new_config_path = get_current_working_dir() + "/" + config_filename.substr(config_filename.find_last_of("/\\") + 1);

    // print("\nCopying config file to: \n", new_config_path, "\n");

    // Open new config file
    std::ofstream dst(new_config_path, std::ios::binary);

    // Copy data
    dst << src.rdbuf();

    // Close filestreams
    src.close();
    dst.close();
}



// Returns a map of key-value pairsfrom the conifuration file
std::map<std::string, std::string> parse_config(std::string filename) {

	// Input stream
    std::ifstream input(filename); 

    if (!input.is_open()) {
    	print("ERROR - Cannot open config file: ", filename);
    	exit(1);
    }

    // Output map of key-value pairs
    std::map<std::string, std::string> output; 

    // While we have input to process;
    while(input) {

        std::string key;
        std::string value;

        // Read up to the : delimiter, store in key
        std::getline(input, key, ':');    

        // Read up to the newline, store in value
        std::getline(input, value, '\n'); 

        // Find the first quote in the value
        std::string::size_type p1 = value.find_first_of("\""); 

        // Find the last quote in the value
        std::string::size_type p2 = value.find_last_of("\"");  

        // Check if the found positions are all valid
        if(p1 != std::string::npos && p2 != std::string::npos && p2 > p1) {

        	// Take a substring of the part between the quotes
            value = value.substr(p1 + 1, p2 - p1 - 1); 

            // Store result
            output[key] = value.c_str();         
        }
    }

    // Close the file stream
    input.close(); 

    // Return the result
    return output; 
}



// Checks that the given iterators are not equal, prints relevant error message
void check_iterator(std::map<std::string, std::string>::iterator it, std::map<std::string, std::string>::iterator end) {
	if (it == end) {
		print("Malformed config file!");
		exit(1);
	}
}



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
					    		for (uint32_t j = temp.at(worker).back() + 1; j <= std::abs(std::stoi(substr)); j++) {
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



void print_params() {
	
	// Print parameters
	print("\nNumber of runs:    ", num_runs, "\n",
		  "Grid size:         ", grid_size, "\n",
		  "Number of stages:  ", num_stages, "\n");

	// Used for printing set_pin_bool
	std::vector<std::string> options = {"Each worker has all cores", "Each worker has one corresponding core (max workers = num cores)", "Custom"};

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
}
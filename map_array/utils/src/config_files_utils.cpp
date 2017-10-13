#include "config_files_utils.hpp"

#include "utils.hpp"



// Dump property tree to ostream o, indented by given indent.
std::ostream& dump(std::ostream &o, boost::property_tree::ptree const &pt, std::string const &indent) {
    // For each node,
    for (auto& node : pt) {
        // Dump the first value.
        o << indent << node.first;

        // Retrieve the second value.
        auto value = node.second.get_value("");

        // If it's not empty, dump it.
        if (!value.empty()) {
            o << ": '" << value << "'";
        }

        // Newline.
        o << std::endl;

        // Recurse.
        dump(o, node.second, indent + "    ");
    }

    return o;
}

// Dump run parameters to ostream o, indented by given indent.
std::ostream& dump(std::ostream &o, const run_parameters &params, std::string const &indent) {
    o << indent << "Number of experiments: " << params.experiments.size() << std::endl <<
         indent << "Number of repeats: " << params.number_of_repeats << std::endl << std::endl;

    for (uint32_t i = 0; i < params.experiments.size(); i++) {
        o << indent << "Experiment " << i + 1 << ": " << std::endl << std::endl;

        dump(o, params.experiments.at(i), indent + "    ");
    }

    return o;
}

// Dump experiment parameters to ostream o, indented by given indent.
std::ostream& dump(std::ostream &o, const experiment_parameters &params, std::string const &indent) {
	// Print parameters to stream.
	o << indent << "Number of threads:      " << params.number_of_threads << std::endl <<
         indent << "Threading library:      " << threading_libraries[params.threading_lib] << std::endl <<
	     indent << "Initial schedule:       " << schedules[params.initial_schedule] << std::endl <<
	     indent << "Initial chunk size:     " << params.initial_chunk_size << std::endl <<
         indent << "User function:          " << user_functions[params.user_function] << std::endl <<
	     indent << "Array size:             " << params.array_size << std::endl <<
	     indent << "Task size distribution:";

	for (uint32_t i = 0; i < params.task_size_distribution.size(); i++) {
		o << " " << params.task_size_distribution.at(i);
	}

    o << std::endl << std::endl;

    return o;
}



// Overloads ostream operator << for property trees.
std::ostream& operator<< (std::ostream &o, const boost::property_tree::ptree &pt) {
  return dump(o, pt);
}

// Overloads ostream operator << for struct run_parameters.
std::ostream& operator<< (std::ostream &o, const run_parameters &params) {
  return dump(o, params);
}

// Overloads ostream operator << for struct experiment_parameters.
std::ostream& operator<< (std::ostream &o, const experiment_parameters &params) {
  return dump(o, params);
}



// Attempts to read the given config file.
boost::property_tree::ptree read_config_file(int argc, char *argv[]) {
	print("\n");

	// Check arguments
	if (argc != 2) {
		print("Incorrect command line arguments!\n\nusage: parallel_test config_file.xml\n\n");

		exit(EXIT_FAILURE);
	}
 
	// Create an empty property tree object
    boost::property_tree::ptree pt;

    // Check that config file exists
    if (!boost::filesystem::exists(argv[1])) {
  		print("Cannot find config file: ", argv[1], "\n\n");

  		exit(EXIT_FAILURE);
	}

    // Read config file to property tree
    read_xml(argv[1], pt, boost::property_tree::xml_parser::trim_whitespace);

    // Return property tree
    return pt;
}



// Translates the parameters of an individual experiment from a property tree to our experiment_parameters struct. 
// Writes the parameters it finds to the given experiment_parameters struct, possibly overwriting existing values.
void translate_experiment_parameters(boost::property_tree::ptree pt, struct experiment_parameters &params) {

	// For each node,
	for (auto& node : pt) {
		// Check against expected parameters.
        if (node.first.compare("number_of_threads") == 0) {
        	// Retrieve value.
        	params.number_of_threads = node.second.get_value<uint32_t>();

        } else if (node.first.compare("thread_pinnings") == 0) {

            // First clear existing pinnings.
            params.thread_pinnings.clear();

            // For each node,
            for (auto& child : node.second) {
                
                if (child.first.compare("value") == 0) {
                    // Retrieve value.
                    params.thread_pinnings.push_back(child.second.get_value<uint32_t>());

                // Catch unexpected nodes, ignoring "<xmlattr>" and "<xmlcomment>".
                } else if (child.first.compare("<xmlattr>") != 0 && child.first.compare("<xmlcomment>") != 0) {
                    print("Unrecognised node in config: ", child.first, "\n\n");

                    exit(EXIT_FAILURE);
                }
            }

        } else if (node.first.compare("threading_library") == 0) { 
            const std::string *t_lib = std::find(threading_libraries, threading_libraries + NUM_THREADING_LIBRARIES, node.second.get_value<std::string>());

            if (t_lib != std::end(threading_libraries)) {
                // Translate threading library string to enum and record it.
                params.threading_lib = (Threading_library) std::distance(threading_libraries, t_lib);

            } else {
                print("\nInvalid threading library: ", node.second.get_value<std::string>(), "\n\n");
                exit(EXIT_FAILURE);
            }

        } else if (node.first.compare("initial_schedule") == 0) {
            const std::string *sched = std::find(schedules, schedules + NUM_SCHEDULES, node.second.get_value<std::string>());

            if (sched != std::end(schedules)) {
                // Translate schedule string to enum and record it.
                params.initial_schedule = (Schedule) std::distance(schedules, sched);

            } else {
                print("\nInvalid schedule: ", node.second.get_value<std::string>(), "\n\n");
                exit(EXIT_FAILURE);
            }

        } else if (node.first.compare("initial_chunk_size") == 0) {
        	// Retrieve value.
        	params.initial_chunk_size = node.second.get_value<uint32_t>();

        } else if (node.first.compare("user_function") == 0) {  
            const std::string *u_func = std::find(user_functions, user_functions + NUM_USER_FUNCTIONS, node.second.get_value<std::string>());

            if (u_func != std::end(user_functions)) {
                // Translate user function string to enum and record it.
                params.user_function = (User_function) std::distance(user_functions, u_func);

            } else {
                print("\nInvalid user function: ", node.second.get_value<std::string>(), "\n\n");
                exit(EXIT_FAILURE);
            }

        } else if (node.first.compare("array_size") == 0) {
        	// Retrieve value.
        	params.array_size = node.second.get_value<uint32_t>();

        } else if (node.first.compare("task_size_distribution") == 0) {
        	// For each node,
        	for (auto& child : node.second) {
        		
        		if (child.first.compare("value") == 0) {
        			// Retrieve value.
        			params.task_size_distribution.push_back(child.second.get_value<uint32_t>());

        		// Catch unexpected nodes, ignoring "<xmlattr>".
        		} else if (child.first.compare("<xmlattr>") != 0) {
	        		print("Unrecognised node in config: ", child.first, "\n\n");

	        		exit(EXIT_FAILURE);
	    		}
        	}

        // Catch unexpected nodes.
        } else {
	        print("Unrecognised node in config: ", node.first, "\n\n");

	        exit(EXIT_FAILURE);
	    }
    }
}

// Translates the parameters of an entire run from a property tree to our run_parameters struct. 
run_parameters translate_run_parameters(boost::property_tree::ptree pt) {
	struct run_parameters params;
	struct experiment_parameters defaults;

	// For each node,
    for (auto& node : pt.get_child("parameters")) {
    	// Check against expected parameters.
        if (node.first.compare("number_of_repeats") == 0) {
        	// Retrieve value.
        	params.number_of_repeats = node.second.get_value<uint32_t>();

        } else if (node.first.compare("defaults") == 0) {
        	// Retrieve default experiment parameters.
       		translate_experiment_parameters(node.second, defaults);

            // Check for thread pinnings.
            if (defaults.thread_pinnings.size() == 0) {
                for (uint32_t i = 0; i < defaults.number_of_threads; i++) {
                    defaults.thread_pinnings.push_back(i);
                }
            }

            // Check integrity of config.
            if (defaults.number_of_threads != defaults.thread_pinnings.size()) {
                print("Malformed config file, number of threads (", defaults.number_of_threads, 
                      ") doesn't match number of thread pinnings (", defaults.thread_pinnings.size(),
                      ")\n\n");
                exit(1);
            }

        } else if (node.first.compare("experiments") == 0) {
        	// For each node (each experiment),
        	for (auto& child : node.second) {
        		// Start with the default parameters.
	        	struct experiment_parameters exp_params = defaults;

	        	// Overwrite with differing parameters.
	        	translate_experiment_parameters(child.second, exp_params);

                // Check integrity of config.
                if (exp_params.number_of_threads != exp_params.thread_pinnings.size()) {
                    print("Malformed config file, number of threads (", exp_params.number_of_threads, 
                          ") doesn't match number of thread pinnings (", exp_params.thread_pinnings.size(),
                          ")\n\n");
                    exit(1);
                }

	        	// Add to deque of experiment parameters.
	        	params.experiments.push_back(exp_params);
	        }

	    // Catch unexpected nodes.
        } else {
	        print("Unrecognised node in config: ", node.first, "\n\n");

	        exit(EXIT_FAILURE);
	    }
    }

	return params;
}


void moveAndCopy(std::string config_filename, std::string prog_dir_name) {
    // Record filepath of the config file before we move so we can copy it later.
    boost::filesystem::path c_p(boost::filesystem::current_path() /= config_filename);

    // Create runs directory if it doesn't exist.
    boost::filesystem::path r_p("runs");
    create_directory(r_p);

    // Move into the runs directory.
    boost::filesystem::current_path("runs");

    // Create program directory.
    boost::filesystem::path p_p(prog_dir_name.c_str());
    create_directory(p_p);

    // Move into the program directory.
    boost::filesystem::current_path(prog_dir_name.c_str());

    // Directory name to start at.
    int i = 1;
    
    // Root directory word.
    std::string root_dir_name = "run";

    // Find what the next run number should be.
    while (boost::filesystem::is_directory(root_dir_name + std::to_string(i).c_str())) {
        i++;
    }

    // Create our run directory.
    boost::filesystem::path rr_p(root_dir_name + std::to_string(i).c_str());
    create_directory(rr_p);

    // Move into our run directory.
    boost::filesystem::current_path(root_dir_name + std::to_string(i).c_str());

    // Copy our config file so we know what parameters were used.
    copy_file(c_p, boost::filesystem::current_path() /= c_p.filename());
}
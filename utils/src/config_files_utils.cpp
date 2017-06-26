#include "config_files_utils.hpp"



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
	o << indent << "Number of threads: " << params.number_of_threads << std::endl <<
	     indent << "Initial schedule: " << params.initial_schedule << std::endl <<
	     indent << "Initial chunk size: " << params.initial_chunk_size << std::endl <<
	     indent << "Array size: " << params.array_size << std::endl <<
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



// Translates the parameters of an individual experiment from a property tree to our experiment_parameters struct. 
// Writes the parameters it finds to the given experiment_parameters struct, possibly overwriting existing values.
void translate_experiment_parameters(boost::property_tree::ptree pt, struct experiment_parameters &params) {
	// For each node,
	for (auto& node : pt) {
		// Check against expected parameters.
        if (node.first.compare("number_of_threads") == 0) {
        	// Retrieve value.
        	params.number_of_threads = node.second.get_value<uint32_t>();

        } else if (node.first.compare("initial_schedule") == 0) {
        	std::string sched = node.second.get_value<std::string>();

        	// Compare value to record enum.
        	if (sched.compare("Static") == 0) {
        		params.initial_schedule = Static;

        	} else if (sched.compare("Dynamic_chunks") == 0) {
        		params.initial_schedule = Dynamic_chunks;

        	} else if (sched.compare("Tapered") == 0) {
        		params.initial_schedule = Tapered;

        	} else if (sched.compare("Auto") == 0) {
        		params.initial_schedule = Auto;
        	}

        } else if (node.first.compare("initial_chunk_size") == 0) {
        	// Retrieve value.
        	params.initial_chunk_size = node.second.get_value<uint32_t>();

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
	        		std::cout << "Unrecognised node in config: " << child.first << std::endl << std::endl;

	        		exit(EXIT_FAILURE);
	    		}
        	}

        // Catch unexpected nodes.
        } else {
	        std::cout << "Unrecognised node in config: " << node.first << std::endl << std::endl;

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

        } else if (node.first.compare("experiments") == 0) {
        	// For each node (each experiment),
        	for (auto& child : node.second) {
        		// Start with the default parameters.
	        	struct experiment_parameters exp_params = defaults;

	        	// Overwrite with differing parameters.
	        	translate_experiment_parameters(child.second, exp_params);

	        	// Add to deque of experiment parameters.
	        	params.experiments.push_back(exp_params);
	        }

	    // Catch unexpected nodes.
        } else {
	        std::cout << "Unrecognised node in config: " << node.first << std::endl << std::endl;

	        exit(EXIT_FAILURE);
	    }
    }

	return params;
}
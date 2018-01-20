#ifndef GENERAL_UTILS_HPP
#define GENERAL_UTILS_HPP

#include <iostream>
#include <mutex>
#include <cxxabi.h> // (gcc only)
#include <vector>


// Mutexed print function (Note - Templated function must be implemented in the header as the compiler
// must be able to see the implementation when linking in order to generate code for all specializations

// With nothing to add to the output stream, just return the stream
std::ostream& print_rec(std::ostream& outS);

// Recursively print arguments
template <class A0, class ...Args>
std::ostream& print_rec(std::ostream& outS, const A0& a0, const Args& ...args) {

    outS << a0;

    return print_rec(outS, args...);
}

// Case when given an output stream
template <class ...Args>
std::ostream& print(std::ostream& outS, const Args& ...args) {

    return print_rec(outS, args...);
}

// Retrieve the mutex. Must be done using this non-templated function, so that we have one mutex across all
// instantiations of the templated function
std::mutex& get_cout_mutex();

// The main print function defaults to the cout output stream
template <class ...Args>
std::ostream& print(const Args& ...args) {

    std::lock_guard<std::mutex> _(get_cout_mutex());

    return print(std::cout, args...);
}



// Returns the typename given to the template. Example usage: typename<int>() returns "int"
template<typename T>
std::string type_name() {

    // Variable to store status
    int status;

    // Mangled name of type T
    std::string mName = typeid(T).name();

    // Attempt to demangle
    char *dmName = abi::__cxa_demangle(mName.c_str(), NULL, NULL, &status);

    // If successful,
    if (status == 0) {

        // Record name
        mName = dmName;

        // Free memory
        free(dmName);
    }

    return mName;
}



// Forces affinity set of calling thread to given vector of ids
int force_affinity_set(std::vector<uint32_t> core_ids);

// Returns the number of cpus in calling thread's affinity set
uint32_t check_affinity_set_size();

// Thread safe version of rand()
long long rand_long_long(const long long& min, const long long& max);

void init_cross_proc_barrier();

void cross_proc_barrier();

void close_cross_proc_barrier();

void unlink_cross_proc_barrier();

#endif // GENERAL_UTILS_HPP
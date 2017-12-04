#include <general_utils.hpp>

#include <unistd.h>
#include <random>
#include <time.h>
#include <thread>



#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#if defined (__GCC__)
#define thread_local __thread
#endif



// Non-Templated print functions. Cannot be defined in header file like the templated functions

// With nothing to add to the output stream, just return the stream
std::ostream& print_rec(std::ostream& outS) {

    return outS;
}

// Retrieve the mutex. Must be done using this non-templated function, so that we have one mutex across all
// instantiations of the templated function
std::mutex& get_cout_mutex() {

    static std::mutex m;

    return m;
}



// Forces affinity set of calling thread to given vector of ids
int force_affinity_set(std::vector<uint32_t> core_ids) {

    if (core_ids.size() == 0) {
        print("ERROR: force_affinity_set called with an empty set of core_ids!");
        exit(1);
    }

    uint32_t num_cores = sysconf(_SC_NPROCESSORS_ONLN);

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    for (uint32_t i = 0; i < core_ids.size(); i++) {

        if (core_ids[i] >= num_cores) {
            return EINVAL;
        }

        CPU_SET(core_ids[i], &cpuset);
    }

    pthread_t current_thread = pthread_self();

    return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}



// Returns the number of cpus in calling thread's affinity set
uint32_t check_affinity_set_size() {

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    pthread_t current_thread = pthread_self();

    pthread_getaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);

    uint32_t count = 0;

    for (int i = 0; i < 32; i++) {

        if (CPU_ISSET(i, &cpuset)) {
            count++;
        }
    }

    return count;
}



// Thread safe version of rand()
long long rand_long_long(const long long& min, const long long& max) {
    static thread_local std::mt19937* gen = nullptr;

    if (!gen) {
        gen = new std::mt19937(clock() + std::hash<std::thread::id>()(std::this_thread::get_id()));
    }

    std::uniform_int_distribution<long long> dist(min, max);

    return dist(*gen);
}
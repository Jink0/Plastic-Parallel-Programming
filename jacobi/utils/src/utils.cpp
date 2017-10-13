#include <utils.hpp>

// #include <pthread.h>
#include <unistd.h>
#include <errno.h>

// #include <thread>

/*
 * Non-Templated print functions. Cannot be defined in header file like the templated functions.
 */

// With nothing to add to the output stream, just return the stream.
std::ostream& print_rec(std::ostream& outS)
{
    return outS;
}

// Retrieve the mutex. Must be done using this non-templated function, so that we have one mutex across all
// instantiations of the templated function.
std::mutex& get_cout_mutex()
{
    static std::mutex m;
    return m;
}



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



// void join_with_threads_legacy(std::deque<pthread_t> threads, uint32_t num_threads_to_join)
// {
//     int inital_threads_max_index = threads.size() - 1;

//     for (uint32_t i = 0; i < num_threads_to_join; i++)
//     {
//         int rc = pthread_join(threads.back(), NULL);

//         threads.pop_back();

//         if (rc)
//         {
//             // If we couldn't create a new thread, throw an error and exit.
//             print("[Main] ERROR; return code from pthread_join() is ", rc, "\n");
//             exit(-1);
//         }

//         print("[Main] Joined with thread ", inital_threads_max_index - i, "\n");
//     }
// }



// void join_with_threads(std::deque<std::thread> threads, uint32_t num_threads_to_join) {
//     uint32_t threads_size = threads.size();

//     if (num_threads_to_join == 0 || num_threads_to_join > threads_size) {
//         num_threads_to_join = threads_size;
//     }

//     for (uint32_t i = threads_size - 1; i >= 0; i--) {

//         threads.at(i).join();

//         threads.pop_back();

//         print("[Main] Joined with thread ", i, "\n");
//     }
// }
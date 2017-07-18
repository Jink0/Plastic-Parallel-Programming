#include <utils.hpp>

#include <pthread.h>
#include <unistd.h>
#include <errno.h>

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



int stick_this_thread_to_cpu(uint32_t core_id) {
    uint32_t num_cores = sysconf(_SC_NPROCESSORS_ONLN);

    if (core_id >= num_cores)
        return EINVAL;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_t current_thread = pthread_self();

    return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}



void join_with_threads(std::deque<pthread_t> threads, uint32_t num_threads_to_join)
{
    int inital_threads_max_index = threads.size() - 1;

    for (uint32_t i = 0; i < num_threads_to_join; i++)
    {
        int rc = pthread_join(threads.back(), NULL);

        threads.pop_back();

        if (rc)
        {
            // If we couldn't create a new thread, throw an error and exit.
            print("[Main] ERROR; return code from pthread_join() is ", rc, "\n");
            exit(-1);
        }

        print("[Main] Joined with thread ", inital_threads_max_index - i, "\n");
    }
}
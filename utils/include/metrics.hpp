#ifndef METRICS_HPP
#define METRICS_HPP

#include <string>
#include <deque>

#include <sys/time.h>



/*
 * Functions for calculating various metrics such as time in overhead/work etc. Each thread only modifies its own
 * array element, so no locking is required. It is assumed that negligible time occurs during the execution of each of
 * these functions.
 */

// Try to obtain the given mutex, calling metrics to measure time spent waiting.
#define MEASURED_MUTEX_LOCK(mutex_p, thread_id) \
    metrics_locking_mutex((thread_id)); \
    pthread_mutex_lock((mutex_p)); \
    metrics_locked_mutex((thread_id));



/*
 * Data structures
 */

// Structure to contain statistics for a single thread in a single run.
struct thread_time {
    struct timeval start_time, finish_time; 

    struct timeval last_start_time;         
    struct timeval overhead_start_time;
    struct timeval mutex_blocked_start_time; 
    struct timeval wait_blocked_start_time; 

    uint32_t cumul_work_millis          = 0;
    uint32_t cumul_overhead_millis      = 0; 
    uint32_t cumul_mutex_blocked_millis = 0;
    uint32_t cumul_wait_blocked_millis  = 0;

    uint32_t tasks_completed = 0;  
};

// Structure to contain statistics for a set of repeats.
struct repeat {
    struct timeval start_time;
    struct timeval finish_time; 

    std::deque<thread_time> thread_times;

    repeat(uint32_t num_threads) {

        // Assign start time and set number of threads.
        gettimeofday(&start_time, NULL);
        thread_times.resize(num_threads);
    }
};

// Global structure for recording metrics.
static struct {
    std::deque<repeat> repeats;

    FILE *output_stream;
} metrics;



/*
 * Functions
 */

// Initialise metrics for a set of repeats.
void metrics_start(std::string output_filename);

// Start metrics for a single repeat.
void metrics_repeat_start(uint32_t num_threads);

// Start metrics for a single thread.
void metrics_thread_start(uint32_t thread_id);

// Called when the thread is starting work.
void metrics_starting_work(uint32_t thread_id);

// Called when the thread has finished work.
void metrics_finishing_work(uint32_t thread_id);

// Finalise metrics for a single thread.
void metrics_thread_finished(uint32_t thread_id);

// Finalise metrics for a single repeat.
void metrics_repeat_finished();

// Calculate and print/record metrics. If we are still working, metrics can still be updated, which could lead to 
// inconsistent results. So metrics should not be fully trusted until all threads have finished.
void metrics_finished(void);



// // Call before thread tries to obtain a mutex.
// void metrics_obtaining_mutex(uint32_t thread_id);

// // Call just after thread has obtained mutex. Increments the cumulative blocking time by the time the thread has been 
// // blocked.
// void metrics_obtained_mutex(uint32_t thread_id);

// // Call when blocked by the master thread. Increment overhead, as last_start_time will be reset when unblocked.
// void metrics_blocked(uint32_t thread_id);

// // Call when unblocked by the master thread. Reset last_start_time.
// void metrics_unblocked(uint32_t thread_id);

#endif // METRICS_HPP
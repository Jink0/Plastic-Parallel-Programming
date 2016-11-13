#ifndef METRICS_H
#define METRICS_H



/*
 * Functions for calculating various metrics such as time in overhead/work etc. Each thread can only modify its own
 * array element, so no locking is required. It is assumed that negligible time occurs during the execution of each of
 * these functions, to avoid going insane
 */

// Try to obtain the given mutex, calling metrics to measure time spent waiting
#define MEASURED_MUTEX_LOCK(mutex_p, thread_id) \
    metrics_locking_mutex((thread_id)); \
    pthread_mutex_lock((mutex_p)); \
    metrics_locked_mutex((thread_id));

// Initialise metrics. If output_file is non-null, output metrics to the given file. Otherwise output to stdout
void metrics_init(int num_threads, std::string output_file);

// Initialise metrics for a single thread
void metrics_thread_start(int thread_id);

// Call when the thread is starting work
void metrics_starting_work(int thread_id);

// Call when the thread has finished work
void metrics_finishing_work(int thread_id);

// Call before thread tries to obtain a mutex
void metrics_pbtaining_mutex(int thread_id);

// Call just after thread has obtained mutex. Increments the cumulative blocking time by the time the thread has been 
// blocked
void metrics_obtained_mutex(int thread_id);

// Call when blocked by the master thread. Increment overhead, as last_start_time will be reset when unblocked
void metrics_blocked(int thread_id);

// Call when unblocked by the master thread. Reset last_start_time
void metrics_unblocked(int thread_id);

// Finalise metrics for a single thread
void metrics_thread_finished(int thread_id);

// Finalise metrics for entire program
void metrics_finalise(void);

// Calculate and print/record metrics. If we are still working, metrics can still be updated, which could lead to 
// inconsistent results. So metrics should not be fully trusted until all threads have finished
void metrics_calc(void);

// Cleanup
void metrics_exit(void);

#endif /* METRICS_H */
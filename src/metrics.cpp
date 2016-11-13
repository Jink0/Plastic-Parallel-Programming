#include <sys/time.h> // For recording the current time
#include <stdlib.h>
#include <stdio.h>

#include <sstream>
#include <string>

#include "metrics.h"



/*
 * Functions for calculating various metrics such as time in overhead/work etc. Each thread can only modify its own
 * array element, so no locking is required. It is assumed that negligible time occurs during the execution of each of
 * these functions, to avoid going insane.
 */



/*
 * Struct to store metrics for a single thread.
 */

typedef struct {
    struct timeval start_time, finish_time; // To calculate the total time the thread is running.

    struct timeval last_start_time;         // To calculate time spent on current work/overhead.
    struct timeval overhead_start_time;     // To calculate time spent in overhead.
    struct timeval mut_blocked_start_time;  // To calculate time spent blocked acquiring a mutex.
    struct timeval wait_blocked_start_time; // To calculate time spent blocked by master thread.

    long cumul_work_millis;                 // Cumulative amount of time spent doing work               (milliseconds).
    long cumul_overhead_millis;             // Cumulative amount of time spent in overhead              (milliseconds).
    long cumul_mut_blocked_millis;          // Cumulative amount of time spent blocked by mutex         (milliseconds).
    long cumul_wait_blocked_millis;         // Cumulative amount of time spent blocked by master thread (milliseconds).

    int tasks_completed;                    // Number of tasks this thread has completed.
} thread_times_t;



/* 
 * Struct to store all metrics
 */

static struct {
    struct timeval start_time;    // To calculate total runtime. Should be set by metrics_init at the start of execution.
    struct timeval finish_time;   // To calculate total runtime. After it is set, we should only calculate metrics etc.
    thread_times_t *thread_times; // Array to store metrics for each thread.

    int num_threads;              // Number of threads we used.

    FILE *output_stream;          // Output stream to write to. Used to write to the terminal or a file.
} metrics;



/*
 * Time difference, in microseconds, between struct timeval objects t1 and t2.
 */

#define TIME_DIFF_MICROS(t1, t2) \
    (1000 * ((t1).tv_sec - (t2).tv_sec) + ((t1).tv_usec - (t2).tv_usec) / 1000)



/*
 * Initialise metrics. If output_filename is non-null, output metrics to the given file. Otherwise output to stdout.
 */

void metrics_init(int num_threads, std::string output_filename)
{
    // Set overall start time.
    gettimeofday(&metrics.start_time, NULL);

    // Reserve space for our per-thread metrics.
    metrics.thread_times = new thread_times_t[num_threads];

    // Record number of threads.
    metrics.num_threads = num_threads;

    // Set output stream.
    metrics.output_stream = fopen((char*) output_filename.c_str(), "w");

    if (metrics.output_stream == NULL)
    {
        // If we couldn't open the file, throw an error.
        perror("Error, metric could not open file");
        exit(EXIT_FAILURE);
    }
}



/*
 * Initialise metrics for a single thread.
 */

void metrics_thread_start(int thread_id)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    // Set start time.
    metrics.thread_times[thread_id].start_time = now;

    // Set work start time.
    metrics.thread_times[thread_id].last_start_time = now;

    // Set cumulative vals to start at 0.
    metrics.thread_times[thread_id].cumul_work_millis         = 0;
    metrics.thread_times[thread_id].cumul_overhead_millis     = 0;
    metrics.thread_times[thread_id].cumul_mut_blocked_millis  = 0;
    metrics.thread_times[thread_id].cumul_wait_blocked_millis = 0;

    // Set task counter to 0;
    metrics.thread_times[thread_id].tasks_completed = 0;
}



/*
 * Call when the thread is starting work.
 */

void metrics_starting_work(int thread_id)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    // Overhead accrued is the time since we last finished doing work.
    metrics.thread_times[thread_id].cumul_overhead_millis += 
        TIME_DIFF_MICROS(now, metrics.thread_times[thread_id].last_start_time);

    // Reset to last work start time.
    metrics.thread_times[thread_id].last_start_time = now;
}



/*
 * Call when the thread has finished work.
 */

void metrics_finishing_work(int thread_id)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    // Work time accrued is the time since work started.
    metrics.thread_times[thread_id].cumul_work_millis +=
        TIME_DIFF_MICROS(now, metrics.thread_times[thread_id].last_start_time);


    // Reset to last overhead start time.
    metrics.thread_times[thread_id].last_start_time = now;

    // Record another task completed.
    metrics.thread_times[thread_id].tasks_completed++;
}



/*
 * Call before thread tries to obtain a mutex.
 */

void metrics_obtaining_mutex(int thread_id)
{
    // Set start time.
    gettimeofday(&metrics.thread_times[thread_id].mut_blocked_start_time, NULL);
}



/*
 * Call just after thread has obtained mutex. Increments the cumulative blocking time by the time the thread has been 
 * blocked.
 */

void metrics_obtained_mutex(int thread_id)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    // Increment cumulative blocking time by the time since we started obtaining the mutex.
    metrics.thread_times[thread_id].cumul_mut_blocked_millis +=
        TIME_DIFF_MICROS(now, metrics.thread_times[thread_id].mut_blocked_start_time);
}



/*
 * Call when blocked by the master thread. Increment overhead, as last_start_time will be reset when unblocked.
 */

void metrics_blocked(int thread_id)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    // Set start time for main thread blocking.
    metrics.thread_times[thread_id].wait_blocked_start_time = now;

    // Increment overhead time.
    metrics.thread_times[thread_id].cumul_overhead_millis +=
        TIME_DIFF_MICROS(now, metrics.thread_times[thread_id].last_start_time);
}



/*
 * Call when unblocked by the master thread. Reset last_start_time.
 */

void metrics_unblocked(int thread_id)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    // Increment time blocked by main thread.
    metrics.thread_times[thread_id].cumul_wait_blocked_millis +=
        TIME_DIFF_MICROS(now, metrics.thread_times[thread_id].wait_blocked_start_time);

    // Reset overhead time counter.
    metrics.thread_times[thread_id].overhead_start_time = now;
}



/*
 * Finalise metrics for a single thread.
 */

void metrics_thread_finished(int thread_id)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    metrics.thread_times[thread_id].cumul_overhead_millis +=
        TIME_DIFF_MICROS(now, metrics.thread_times[thread_id].last_start_time);

    metrics.thread_times[thread_id].finish_time = now;
}



/*
 * Finalise metrics for entire program.
 */

void metrics_finalise(void)
{
    gettimeofday(&metrics.finish_time, NULL);
}



/*
 * Calculate and print/record metrics. If we are still working, metrics can still be updated, which could lead to 
 * inconsistent results. So metrics should not be fully trusted until all threads have finished.
 */

void metrics_calc(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    // Calculate total program time.
    long program_time = TIME_DIFF_MICROS(now, metrics.start_time);

    thread_times_t time;

    std::ostringstream thread_numbers;
    std::ostringstream num_tasks_completed;

    std::ostringstream time_working;
    std::ostringstream time_in_overhead;
    std::ostringstream time_mutex_blocked;
    std::ostringstream time_blocked_by_master_thread;

    thread_numbers                << "Thread:";
    num_tasks_completed           << "Number of tasks completed:";

    time_working                  << "Time working:";
    time_in_overhead              << "Time in overhead:";
    time_mutex_blocked            << "Time mutex blocked:";
    time_blocked_by_master_thread << "Time blocked by master thread:";

    // Write metrics to output stream for each thread.
    for (int i = 0; i < metrics.num_threads; i++) 
    {
        time = metrics.thread_times[i];

        long total_time = time.cumul_work_millis + time.cumul_overhead_millis + time.cumul_wait_blocked_millis;

        if (total_time == 0) total_time = 1; // to prevent NANs.

        thread_numbers                << "\t" << i;
        num_tasks_completed           << "\t" << time.tasks_completed;

        time_working                  << "\t" << time.cumul_work_millis;
        time_in_overhead              << "\t" << time.cumul_overhead_millis;
        time_mutex_blocked            << "\t" << time.cumul_mut_blocked_millis;
        time_blocked_by_master_thread << "\t" << time.cumul_wait_blocked_millis;
    }

    thread_numbers                << "\n";
    num_tasks_completed           << "\n";

    time_working                  << "\n";
    time_in_overhead              << "\n";
    time_mutex_blocked            << "\n";
    time_blocked_by_master_thread << "\n";

    fprintf(metrics.output_stream, thread_numbers.str().c_str());
    fprintf(metrics.output_stream, num_tasks_completed.str().c_str());

    fprintf(metrics.output_stream, time_working.str().c_str());
    fprintf(metrics.output_stream, time_in_overhead.str().c_str());
    fprintf(metrics.output_stream, time_mutex_blocked.str().c_str());
    fprintf(metrics.output_stream, time_blocked_by_master_thread.str().c_str());
}



/*
 * Cleanup.
 */

void metrics_exit(void)
{
    // Free our malloc'ed memory.
    free(metrics.thread_times);
}
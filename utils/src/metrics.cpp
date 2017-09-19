#include "metrics.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <sstream>



#define TIME_DIFF_NANOS(t1, t2) \
    (1000 * ((t1).tv_sec - (t2).tv_sec) + ((t1).tv_nsec - (t2).tv_nsec) / 1000)

void timespec_diff(struct timespec *t1, struct timespec *t2, struct timespec *result) {

    result->tv_sec = std::abs(t2->tv_sec - t1->tv_sec);
    result->tv_nsec = std::abs(t2->tv_nsec - t1->tv_nsec);

    return;
}

void timespec_cumul_add(struct timespec *t1, struct timespec *t2) {

    t1->tv_sec += t2->tv_sec;

    long nanos = t1->tv_nsec + t2->tv_nsec;

    if (nanos > 999999999) {
        t1->tv_sec += nanos / 1000000000;
        t1->tv_nsec = nanos % 1000000000;

    } else {
        t1->tv_nsec = nanos;
    }

    return;
}



// Initialise metrics for a set of repeats.
void metrics_start(std::string output_filename) {

    // Attempt to open/create output file.
    metrics.output_stream = fopen((char*) output_filename.c_str(), "w");

    if (metrics.output_stream == NULL) {
        // If we couldn't open the file, throw an error.
        perror("Error, metric could not open file");
        exit(EXIT_FAILURE);
    }
}

// Start metrics for a single repeat.
void metrics_repeat_start(uint32_t num_threads) {

    // Create new repeat.
    struct repeat new_repeat(num_threads);

    // Add new repeat to metrics struct.
    metrics.repeats.push_back(new_repeat);

    // Record start time.
    clock_gettime(CLOCK_MONOTONIC, &metrics.repeats.back().start_time);
}

// Expand the number of threads by num_threads.
void metrics_expand_threads(uint32_t num_threads) {

    metrics.repeats.back().thread_times.resize(metrics.repeats.back().thread_times.size() + num_threads);
}

// Start metrics for a single thread.
void metrics_thread_start(uint32_t thread_id) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // Set start time.
    metrics.repeats.back().thread_times.at(thread_id).start_time = now;

    // Set work start time.
    metrics.repeats.back().thread_times.at(thread_id).last_start_time = now;
}

// Called when the thread is starting work.
void metrics_starting_work(uint32_t thread_id) {
    struct timespec now, diff;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // Overhead accrued is the time since we last finished doing work.
    // metrics.repeats.back().thread_times.at(thread_id).cumul_overhead_millis +=
    //     TIME_DIFF_NANOS(now, metrics.repeats.back().thread_times.at(thread_id).last_start_time);

    timespec_diff(&now, &metrics.repeats.back().thread_times.at(thread_id).last_start_time, &diff);

    timespec_cumul_add(&metrics.repeats.back().thread_times.at(thread_id).cumul_overhead_millis, &diff);






    // Reset to last work start time.
    metrics.repeats.back().thread_times.at(thread_id).last_start_time = now;
}

// Called when the thread has finished work.
void metrics_finishing_work(uint32_t thread_id) {
    struct timespec now, diff;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // Work time accrued is the time since work started.
    // metrics.repeats.back().thread_times.at(thread_id).cumul_work_millis +=
    //     TIME_DIFF_NANOS(now, metrics.repeats.back().thread_times.at(thread_id).last_start_time);

    timespec_diff(&now, &metrics.repeats.back().thread_times.at(thread_id).last_start_time, &diff);

    timespec_cumul_add(&metrics.repeats.back().thread_times.at(thread_id).cumul_work_millis, &diff);






    // Record another task completed.
    metrics.repeats.back().thread_times.at(thread_id).tasks_completed++;

    // Reset to last overhead start time.
    metrics.repeats.back().thread_times.at(thread_id).last_start_time = now;
}

// Finalise metrics for a single thread.
void metrics_thread_finished(uint32_t thread_id) {
    struct timespec now, diff;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // Overhead accrued is the time since we last finished doing work.
    // metrics.repeats.back().thread_times.at(thread_id).cumul_overhead_millis +=
    //     TIME_DIFF_NANOS(now, metrics.repeats.back().thread_times.at(thread_id).last_start_time);

    timespec_diff(&now, &metrics.repeats.back().thread_times.at(thread_id).last_start_time, &diff);

    timespec_cumul_add(&metrics.repeats.back().thread_times.at(thread_id).cumul_overhead_millis, &diff);






    // Reset to last work start time.
    metrics.repeats.back().thread_times.at(thread_id).finish_time = now;
}

// Finalise metrics for a single repeat.
void metrics_repeat_finished() {
    // Record finish time.
    clock_gettime(CLOCK_MONOTONIC, &metrics.repeats.back().finish_time);
}

// Calculate and print/record metrics. If we are still working, metrics can still be updated, which could lead to
// inconsistent results. So metrics should not be fully trusted until all threads have finished.
void metrics_finished(void) {
    for (uint32_t i = 0; i < metrics.repeats.size(); i++) {

        // Calculate total runtime of repeat.
        // long program_time = TIME_DIFF_NANOS(metrics.repeats.at(i).finish_time, metrics.repeats.at(i).start_time);

        struct timespec diff;

        timespec_diff(&metrics.repeats.at(i).start_time, &metrics.repeats.at(i).finish_time, &diff);

        long program_time = (diff.tv_sec * 1000) + (diff.tv_nsec / 1000000);






        // Initialise streams for each statistic.
        std::ostringstream repeat_number;
        std::ostringstream total_runtime;

        std::ostringstream thread_numbers;
        std::ostringstream num_tasks_completed;

        std::ostringstream time_working;
        std::ostringstream time_in_overhead;
        // std::ostringstream time_mutex_blocked;
        // std::ostringstream time_blocked_by_master_thread;

        repeat_number                 << "Repeat number:" << "\t" << i << "\n\n";
        total_runtime                 << "Total runtime:" << "\t" << program_time << "\n";

        thread_numbers                << "Thread:";
        num_tasks_completed           << "Number of tasks completed:";

        time_working                  << "Time working:";
        time_in_overhead              << "Time in overhead:";
        // time_mutex_blocked            << "Time mutex blocked:";
        // time_blocked_by_master_thread << "Time blocked by master thread:";

        // Add stats to streams.
        for (uint32_t j = 0; j < metrics.repeats.at(i).thread_times.size(); j++) {

            thread_time time = metrics.repeats.at(i).thread_times.at(j);

            thread_numbers                << "\t" << j;
            num_tasks_completed           << "\t" << time.tasks_completed;

            time_working                  << "\t" << (time.cumul_work_millis.tv_sec * 1000) + (time.cumul_work_millis.tv_nsec / 1000000);
            time_in_overhead              << "\t" << (time.cumul_overhead_millis.tv_sec * 1000) + (time.cumul_overhead_millis.tv_nsec / 1000000);
            // time_mutex_blocked            << "\t" << time.cumul_mutex_blocked_millis;
            // time_blocked_by_master_thread << "\t" << time.cumul_wait_blocked_millis;
        }

        // Add newlines.
        thread_numbers                << "\n";
        num_tasks_completed           << "\n";

        time_working                  << "\n";
        time_in_overhead              << "\n\n\n\n";
        // time_mutex_blocked            << "\n";
        // time_blocked_by_master_thread << "\n\n\n\n";

        // Print statistics to file.
        fputs(repeat_number.str().c_str(), metrics.output_stream);
        fputs(total_runtime.str().c_str(), metrics.output_stream);

        fputs(thread_numbers.str().c_str(), metrics.output_stream);
        fputs(num_tasks_completed.str().c_str(), metrics.output_stream);

        fputs(time_working.str().c_str(), metrics.output_stream);
        fputs(time_in_overhead.str().c_str(), metrics.output_stream);
        // fputs(time_mutex_blocked.str().c_str(), metrics.output_stream);
        // fputs(time_blocked_by_master_thread.str().c_str(), metrics.output_stream);
    }

    metrics.repeats.clear();
}



// // Call before thread tries to obtain a mutex.
// void metrics_obtaining_mutex(uint32_t thread_id) {
//     // Set start time.
//     clock_gettime(CLOCK_MONOTONIC, &metrics.repeats.back().thread_times.at(thread_id).mutex_blocked_start_time);
// }

// // Call just after thread has obtained mutex. Increments the cumulative blocking time by the time the thread has been
// // blocked.
// void metrics_obtained_mutex(uint32_t thread_id) {
//     struct timespec now;
//     clock_gettime(CLOCK_MONOTONIC, &now);

//     // Increment cumulative blocking time by the time since we started obtaining the mutex.
//     metrics.repeats.back().thread_times.at(thread_id).cumul_mutex_blocked_millis +=
//         TIME_DIFF_NANOS(now, metrics.repeats.back().thread_times.at(thread_id).mutex_blocked_start_time);





// }

// // Call when blocked by the master thread. Increment overhead, as last_start_time will be reset when unblocked.
// void metrics_blocked(uint32_t thread_id) {
//     struct timespec now;
//     clock_gettime(CLOCK_MONOTONIC, &now);

//     // Set start time for main thread blocking.
//     metrics.repeats.back().thread_times.at(thread_id).wait_blocked_start_time = now;

//     // Increment overhead time.
//     metrics.repeats.back().thread_times.at(thread_id).cumul_overhead_millis +=
//         TIME_DIFF_NANOS(now, metrics.repeats.back().thread_times.at(thread_id).last_start_time);





// }

// // Call when unblocked by the master thread. Reset last_start_time.
// void metrics_unblocked(uint32_t thread_id) {
//     struct timespec now;
//     clock_gettime(CLOCK_MONOTONIC, &now);

//     // Increment time blocked by main thread.
//     metrics.repeats.back().thread_times.at(thread_id).cumul_wait_blocked_millis +=
//         TIME_DIFF_NANOS(now, metrics.repeats.back().thread_times.at(thread_id).wait_blocked_start_time);






//     // Reset overhead time counter.
//     metrics.repeats.back().thread_times.at(thread_id).overhead_start_time = now;
// }
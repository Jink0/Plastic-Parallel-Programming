#ifndef COMMS_H
#define COMMS_H

#include <vector> // Vectors

using namespace std;

#define DEFAULT_PORT 6666

// Structures that are used to pass messages between the controller and applications.

/* Different possible schedules Static             - Give each thread equal portions.
                                Dynamic_chunks     - Threads dynamically retrive a chunk of the tasks when they can.
                                Dynamic_individual - Threads retrieve a single task when they can.
                                Tapered            - Chunk size starts off large and decreases to better handle load 
                                                     imbalance between iterations.
                                Auto               - Automatically try to figure out the best schedule. */
enum Schedule {Static, Dynamic_chunks, Dynamic_individual, Tapered, Auto};

struct settings {
	// How many threads to pin where.
    vector<uint32_t> thread_pinnings;

    // Schedule to use.
    Schedule schedule;
};

// Message headers
#define APP_SYN 0

#define CON_ACK 10
#define CON_SET 11

struct message {
	int header = -1;

	uint32_t pid;

	struct settings settings;
};

#endif
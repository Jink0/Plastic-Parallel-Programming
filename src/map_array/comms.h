#ifndef COMMS_H
#define COMMS_H

using namespace std;

#define DEFAULT_PORT 5555

#define MAX_NUM_THREADS 128

// Structures that are used to pass messages between the controller and applications.

/* Different possible schedules Static             - Give each thread equal portions.
                                Dynamic_chunks     - Threads dynamically retrive a chunk of the tasks when they can.
                                Dynamic_individual - Threads retrieve a single task when they can.
                                Tapered            - Chunk size starts off large and decreases to better handle load 
                                                     imbalance between iterations.
                                Auto               - Automatically try to figure out the best schedule. */
enum Schedule {Static, Dynamic_chunks, Dynamic_individual, Tapered, Auto};

string Schedules[5] {
	"Static",
	"Dynamic_chunks", 
	"Dynamic_individual", 
	"Tapered", 
	"Auto"
};

struct settings {
	// How many threads to pin where.
    int thread_pinnings[MAX_NUM_THREADS];

    // Schedule to use.
    Schedule schedule;
};

// Message headers
#define CON_DATA_REP 0
#define CON_UPDT_SND 1

#define APP_DATA_SND 10
#define APP_UPDT_REP 11

struct message {
	int header = -1;

	uint32_t pid;

	struct settings settings;
};

#endif
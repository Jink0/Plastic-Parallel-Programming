#ifndef COMMS_H
#define COMMS_H

#define DEFAULT_PORT 6666

// Structures that are used to pass messages between the controller and applications.

typedef struct settings_t {
	// How many threads to pin where.
    vector<uint32_t> thread_pinnings;

    // Schedule to use.
    Schedule schedule;
}

// Message headers
#define APP_SYN 0

#define CON_ACK 10
#define CON_SET 11

typedef struct message_t {
	int header = -1;
	union {
		uint32_t pid;
		settings_t settings;
	}
}

#endif
#ifndef COMMS_HPP
#define COMMS_HPP

#include <zmq.hpp> // ZMQ communication library.

using namespace std;
using namespace zmq;

#define DEFAULT_PORT 5555

#define MAX_NUM_THREADS 128

// Structures that are used to pass messages between the controller and applications.

/* Different possible schedules Static             - Give each thread equal portions.
                                Dynamic_chunks     - Threads dynamically retrieve a chunk of the tasks when they can.
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
#define CON_REP  0
#define CON_UPDT 1

#define APP_REG  10
#define APP_TERM 11

struct message {
	int header = -1;

	uint32_t pid;

	struct settings settings;
};

// Receive 0MQ message from socket and convert into a message struct.
/*static struct message m_recv(socket_t &socket) {

    message_t msg;
    socket.recv(&msg);

    return *(static_cast<struct message*>(msg.data()));
}*/

// Receive 0MQ message from socket and convert into a message struct.
static struct message m_no_block_recv(socket_t &socket) {

    message_t msg;
    int rc = socket.recv(&msg, ZMQ_NOBLOCK);

    if (rc == 0)
    {
        struct message blank;

        blank.header = -1;

        return blank;
    }

    return *(static_cast<struct message*>(msg.data()));
}

// Send a message struct.
static bool m_send(socket_t &socket, const struct message &to_send) {

    message_t msg(sizeof(to_send));
    memcpy(msg.data(), &to_send, sizeof(to_send));

    bool rc = socket.send(msg);

    return rc;
}

//  Send a message struct as multipart non-terminal.
/*static bool m_sendmore(socket_t & socket, const struct message &to_send) {

    message_t msg(sizeof(to_send));
    memcpy(msg.data(), &to_send, sizeof(to_send));

    bool rc = socket.send(msg, ZMQ_SNDMORE);

    return (rc);
}*/

#endif // COMMS_HPP
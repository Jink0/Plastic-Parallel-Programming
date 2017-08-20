#ifndef COMMS_HPP
#define COMMS_HPP

#include <zmq.hpp> // ZMQ communication library.

#include "config_files_utils.hpp"

using namespace std;
using namespace zmq;

#define DEFAULT_PORT 5555

// Structures that are used to pass messages between the controller and applications.



// #include <experimental/optional>

// Data structure for a parameters message.
struct parameters_msg {

    // Number of threads to use.
    uint32_t number_of_threads;

    // Schedule to use.
    Schedule schedule;

    // Chunk size to use.
    uint32_t chunk_size;

    // Array size to use.
    uint32_t array_size;

    // Relative distribution of tasks, e.g. 1 1 1 4 means last 1/4 of the array has tasks 4x as large.
    // std::experimental::optional<std::deque<uint32_t>> task_size_distribution;

    // User function to use.
    // std::experimental::optional<User_function> user_function;

    // Threading library to use.
    // std::experimental::optional<Threading_library> threading_lib;
};



// Since thread pinnings is of variable size, we need a separate message to send it, so we can first tell the receiver what size to expect.
struct thread_pinnings_msg {
    // List of where to pin threads.
    std::deque<uint32_t> thread_pinnings;
};



// Message headers
#define INVALID 0

#define APP_P_REQ   1
#define APP_INIT    2
#define APP_INIT_TP 3
#define APP_TERM    4

#define CON_P_REP   11
#define CON_UPDT    12
#define CON_UPDT_TP 13



struct message {
    uint32_t header = INVALID;

    uint32_t pid;

    struct parameters_msg parameters;

    std::deque<uint32_t> thread_pinnings;
};



// Receive uint32_t from socket and convert into a message struct.
static uint32_t uint32_t_recv(socket_t &socket) {

    message_t msg;
    socket.recv(&msg);

    return *(static_cast<uint32_t*>(msg.data()));
}



// Receive 0MQ message from socket and convert into a message struct.
static struct message m_recv(socket_t &socket) {

    message_t msg;
    socket.recv(&msg);

    return *(static_cast<struct message*>(msg.data()));
}



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



// Send a uint32_t.
static bool uint32_t_send(socket_t &socket, const uint32_t message) {

    message_t msg(sizeof(message));
    memcpy(msg.data(), &message, sizeof(message));

    bool rc = socket.send(msg, ZMQ_NOBLOCK);

    return rc;
}



// Send a message struct.
static bool m_send(socket_t &socket, const struct message &to_send) {

    message_t msg(sizeof(to_send));
    memcpy(msg.data(), &to_send, sizeof(to_send));

    bool rc = socket.send(msg, ZMQ_NOBLOCK);

    return rc;
}



// Send a message struct.
static bool m_no_block_send(socket_t &socket, const struct message &to_send) {

    message_t msg(sizeof(to_send));
    memcpy(msg.data(), &to_send, sizeof(to_send));

    bool rc = socket.send(msg);

    return rc;
}



#endif // COMMS_HPP
#ifndef COMMS_HPP
#define COMMS_HPP

#include <zmq.hpp> // ZMQ communication library.

#include "config_files_utils.hpp"

// #include <experimental/optional>



#define DEFAULT_PORT 5555

// Message headers
#define INVALID 0

#define APP_P_REQ   1
#define APP_INIT    2
#define APP_TERM    3

#define CON_UPDT    11



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



struct message {
    uint32_t header = INVALID;

    uint32_t pid;

    struct parameters_msg parameters;
};



enum send_or_receive {SEND = 0, RECEIVE = 1};



void message_printout(struct message mess, send_or_receive toggle) {

    std::stringstream output;

    switch (toggle) {
        case SEND: {
            switch (mess.header) {
                case APP_P_REQ: {
                    output << "Sending port request with my PID: " << mess.pid;
                    break;
                }

                case APP_INIT: {
                    output << "Sending app initialization with my PID: " << mess.pid;
                    break;
                }

                case APP_TERM: {
                    output << "Sending app termination with my PID: " << mess.pid;
                    break;
                }

                case CON_UPDT: {
                    output << "Sending update message to PID: " << mess.pid;
                    break;
                }

                default: {
                    output << "Unrecognised message header: " << mess.header;

                    exit(EXIT_FAILURE);
                }
        
                break;
            }
        }

        case RECEIVE: {
            switch (mess.header) {
                case APP_P_REQ: {
                    output << "Received port request from PID: " << mess.pid;
                    break;
                }

                case APP_INIT: {
                    output << "Received app initialization from PID: " << mess.pid;
                    break;
                }

                case APP_TERM: {
                    output << "Received app termination from PID: " << mess.pid;
                    break;
                }

                case CON_UPDT: {
                    output << "Received updates from controller";
                    break;
                }

                default: {
                    output << "Unrecognised message header: " << mess.header;

                    exit(EXIT_FAILURE);
                }
        
                break;
            }
        }
    }

    output << std::endl << std::endl;
    

    // std::cout << mess.pid << std::endl;
    // std::cout << "   With schedule:       " << schedules[mess.parameters.schedule] << std::endl;
    // std::cout << "   And thread pinnings: ";

    // uint32_t i_w = 0;

    // while (mess.parameters.thread_pinnings[i_w] != -1) {
    //     std::cout << mess.parameters.thread_pinnings[i_w] << ' ';
    //     i_w++;
    // }

    std::cout << output.str();
}



// Receive uint32_t from socket and convert into a message struct.
static uint32_t uint32_t_recv(zmq::socket_t &socket) {

    zmq::message_t msg;
    socket.recv(&msg);

    return *(static_cast<uint32_t*>(msg.data()));
}



// Receive message from socket and convert into a message struct.
static struct message m_recv(zmq::socket_t &socket) {

    zmq::message_t msg;
    socket.recv(&msg);

    struct message data = *(static_cast<struct message*>(msg.data()));

    // message_printout(data, RECEIVE);

    return data;
}



// Send a uint32_t.
static bool uint32_t_send(zmq::socket_t &socket, const uint32_t message) {

    zmq::message_t msg(sizeof(message));
    memcpy(msg.data(), &message, sizeof(message));

    bool rc = socket.send(msg);

    return rc;
}



// Send a message struct.
static bool m_send(zmq::socket_t &socket, const struct message &to_send) {

    zmq::message_t msg(sizeof(to_send));
    memcpy(msg.data(), &to_send, sizeof(to_send));

    bool rc = socket.send(msg);

    message_printout(to_send, SEND);

    return rc;
}



#endif // COMMS_HPP
#include <string>
#include <iostream>

#include <unistd.h>

#include <boost/thread.hpp> // boost::thread::hardware_concurrency();

#include "comms.hpp"



#define MAX_CLIENTS 4



int main () {
    // Prepare our context and sockets.
    zmq::context_t context(1);

    zmq::socket_t port_req_socket(context, ZMQ_REP);
    zmq::socket_t client1(context, ZMQ_PAIR);
    zmq::socket_t client2(context, ZMQ_PAIR);
    zmq::socket_t client3(context, ZMQ_PAIR);
    zmq::socket_t client4(context, ZMQ_PAIR);

    // Bind sockets to ports.
    port_req_socket.bind("tcp://*:" + std::to_string(DEFAULT_PORT));
    client1.bind("tcp://*:" + std::to_string(DEFAULT_PORT + 1));
    client2.bind("tcp://*:" + std::to_string(DEFAULT_PORT + 2));
    client3.bind("tcp://*:" + std::to_string(DEFAULT_PORT + 3));
    client4.bind("tcp://*:" + std::to_string(DEFAULT_PORT + 4));

    // Setup the next port to allocate.
    uint32_t next_port = DEFAULT_PORT + 1;

    // Initialize poll set.
    zmq::pollitem_t items[MAX_CLIENTS + 1] = {
        { port_req_socket, 0, ZMQ_POLLIN, 0 },
        { client1, 0, ZMQ_POLLIN, 0 },
        { client2, 0, ZMQ_POLLIN, 0 },
        { client3, 0, ZMQ_POLLIN, 0 },
        { client4, 0, ZMQ_POLLIN, 0 }
    };

    // Continually poll for messages from all sockets.
    while (true) {
        zmq::message_t message;
        zmq::poll(&items[0], MAX_CLIENTS + 1, -1);
        
        // If we have a port request,
        if (items[0].revents & ZMQ_POLLIN) {

            // Receive message.
            struct message data = m_recv(port_req_socket);

            if (data.header == APP_P_REQ) {

                // Reply with port allocation.
                uint32_t_send(port_req_socket, next_port);

                next_port++;
            }
        }

        // If we have a message from client 1,
        if (items[1].revents & ZMQ_POLLIN) {

            // Receive message.
            struct message data = m_recv(client1);

            // Switch on message type.
            switch (data.header) {
                case APP_INIT: {

                    std::deque<uint32_t> thread_pinnings;

                    for (uint32_t i = 0; i < data.parameters.number_of_threads; i++) {
                        thread_pinnings.push_back(uint32_t_recv(client1));
                    }

                    struct message update;

                    update.header              = CON_UPDT;
                    update.pid                 = data.pid;
                    // update.parameters          = data.parameters;
                    update.parameters.schedule = Dynamic_chunks;
                    update.parameters.number_of_threads = 4;
                    update.parameters.chunk_size = 10000;
                    update.parameters.array_size = 0;

                    // Send update to client.
                    m_send(client1, update);

                    for (uint32_t i = 0; i < update.parameters.number_of_threads; i++) {
                        uint32_t_send(client1, i);
                    }

                    break;
                }

                case APP_TERM: {
                    // Remove application from active list.

                    break;
                }
            }
        }
    }

    return 0;
}

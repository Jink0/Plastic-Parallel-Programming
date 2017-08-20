#include <string>
#include <iostream>

#include <unistd.h>

#include <boost/thread.hpp> // boost::thread::hardware_concurrency();

#include "comms.hpp"

using namespace std;
using namespace zmq;

#define MAX_CLIENTS 4

enum Message_type {
    Sending, Receving
};

void message_printout(Message_type type, struct message mess) {
    switch (type) {
    case Sending:
        cout << "Sending ACK to PID:     ";
        break;

    case Receving:
        cout << "Received SYN from PID:  ";
        break;

    default:
        break;
    }

    cout << mess.pid << endl;
    cout << "   With schedule:       " << schedules[mess.parameters.schedule] << endl;
    cout << "   And thread pinnings: ";

    uint32_t i_w = 0;

    // while (mess.parameters.thread_pinnings[i_w] != -1) {
    //     cout << mess.parameters.thread_pinnings[i_w] << ' ';
    //     i_w++;
    // }

    cout << endl << endl;
}

int main () {
    // Prepare our context and sockets.
    context_t context(1);

    socket_t port_req_socket(context, ZMQ_REP);
    socket_t client1(context, ZMQ_PAIR);
    socket_t client2(context, ZMQ_PAIR);
    socket_t client3(context, ZMQ_PAIR);
    socket_t client4(context, ZMQ_PAIR);

    port_req_socket.bind("tcp://*:5555");
    client1.bind("tcp://*:5556");
    client2.bind("tcp://*:5557");
    client3.bind("tcp://*:5558");
    client4.bind("tcp://*:5559");

    uint32_t next_port = 5556;

    // Initialize poll set
    zmq::pollitem_t items[MAX_CLIENTS + 1] = {
        { port_req_socket, 0, ZMQ_POLLIN, 0 },
        { client1, 0, ZMQ_POLLIN, 0 },
        { client2, 0, ZMQ_POLLIN, 0 },
        { client3, 0, ZMQ_POLLIN, 0 },
        { client4, 0, ZMQ_POLLIN, 0 }
    };

    // Process messages from all sockets
    while (true) {
        zmq::message_t message;
        zmq::poll(&items[0], MAX_CLIENTS + 1, -1);
        
        if (items[0].revents & ZMQ_POLLIN) {

            struct message data = m_recv(port_req_socket);

            std::cout << "Received port request from: " + std::to_string(data.pid) + "\n";

            uint32_t_send(port_req_socket, next_port);

            next_port++;
        }

        if (items[1].revents & ZMQ_POLLIN) {

            struct message data = m_recv(client1);

            std::cout << "Connected with: " + std::to_string(data.pid) + " on port: " + std::to_string(5556) + "\n";

            struct message rep;

            rep.header              = CON_UPDT;
            rep.pid                 = data.pid;
            rep.parameters          = data.parameters;
            rep.parameters.schedule = Static;

            // fill_n(rep.parameters.thread_pinnings, MAX_NUM_THREADS, -1);

            // for (uint32_t i = 0; i < 4; i++)
            // {
            //     rep.parameters.thread_pinnings[i] = i;
            // }

            // Send reply to client.
            m_send(client1, rep);
        }

        if (items[2].revents & ZMQ_POLLIN) {

            struct message data = m_recv(client2);

            std::cout << "Connected with: " + std::to_string(data.pid) + " on port: " + std::to_string(5557) + "\n";

            struct message rep;

            rep.header              = CON_UPDT;
            rep.pid                 = data.pid;
            rep.parameters          = data.parameters;
            rep.parameters.schedule = Static;

            // fill_n(rep.parameters.thread_pinnings, MAX_NUM_THREADS, -1);

            // for (uint32_t i = 0; i < 4; i++)
            // {
            //     rep.parameters.thread_pinnings[i] = i;
            // }

            // Send reply to client.
            m_send(client2, rep);
        }

        if (items[3].revents & ZMQ_POLLIN) {

            struct message data = m_recv(client3);

            std::cout << "Connected with: " + std::to_string(data.pid) + " on port: " + std::to_string(5558) + "\n";

            struct message rep;

            rep.header              = CON_UPDT;
            rep.pid                 = data.pid;
            rep.parameters          = data.parameters;
            rep.parameters.schedule = Static;

            // fill_n(rep.parameters.thread_pinnings, MAX_NUM_THREADS, -1);

            // for (uint32_t i = 0; i < 4; i++)
            // {
            //     rep.parameters.thread_pinnings[i] = i;
            // }

            // Send reply to client.
            m_send(client3, rep);
        }

        if (items[4].revents & ZMQ_POLLIN) {

            struct message data = m_recv(client4);

            std::cout << "Connected with: " + std::to_string(data.pid) + " on port: " + std::to_string(5559) + "\n";

            struct message rep;

            rep.header              = CON_UPDT;
            rep.pid                 = data.pid;
            rep.parameters          = data.parameters;
            rep.parameters.schedule = Static;

            // fill_n(rep.parameters.thread_pinnings, MAX_NUM_THREADS, -1);

            // for (uint32_t i = 0; i < 4; i++)
            // {
            //     rep.parameters.thread_pinnings[i] = i;
            // }

            // Send reply to client.
            m_send(client4, rep);
        }
    }

        // Wait for next message from client
        // struct message data = m_recv(socket);

        // switch (data.header) {
            // case APP_C_REQ: {

            //     // uint32_t_send(socket, next_port);

            //     next_port++;

            //     break;
            // }

            // case APP_INIT: {
            //         message_printout(Receving, data);

            //         struct message rep;

            //         rep.header              = CON_UPDT;
            //         rep.pid                 = data.pid;
            //         rep.parameters.schedule = Static;

            //         // fill_n(rep.parameters.thread_pinnings, MAX_NUM_THREADS, -1);

            //         // for (uint32_t i = 0; i < 4; i++)
            //         // {
            //         //     rep.parameters.thread_pinnings[i] = i;
            //         // }

            //         // Send reply to client.
            //         m_send(socket, rep);

            //         message_printout(Sending, rep);

            //         //sleep(30);

            //         // m_send(socket, rep);

            //         // message_printout(Sending, rep);

            //         break;
            //     }

            // case APP_TERM: {
            //         cout << "PID: " << data.pid << " terminated" << endl << endl;

            //         // Remove application from active list.

            //         break;
            //     }
        // }

    return 0;
}

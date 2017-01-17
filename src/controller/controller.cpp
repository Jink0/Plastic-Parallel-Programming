//
//  Hello World server in C++
//  Binds REP socket to tcp://*:5555
//  Expects "Hello" from client, replies with "World"
//
#include <zmq.hpp>
#include <string>
#include <iostream>

#include <unistd.h>

using namespace std;
using namespace zmq;

int main () {
    //  Prepare our context and socket
    context_t context (1);
    socket_t socket (context, ZMQ_REP);
    socket.bind ("tcp://*:5555");

    uint32_t socket_start_number = 5556;

    while (true) {
        message_t msg;

        //  Wait for next message from client
        socket.recv (&msg);

        uint32_t app_pid = *(static_cast<uint32_t*>(msg.data()));

        cout << "Received socket request from PID " << app_pid << endl;

        //  Do some 'work'
        sleep(2);

        //  Send reply back to client.
        message_t reply (sizeof(socket_start_number));
        memcpy(reply.data(), &socket_start_number, sizeof(socket_start_number));
        cout << "Sending PID " << app_pid << " socket number " << socket_start_number << "..." << endl;
        socket.send(reply);

        socket_start_number++;
    }
    return 0;
}

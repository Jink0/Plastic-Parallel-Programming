//
//  Hello World server in C++
//  Binds REP socket to tcp://*:5555
//  Expects "Hello" from client, replies with "World"
//
#include <zmq.hpp>
#include <string>
#include <iostream>

#include <unistd.h>

#include <boost/thread.hpp> // boost::thread::hardware_concurrency();

#include <comms.h>

using namespace std;
using namespace zmq;

int main () {
    //  Prepare our context and socket
    context_t context (1);
    socket_t socket (context, ZMQ_REP);
    socket.bind ("tcp://*:5555");

    while (true) {
        message_t msg;

        //  Wait for next message from client
        socket.recv (&msg);

        struct message syn = *(static_cast<struct message*>(msg.data()));

        switch (syn.header) {
            case APP_SYN:
                cout << "Received SYN from PID " << syn.pid << endl;

                struct message ack;

                ack.header = CON_ACK;
                ack.settings.schedule = Tapered;

                uint32_t num_threads = boost::thread::hardware_concurrency();

                for (uint32_t i = 0; i < num_threads; i++)
                {
                    ack.settings.thread_pinnings.push_back(i);
                }

                //  Send ACK back to client.
                message_t reply (sizeof(ack));
                memcpy(reply.data(), &ack, sizeof(ack));
                cout << "Sending ACK to PID    " << syn.pid << endl;
                socket.send(reply);

                break;
        }
    }
    return 0;
}

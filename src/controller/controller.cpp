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

enum Message_type {
    Sending, Receving
};

void print_messages(Message_type type, struct message mess) {
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
    cout << "   With schedule:       " << Schedules[mess.settings.schedule] << endl;
    cout << "   And thread pinnings: ";

    uint32_t i_w = 0;

    while (mess.settings.thread_pinnings[i_w] != -1) {
        cout << mess.settings.thread_pinnings[i_w] << ' ';
        i_w++;
    }

    cout << endl << endl;
}

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
                print_messages(Receving, syn);
                    
                struct message ack;

                ack.header            = CON_ACK;
                ack.pid               = syn.pid;
                ack.settings.schedule = Static;

                fill_n(ack.settings.thread_pinnings, MAX_NUM_THREADS, -1);

                uint32_t num_threads = boost::thread::hardware_concurrency();

                for (uint32_t i = 0; i < num_threads; i++)
                {
                    ack.settings.thread_pinnings[i] = i;
                }

                //  Send ACK back to client.
                message_t reply (sizeof(ack));
                memcpy(reply.data(), &ack, sizeof(ack));

                print_messages(Sending, ack);

                socket.send(reply);

                break;
        }
    }
    return 0;
}

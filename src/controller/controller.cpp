//
//  Hello World server in C++
//  Binds REP socket to tcp://*:5555
//  Expects "Hello" from client, replies with "World"
//
//#include <zmq.hpp>
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
    socket_t  socket (context, ZMQ_PAIR);

    socket.bind("tcp://*:5555");

    while (true) {

        // Wait for next message from client
        struct message data = m_recv(socket);

        switch (data.header) {
            case APP_REG:
                message_printout(Receving, data);
                    
                struct message rep;

                rep.header            = CON_REP;
                rep.pid               = data.pid;
                rep.settings.schedule = Dynamic_chunks;

                fill_n(rep.settings.thread_pinnings, MAX_NUM_THREADS, -1);

                for (uint32_t i = 0; i < 4; i++)
                {
                    rep.settings.thread_pinnings[i] = i;
                }

                // Send reply to client.
                m_send(socket, rep);

                message_printout(Sending, rep);

                rep.settings.schedule = Static;

                sleep(3);

                m_send(socket, rep);

                message_printout(Sending, rep);

                break;
        }
    }
    return 0;
}

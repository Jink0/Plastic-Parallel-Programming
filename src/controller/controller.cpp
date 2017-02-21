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

#include "zhelpers.hpp"

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
    socket_t socket (context, ZMQ_PAIR);

    socket.bind("tcp://*:5555");

    string str = s_recv(socket);
        
    cout << "Received " << str << "\n\n";

    s_send(socket, "Data1");

    cout << "\nSent Data1\n\n";

    s_send(socket, "Data2");

    cout << "\nSent Data2\n\n";

    s_send(socket, "Data3");

    cout << "\nSent Data3\n\n";

    str = s_recv(socket);
        
    cout << "Received " << str << "\n\n";




    /*struct message ack;

    ack.header            = CON_DATA_REP;
    ack.pid               = data.pid;
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

    zmq::message_t id(sizeof(identity));
    memcpy (id.data(), &identity, sizeof(identity));



    cout << "REPLY SENT!\n\n" << endl;*/


    /*//  Initialize poll set
    zmq::pollitem_t items [] = {
        { rgsr_socket, 0, ZMQ_POLLIN, 0 },
        { data_socket, 0, ZMQ_POLLIN, 0 }
    };
    
    //  Switch messages between sockets
    while (1) {
        zmq::message_t message;

        zmq::poll (&items [0], 2, -1);
        
        if (items [0].revents & ZMQ_POLLIN) {
            while (1) {
                string id = s_recv(rgsr_socket);
                s_recv(rgsr_socket);

                std::cout << "Received " << s_recv(rgsr_socket) << " from id " << id << std::endl;

                s_sendmore(rgsr_socket, id);
                s_sendmore(rgsr_socket, "");
                s_send(rgsr_socket, "World");

                std::cout << "Sent World" << std::endl;



                s_sendmore(data_socket, id);
                s_sendmore(data_socket, "");
                s_send(data_socket, "World2");

                std::cout << "Sent World2" << std::endl;



                id = s_recv(data_socket);
                s_recv(data_socket);

                std::cout << "Received " << s_recv(data_socket) << " from id " << id << std::endl;
            }
        }

        if (items [1].revents & ZMQ_POLLIN) {
            while (1) {
                //  Process all parts of the message
                //data_socket.recv(&message);
                //rgsr_socket.send(message, more? ZMQ_SNDMORE: 0);
            }
        }
    }


    //socket.send(id, ZMQ_SNDMORE);
    //socket.send(id, ZMQ_SNDMORE); // Envelope delimiter
    //socket.send(reply);

    /*s_sendmore(socket, identity);
    s_sendmore(socket, "");
    s_send(socket, "Test successful!");

    cout << "REPLY2 SENT!\n\n" << endl;



    /*zmq::message_t message(string.size());
    memcpy (message.data(), string.data(), string.size());

    bool rc = socket.send (message, ZMQ_SNDMORE);

        s_sendmore(socket, identity);
        s_sendmore(socket, "");

        //  Encourage workers until it's time to fire them
    s_send(socket, "Hello from controller!\n");

    s_sendmore(socket, identity);
    s_sendmore(socket, "");

        //  Encourage workers until it's time to fire them
    s_send(socket, "Hello from controller2!!!\n");

    /*while (true) {
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

                ack.settings.schedule = Dynamic_individual;

                message_t reply2 (sizeof(ack));
                memcpy(reply2.data(), &ack, sizeof(ack));

                sleep(5);

                print_messages(Sending, ack);

                socket.send(reply2);

                break;
        }
    }*/
    return 0;
}

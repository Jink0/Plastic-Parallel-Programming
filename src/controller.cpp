#include <string>
#include <iostream>

#include <unistd.h>

#include <boost/thread.hpp> // boost::thread::hardware_concurrency();

#include <comms.hpp>

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



static int exec_prog(const char **args) 
{
        pid_t pid;
        int   status, timeout;

        if ((pid = fork()) == 0) 
        {
            if (execve(args[0], (char **)args , NULL) == -1) 
            {
                perror("fork() failed [%m]");

                return -1;
            }
        }


        timeout = 1000;

        while (waitpid(pid , &status , WNOHANG) == 0) 
        {
                if (--timeout < 0) 
                {
                        perror("timeout");

                        return -1;
                }

                sleep(1);
        }

        // printf("%s WEXITSTATUS %d WIFEXITED %d [status %d]\n\n", args[0], WEXITSTATUS(status), WIFEXITED(status), status);

        // if (WIFEXITED(status) != 1 || WEXITSTATUS(status) != 0) 
        // {
        //         perror("%s failed, halt system");

        //         return -1;
        // }

        return 0;
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
                {
                    message_printout(Receving, data);
                        
                    struct message rep;

                    rep.header            = CON_REP;
                    rep.pid               = data.pid;
                    rep.settings.schedule = Static;

                    fill_n(rep.settings.thread_pinnings, MAX_NUM_THREADS, -1);

                    for (uint32_t i = 0; i < 4; i++)
                    {
                        rep.settings.thread_pinnings[i] = i;
                    }

                    // Send reply to client.
                    m_send(socket, rep);

                    message_printout(Sending, rep);

                    break;
                }

            case APP_TERM:
                {
                    cout << "PID: " << data.pid << " terminated" << endl << endl;

                    // Remove application from active list.

                    break;
                }
        }
    }
    return 0;
}
// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#ifndef GCHAT_SERVER_H
#define GCHAT_SERVER_H

#include "server_thread.h"
#include <map>
#include <functional>

#define SERVER_DATAFILE "server_data.data"

/*
    A basic overview of the server:
    The server_data file is first read in and loaded into credentials. The server's main thread then enters the main loop.
    The server's main loop uses poll() to listen to the input socket. When a new connection is requested,
    the resulting socket (along with server_data_t data) is passed to a new ServerThread, which handles the socket on a new thread.
    In ServerThread's constructor, an AsyncSender is also created for the socket, giving each socket 2 threads, one for input and one for output.
    The main loop also does periodic resource cleanups when not responding to a new connection request.
*/

class Server{
    private:
        int listen_sock; //the socket used to establish new connections

        unsigned short port; //the port the server is running on
        bool shutdown; //if set to true, the main loop will close the server gracefully as soon as possible

        //socket -> ServerThread*
        std::map<int, ServerThread*> reader_threads; //holds the ServerThreads which react to inputs on the socket
        std::mutex readers_mutex;

        //socket -> AsyncSender*
        std::map<int, AsyncSender*> async_senders; //used to send a message thru a socket on a background thread
        std::mutex senders_mutex;

        //username -> socket_fd if online
        std::map<std::string, int> users;
        std::mutex users_mutex;

        //username -> message_queue
        std::map<std::string, std::queue<message_t*>*> offline_msg_queue; //buffers messages for offline uses
        std::mutex offline_msg_mutex;

        //username -> password_hash
        std::map<std::string, size_t> credentials;
        std::mutex credentials_mutex;

        server_data_t data;

        void LoadData(); //load server_data.data (credentials)
        void SaveData(); //save credentials
        int InitListenSocket(); //setup the listening socket
        void AcceptConnection(); //accept a new connection req from the listening socket

    public:
        Server(unsigned short port);
        ~Server();
        void Run(); //start the server
        
};

#endif
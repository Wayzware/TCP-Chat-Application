// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#ifndef GCHAT_CLIENT_H
#define GCHAT_CLIENT_H

#include "../core/gchat_core.h"


class Client
{
    private:
        std::string script_file; // path to the script this client will be running
        std::string server_ip;
        int port;
        std::thread* listener_thread; // thread used to listen for messages on
        MessageFactory message_factory; // used to create messages
        AsyncSender* async_sender; // used to send messages to the server without blocking
        int connected; // 0 if not connected
        int sock; // the socket connection with the server

        bool kill_listener; //set to true to kill the listener and async sender;
        
        void Execute(std::string command); // Executes a single command line
        void ListenerMain(); // Listens on sock for input data and reacts to data, also monitors for disconnections
        int StartListener(); // Starts listener_thread
        void StopListener(); // Kills listener_thread
        int SendMessage(message_t* message); // Sends a message to the server using async_sender
        void InitSocket(); // Initializes the socket with the server
        void HandleMessage(message_t* message); // Called by listener_thread to react to a message from the server
        int RebootConnection(); // Called to attempt to repair a broken connection

    
    public:
        Client(std::string ip, int port, std::string script);
        ~Client();
        void Run(); // Starts the client using the arguments supplied in the constructor
        int Connect(); // Connects the client to the server
        
};


#endif
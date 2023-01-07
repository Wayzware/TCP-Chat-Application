// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#ifndef GCHAT_SERVER_THREAD_H
#define GCHAT_SERVER_THREAD_H

#include <thread>
#include <map>
#include <mutex>
#include "../core/gchat_core.h"

#define SERVER_POLL_TIMEOUT 100

#define STSTATUS_DEAD -1
#define STSTATUS_INIT 0
#define STSTATUS_ACTIVE 1

struct server_data;

//The ServerThread's job is to listen to an open socket and react
class ServerThread{
    private:
        struct server_data* data;

        int sock;
        int connected_asender;
        int kill;
        std::string user;
        MessageFactory message_factory;
        std::mutex listener_is_alive;

        void ServerThreadMain();
        void HandleMessage(message_t* message);
        
        void HandleRegister(std::string username, size_t password);
        void HandleLogin(std::string username, size_t password);
        void HandleTextMessage(std::string dest, std::string message, unsigned int time);
        void HandleFileMessage(std::string dest, std::string filename, char* file, size_t file_size, unsigned int time);
        void HandleListRequest();
        void HandleDisconnect();

    public:
        std::thread* thread;
        int status;
        ServerThread(struct server_data* data, int open_socket);
        ~ServerThread();
        void KillAsync();
        void HandleLogout(bool lock);

};

//holds pointers to the server's data
struct server_data {
    std::map<int, ServerThread*>* reader_threads;

    //socket -> AsyncSender*
    std::map<int, AsyncSender*>* async_senders;
    std::mutex* senders_mutex;

    //username -> socket_fd if online else -1
    std::map<std::string, int>* users;
    std::mutex* users_mutex;

    //username -> queue of messages
    std::map<std::string, std::queue<message_t*>*>* offline_msg_queue;
    std::mutex* offline_msg_mutex;

    std::map<std::string, size_t>* credentials;
    std::mutex* credentials_mutex;

    unsigned short* port;
    bool* shutdown;
} typedef server_data_t;

#endif
// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#ifndef ASYNC_SENDER_H
#define ASYNC_SENDER_H

#include <thread>
#include <mutex>
#include <queue>
#include <semaphore.h>
#include <functional>
#include <unistd.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "message.h"


// The AsyncSender class is used to send messages through a socket (either client->server or server->client) on a background thread
// This is done so we do not block the main thread, and on the server, we can send multiple messages at once
class AsyncSender{
    private:
        std::thread* sender_thread; // The background thread
        std::mutex is_alive; // mutex held by the sender_thread when it is alive (used to prevent busy waiting)

        std::queue<message_t*> msg_queue; // The message queue that is added to by another thread and removed from by sender_thread's MainLoop()
        std::mutex queue_mutex; // Mutex to protect msg_queue
        
        sem_t* queue_sem; // Semaphore used to wake up the AsyncSender after a new item has been added to msg_queue by SendAsync()

        int sock; // The socket we send data through
        bool kill; // Used to signal sender_thread to shut down
        int* connected; // Pointer to the socket's connected status (from client or server)
        void MainLoop(); // sender_thread's main loop function

    public:
        AsyncSender(int sock, int* connected);
        ~AsyncSender();
        void SendAsync(message_t* message); // Adds a message_t to msg_queue to be sent over the network on sender_thread
};

#endif
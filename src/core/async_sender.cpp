// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#include "async_sender.h"

AsyncSender::AsyncSender(int sock, int* connected){
    kill = 0;
    this->sock = sock;
    this->connected = connected;
    queue_sem = (sem_t*) malloc(sizeof(sem_t));
    sem_init(queue_sem, 0, 0); //initialize the queue semaphore
    sender_thread = new std::thread(&AsyncSender::MainLoop, this);
}

AsyncSender::~AsyncSender(){
    kill = true;
    sem_post(queue_sem);
    is_alive.lock(); //wait for the listener thread to die
    sender_thread->join();
    for(size_t i = 0; i < msg_queue.size(); i++){ //free everything in the message queue
        message_t* message = msg_queue.front();
        msg_queue.pop();
        delete[] message->payload;
        delete message;
    }
    sem_destroy(queue_sem);
    free(queue_sem);
    delete sender_thread;
}


void AsyncSender::MainLoop(){
    is_alive.lock(); //take the listener life lock
    while(1){
        sem_wait(queue_sem); //wait for a signal that something is in queue (or a shutdown signal)
        if(kill) { is_alive.unlock(); return; } //shutdown

        if(!*connected) {
            //std::cout << "Async sender thread died unexpectedly due to loss of connection (all queued messages were lost)" << std::endl;
            is_alive.unlock();
            return;
        }

        queue_mutex.lock(); //grab the queue mutex
        message_t* message = msg_queue.front(); //grab the top item from the queue
        msg_queue.pop(); //remove the item from the queue
        queue_mutex.unlock(); //unlock the queue mutex

        //these next several lines place the message_t into a uniform char array
        char* msgbuff = new char[1 + sizeof(uint64_t) + sizeof(size_t) + message->payload_size];
        memset(msgbuff, 0, 1 + sizeof(uint64_t) + sizeof(size_t) + message->payload_size);
        memmove(msgbuff, &message->type, 1);
        memmove(msgbuff + 1, &message->id, sizeof(uint64_t));
        memmove(msgbuff + 1 + sizeof(uint64_t), &message->payload_size, sizeof(size_t));
        memmove(msgbuff + 1 + sizeof(uint64_t) + sizeof(size_t), message->payload, message->payload_size);
 
        size_t bytes = send(sock, msgbuff, 1 + sizeof(uint64_t) + sizeof(size_t) + message->payload_size, 0); //send the message!
        if(bytes != 1 + sizeof(uint64_t) + sizeof(size_t) + message->payload_size) std::cout << "Message not fully sent (bytes sent: " << bytes << ") (bytes expected: " << 1 + sizeof(uint64_t) + sizeof(size_t) + message->payload_size << ")" << std::endl;
        delete[] msgbuff;

        //due to differences in the client/server code, this is necessary
#ifdef GCHAT_CLIENT_H
        delete message->payload;
#endif
#ifdef GCHAT_SERVER_H
        delete[] message->payload;
#endif
        delete message;
    }
}

void AsyncSender::SendAsync(message_t* message){ //add a message to the queue
    queue_mutex.lock();
    msg_queue.push(message);
    queue_mutex.unlock();
    sem_post(queue_sem);
}
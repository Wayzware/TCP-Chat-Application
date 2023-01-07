// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#ifndef MESSAGE_H
#define MESSAGE_H

#include "util.h"
#include <stdint.h>
#include <time.h>
#include "payload.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "debug.h"


/*

    The Message hierarchy works like this:
    layer0: char[] of varying size, first byte is type, next 8 are the id, next 8 are payload_size, rest are payload; send and recv over sockets
    layer1: message_t struct, payload is still in char[] format
    layer2: payload is deserialized to Payload type
    layer3: Payload is cast to specific payload child class (ex. RegisterRequest or LoginRequest)

*/


#pragma pack(1)
struct __message
{
    char type; // one of the constants defined in payload.h
    uint64_t id; // the upper 32 bits contain a random number (currently not used for anything), the lower 32 bits contain the unix timestamp of the message
    size_t payload_size; // the size of the buffer containing the payload
    char* payload; // pointer to the buffer containing the payload
} typedef message_t;

// Get the timestamp from a message in printable format
inline std::string GetTimestamp(message_t* message){
    time_t tim = (uint32_t) (message->id & 0xFFFFFFFF);
    struct tm tim_struct;
    localtime_r(&tim, &tim_struct);
    char time_chars[128];
    strftime(time_chars, 127, "%F %r", &tim_struct);
    return std::string(time_chars);
}

inline std::string GetTimestamp(unsigned int timestamp){
    time_t tim = (uint32_t) (timestamp & 0xFFFFFFFF);
    struct tm tim_struct;
    localtime_r(&tim, &tim_struct);
    char time_chars[128];
    strftime(time_chars, 127, "%F %r", &tim_struct);
    return std::string(time_chars);
}

// Ideally for server use only, used to change the timestamp of a message while keeping the random bits of id the same
inline message_t* AlterTimestamp(message_t* message, uint32_t timestamp){
    message->id = (message->id & 0xFFFFFFFF00000000) | timestamp;
    return message;
}

// For reading a message_t* from an open socket
inline message_t* ReadMessage(int sock){
    char* buffer = new char[10485881]; //this is on the heap because it will overflow the stack, size is max message size we could generate + 1
    message_t* message;
    size_t bytes = recv(sock, buffer, 1 + sizeof(uint64_t) + sizeof(size_t), 0);
    if(bytes != 1 + sizeof(uint64_t) + sizeof(size_t)){
        delete[] buffer;
        return nullptr;
    }
    size_t bytes_to_read = 0;
    memcpy(&bytes_to_read, (char*)(buffer + 1 + sizeof(uint64_t)), sizeof(size_t)); //get the payload size
    if(bytes_to_read > MAX_FILE_SIZE + MAX_CRED_SIZE + MAX_FILENAME_SIZE){
        delete[] buffer;
        return nullptr;
    } 
    while(bytes_to_read > 0) { 
        size_t bytes_read = recv(sock, buffer + bytes, bytes_to_read, 0);
        if(!bytes_read){
            delete[] buffer;
            return nullptr;
        }
        bytes_to_read -= bytes_read;
        bytes += bytes_read;
    }
    
    if((bytes == -1) || (bytes == 0)){ // if we read nothing or something went wrong
        delete[] buffer;
        return nullptr;
    }
    //std::cout << "read " << bytes << std::endl;
    if(bytes == sizeof(buffer)){ // if we read the max message size + 1, we know this is a bad transmission so dump it
        printf("Invalid message recv'd on socket (too large)\n");
        //std::cout << buffer << std::endl;
        delete[] buffer;
        return nullptr;
    }
    if(bytes < 1 + sizeof(uint64_t) + sizeof(size_t)){ // if we read less than a message's layer0 minimum size, we know this is a bad transmission so dump it too
        printf("Invalid message recv'd on socket (too small)\n");
        delete[] buffer;
        return nullptr;
    }
    message = new message_t;
    memmove(&message->type, buffer, 1); // fill in the message type
    memmove(&message->id, buffer + 1, sizeof(uint64_t)); // fill in the message id (including timestamp)
    memmove(&message->payload_size, buffer + 1 + sizeof(uint64_t), sizeof(size_t)); // fill in the payload_size
    if(message->payload_size > bytes - (1 + sizeof(uint64_t) + sizeof(size_t))){ // if the payload_size is bigger than the bytes we have left to read from the buffer, fix it (though it is still probably a bad transmission)
        printf("Warning: changing payload size automagically (%lu -> %lu)\n", message->payload_size, bytes - (1 + sizeof(uint64_t) + sizeof(size_t)));
        message->payload_size = bytes - (1 + sizeof(uint64_t) + sizeof(size_t));
    }
    message->payload = new char[message->payload_size]; // make a new layer1 buffer for the payload
    memmove(message->payload, buffer + 1 + sizeof(uint64_t) + sizeof(size_t), message->payload_size); // fill in the layer1 buffer
    delete[] buffer; // delete the layer0 buffer
    return message; //return message_t (layer1)
}

// prints a file message payload
void PrintFileMessage(FileMessage* payload, size_t file_size, std::string timestamp);

// prints a list message payload
void PrintListMessage(ListMessage* payload, std::string timestamp);


// prints a text message payload
void PrintTextMessage(TextMessage* payload, std::string timestamp);

void PrintFileMessage(std::string user, std::string dest, std::string filename, char* file, size_t file_size, unsigned int timestamp);

#endif
// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#ifndef MESSAGE_FACTORY_H
#define MESSAGE_FACTORY_H

#include "util.h"
#include "message.h"
#include "time.h"
#include <random>
#include <functional>

// This class generates message_t's from basic arguments and deserializes the payload
class MessageFactory{

    private:
        // After the payloads are created by the Generate____() functions, this function is called to wrap it in a message_t
        message_t* ToMessage(char* payload, size_t payload_size, char type);

    public:
        MessageFactory();

        // Extracts the payload from the char buffer in message and returns the full message class pointer (ex. RegisterRequest*, LoginRequest*)
        Payload* ExtractPayload(message_t* message);

        //These functions generate messages from basic arguments
        message_t* GenerateRegisterRequest(std::string username, std::string password);
        message_t* GenerateLoginRequest(std::string username, std::string password);
        message_t* GenerateLogoutRequest();
        message_t* GenerateTextMessage(std::string dest, std::string text);
        message_t* GenerateFileMessage(std::string dest, std::string filepath);
        message_t* GenerateListRequest();
        message_t* GenerateAcknowledgeResponse(uint64_t id);
        message_t* GenerateListResponse(std::vector<std::string> online_users);
};

#endif
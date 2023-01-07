// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#ifndef PAYLOAD_H
#define PAYLOAD_H

#define MAX_CRED_SIZE 16
#define MAX_MSG_SIZE 256
#define MAX_FILENAME_SIZE 64
#define MAX_LISTM_SIZE 1024
#define MAX_FILE_SIZE 10485760

#define REG_PAYLOAD 1
#define LOGIN_PAYLOAD 2
#define LOGOUT_PAYLOAD 3
#define TEXT_PAYLOAD 4
#define FILE_PAYLOAD 5
#define LISTR_PAYLOAD 10
#define LISTM_PAYLOAD 11
#define ACK_PAYLOAD 16

// the base class for the other payloads (layer2 from message.h)
class Payload{};

// the child classes of Payload (i.e. the actual payloads, layer3 from message.h)
#pragma pack(1) //pack it down
class RegisterRequest : public Payload{
    public:
        char username[MAX_CRED_SIZE];
        size_t password; // password is sent as a hash
};

#pragma pack(1)
class LoginRequest : public Payload{
    public:
        char username[MAX_CRED_SIZE];
        size_t password; // password is sent as a hash
};

#pragma pack(1)
class LogoutRequest : public Payload{

};

#pragma pack(1)
class TextMessage : public Payload{
    public:
        char dest[MAX_CRED_SIZE];
        char message[MAX_MSG_SIZE];
};

#pragma pack(1)
class FileMessage: public Payload{
    public:
        char dest[MAX_CRED_SIZE];
        char filename[MAX_FILENAME_SIZE];
        char* file;
};

#pragma pack(1)
class ListRequest : public Payload{

};

#pragma pack(1)
class ListMessage : public Payload{ // not used
    public:
        char list[MAX_LISTM_SIZE];
};

#pragma pack(1)
class AcknowledgeResponse : public Payload{ // not used
    public:
        uint64_t id;
};

#endif
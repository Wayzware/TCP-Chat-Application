// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#include "message_factory.h"
#include "debug.h"

MessageFactory::MessageFactory(){
    srandom(time(NULL));
}

//convert the payload to a message_t
message_t* MessageFactory::ToMessage(char* payload, size_t payload_size, char type){
    message_t* msg = new message_t;
    msg->payload = payload;
    msg->payload_size = payload_size;
    msg->type = type;
    msg->id = (((uint64_t)random()) << 32) + time(NULL); //set the id to a random number | timestamp
    return msg;
}

Payload* MessageFactory::ExtractPayload(message_t* message){
    switch (message->type){
        case REG_PAYLOAD:{ //build a RegisterRequest from char* payload
            RegisterRequest* rreq = new RegisterRequest();
            if(message->payload_size < sizeof(RegisterRequest)) goto payload_failure;
            memset(rreq->username, 0, MAX_CRED_SIZE);
            strncpy(rreq->username, message->payload, MAX_CRED_SIZE);
            dprint("RUSERNAME: " << message->payload);
            memmove(&rreq->password, message->payload + MAX_CRED_SIZE, sizeof(size_t));
            delete[] message->payload;
            return rreq;
        }
        case LOGIN_PAYLOAD:{ //build a LoginRequest from char* payload
            LoginRequest* lreq = new LoginRequest();
            if(message->payload_size < sizeof(LoginRequest)) goto payload_failure;
            strncpy(lreq->username, message->payload, MAX_CRED_SIZE);
            memmove(&lreq->password, message->payload + MAX_CRED_SIZE, sizeof(size_t));
            delete[] message->payload;
            return lreq;
        }
        case LOGOUT_PAYLOAD:{ //build a LogoutRequest from char* payload
            delete[] message->payload;
            return new LogoutRequest();
        }
        case TEXT_PAYLOAD:{ //build a TextMessage from char* payload
            TextMessage* tmsg = new TextMessage();
            if(message->payload_size < sizeof(TextMessage)) goto payload_failure;
            strncpy(tmsg->dest, message->payload, MAX_CRED_SIZE);
            strncpy(tmsg->message, message->payload + MAX_CRED_SIZE, MAX_MSG_SIZE);
            delete[] message->payload;
            return tmsg;
        }
        case FILE_PAYLOAD:{ //build a FileMessage from char* payload
            FileMessage* fmsg = new FileMessage();
            if(message->payload_size < MAX_CRED_SIZE + MAX_FILENAME_SIZE) goto payload_failure;
            strncpy(fmsg->dest, message->payload, MAX_CRED_SIZE);
            memmove(fmsg->filename, message->payload + MAX_CRED_SIZE, MAX_FILENAME_SIZE);
            fmsg->file = new char[message->payload_size - MAX_CRED_SIZE - MAX_FILENAME_SIZE];
            memmove(fmsg->file, message->payload + MAX_CRED_SIZE + MAX_FILENAME_SIZE, message->payload_size - MAX_CRED_SIZE - MAX_FILENAME_SIZE);
            delete[] message->payload;
            return fmsg;
        }
        case LISTR_PAYLOAD:{ //build a ListRequest from char* payload
            delete[] message->payload;
            return new ListRequest();
        }
        case LISTM_PAYLOAD:{ //build a ListMessage from char* payload (unused)
            ListMessage* lmsg = new ListMessage();
            if(message->payload_size < sizeof(ListMessage)) goto payload_failure;
            strncpy(lmsg->list, message->payload, MAX_LISTM_SIZE);
            delete[] message->payload;
            return lmsg;
        }
        case ACK_PAYLOAD:{ //build an AcknowledgeResponse from char* payload (unused)
            AcknowledgeResponse* ackmsg = new AcknowledgeResponse();
            if(message->payload_size != sizeof(uint64_t)) goto payload_failure;
            memmove((void*) ackmsg->id, message->payload, sizeof(uint64_t));
            delete[] message->payload;
            return ackmsg;
        }
        default:{
            goto payload_failure;
        }
    }

    payload_failure:
    std::cout << "Extract payload failure for message " << message->id << std::endl;
    return nullptr;
}

//build a RegisterRequest from basic arguments
message_t* MessageFactory::GenerateRegisterRequest(std::string username, std::string password){
    char* rreq = new char[sizeof(RegisterRequest)];
    std::hash<std::string> hasher;
    memset(rreq, 0, sizeof(RegisterRequest));
    strncpy(rreq, username.c_str(), 15);
    rreq[15] = 0;
    size_t hash = hasher(password);
    memmove(rreq+16, &hash, sizeof(size_t));
    return ToMessage(rreq, sizeof(RegisterRequest), REG_PAYLOAD);
}

//build a LoginRequest from basic arguments
message_t* MessageFactory::GenerateLoginRequest(std::string username, std::string password){
    LoginRequest* lreq = new LoginRequest();
    std::hash<std::string> hasher;
    strncpy(lreq->username, username.c_str(), 15);
    lreq->username[15] = 0;
    lreq->password = hasher(password);
    return ToMessage((char*) lreq, sizeof(LoginRequest), LOGIN_PAYLOAD);
}

//build a LogoutRequest from basic arguments
message_t* MessageFactory::GenerateLogoutRequest(){
    LogoutRequest* lreq = new LogoutRequest();
    return ToMessage((char*) lreq, sizeof(LogoutRequest), LOGOUT_PAYLOAD);
}

//build a TextMessage from basic arguments
message_t* MessageFactory::GenerateTextMessage(std::string dest, std::string text){
    char* tmsg = new char[sizeof(TextMessage)];
    memset(tmsg, 0, sizeof(TextMessage));
    strncpy(tmsg, dest.c_str(), 15);
    tmsg[15] = 0;
    strncpy(tmsg+16, text.c_str(), MAX_MSG_SIZE);
    tmsg[sizeof(TextMessage) - 1] = 0;
    //printf("STRING:%s\n", tmsg+16);
    return ToMessage(tmsg, sizeof(TextMessage), TEXT_PAYLOAD);
}

//build a FileMessage from basic arguments
message_t* MessageFactory::GenerateFileMessage(std::string dest, std::string filepath){
    char* file_data = new char[MAX_FILE_SIZE];
    memset(file_data, 0, MAX_FILE_SIZE);
    size_t bytes = ReadFile(file_data, filepath, MAX_FILE_SIZE);
    char* payload = new char[MAX_CRED_SIZE + MAX_FILENAME_SIZE + bytes];
    dprint("bytes::::: " << bytes);
    memset(payload, 0, MAX_CRED_SIZE + MAX_FILENAME_SIZE + bytes);
    strncpy(payload, dest.c_str(), 15);
    payload[15] = 0;
    strncpy(payload + 16, filepath.c_str(), MAX_FILENAME_SIZE);
    payload[64+15] = 0;
    memmove(payload + 64 + 16, file_data, bytes);
    delete[] file_data;
    return ToMessage(payload, MAX_CRED_SIZE + MAX_FILENAME_SIZE + bytes, FILE_PAYLOAD);
}

//build a ListRequest from basic arguments
message_t* MessageFactory::GenerateListRequest(){
    ListRequest* lreq = new ListRequest();
    memset(lreq, 0, 1);
    return ToMessage((char*) lreq, sizeof(ListRequest), LISTR_PAYLOAD);
}

//build an AcknowledgeResponse from basic arguments (unused)
message_t* MessageFactory::GenerateAcknowledgeResponse(uint64_t id){
    AcknowledgeResponse* ares = new AcknowledgeResponse();
    return ToMessage((char*) ares, sizeof(AcknowledgeResponse), ACK_PAYLOAD);
}

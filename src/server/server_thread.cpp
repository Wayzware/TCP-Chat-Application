// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#include "server_thread.h"
#include "gchat_server.h"

ServerThread::ServerThread(server_data_t* data, int open_socket){
    this->data = data;
    sock = open_socket;
    connected_asender = 1;
    kill = 0;
    status = STSTATUS_INIT;
    user = "";

    //create an AsyncSender for this socket so we can non-blocking send messages
    data->senders_mutex->lock();
    auto sender = data->async_senders->find(sock);
    if(sender != data->async_senders->end()){
        delete data->async_senders->at(sock);
        data->async_senders->erase(sock);
    }
    data->async_senders->operator[](sock) = new AsyncSender(sock, &connected_asender);
    data->senders_mutex->unlock();

    //run ServerThreadMain() on a new thread
    thread = new std::thread(&ServerThread::ServerThreadMain, this);
}

ServerThread::~ServerThread(){
    kill = 1;
    listener_is_alive.lock();
    if(thread){
        if(thread->joinable()){
            thread->join();
        }
        delete thread;
    }
}

void ServerThread::ServerThreadMain(){
    listener_is_alive.lock(); //take the listener thread mutex

    struct pollfd pfd;
    pfd.fd = sock;
    pfd.events = POLLIN | POLLRDHUP | POLLHUP;

    while(!*data->shutdown && !kill){
        int ret = poll(&pfd, 1, SERVER_POLL_TIMEOUT); //listen for incomming messages from the client
        if((pfd.revents || ret) && 0){
            dprint_v("revents: " << pfd.revents);
            dprint_v("ret: " << ret);
        }
        if(*data->shutdown || kill) { //shutdown signal
            HandleDisconnect();
            status = STSTATUS_DEAD;
            listener_is_alive.unlock();
            return;
        }
        if(pfd.revents & (POLLRDHUP | POLLHUP)){ //the connection was interrupted
            std::cout << "User disconnected from socket " << sock << std::endl;
            if(status) HandleLogout(false);
            HandleDisconnect();
            status = STSTATUS_DEAD;
            listener_is_alive.unlock();
            return;
        }
        if(pfd.revents & POLLIN){ //there is data to read from the client
            message_t* msg = ReadMessage(sock);
            HandleMessage(msg);
            pfd.revents = 0;
            continue;
        }
        if(!ret){
            continue;
        }
        if(!pfd.revents){
            std::cout << "User disconnected from socket " << sock << std::endl;
            if(status) HandleLogout(false);
            HandleDisconnect();
            status = STSTATUS_DEAD;
            listener_is_alive.unlock();
            return;
        }
        
    }
    status = STSTATUS_DEAD;
    listener_is_alive.unlock();
}


void ServerThread::HandleDisconnect(){
    //get rid of the async sender if the client disconnected
    data->senders_mutex->lock();
    auto sender = data->async_senders->find(sock);
    if(sender != data->async_senders->end()){
        delete data->async_senders->at(sock);
        data->async_senders->erase(sock);
    }
    data->senders_mutex->unlock();
}

//send the kill signal to the listener thread
void ServerThread::KillAsync(){
    kill = 1;
}

//process a message from the client
void ServerThread::HandleMessage(message_t* message){
    if(!message) return;
    switch(message->type){
        case REG_PAYLOAD:{
            RegisterRequest* rreq = (RegisterRequest*) (message_factory.ExtractPayload(message));
            if(!rreq) goto error;
            HandleRegister(rreq->username, rreq->password);
            delete rreq;
            break;
        }
        case LOGIN_PAYLOAD:{
            LoginRequest* lreq = (LoginRequest*) message_factory.ExtractPayload(message);
            if(!lreq) goto error;
            HandleLogin(lreq->username, lreq->password);
            delete lreq;
            break;
        }
        case LOGOUT_PAYLOAD:{
            LogoutRequest* lreq = (LogoutRequest*) message_factory.ExtractPayload(message);
            if(!lreq) goto error;
            HandleLogout(true);
            delete lreq;
            break;
        }
        case TEXT_PAYLOAD:{
            TextMessage* tmsg = (TextMessage*) message_factory.ExtractPayload(message);
            if(!tmsg) goto error;
            HandleTextMessage(tmsg->dest, tmsg->message, message->id & UINT32_MAX);
            delete tmsg;
            break;
        }
        case FILE_PAYLOAD:{
            FileMessage* fmsg = (FileMessage*) message_factory.ExtractPayload(message);
            if(!fmsg) goto error;
            HandleFileMessage(fmsg->dest, fmsg->filename, fmsg->file, message->payload_size - MAX_CRED_SIZE - MAX_FILENAME_SIZE, message->id & UINT32_MAX);
            delete[] fmsg->file;
            delete fmsg;
            break;
        }
        case LISTR_PAYLOAD:{
            ListRequest* lreq = (ListRequest*) message_factory.ExtractPayload(message);
            if(!lreq) goto error;
            HandleListRequest();
            delete lreq;
        }
        case LISTM_PAYLOAD:{
            break;
            ListMessage* lmsg = (ListMessage*) message_factory.ExtractPayload(message);
            if(!lmsg) goto error;
            std::cout << "Recv'd ListMessage as server???" << std::endl;
            delete lmsg; 
            break;
        }
        default:{
            error:
            printf("Invalid message or message type recv'd\n");
            break;
        }
    }
    delete message;
    return;
}

//register a new user by adding them to the data structures
void ServerThread::HandleRegister(std::string username, size_t password){
    dprint("REGISTER");
    data->credentials_mutex->lock();
    if(data->credentials->find(username) == data->credentials->end()){
        data->credentials->operator[](username) = password;
        data->credentials_mutex->unlock();
        data->offline_msg_mutex->lock();
        data->offline_msg_queue->operator[](username) = new std::queue<message_t*>();
        data->offline_msg_mutex->unlock();
        std::cout << "[" << GetTimestamp(time(NULL)) << "] " << "User: " << username << " registered" << std::endl; 
        data->async_senders->operator[](sock)->SendAsync(message_factory.GenerateTextMessage(std::string("*SERVER*"), std::string("Register successful")));
    }
    else{
        data->credentials_mutex->unlock();
        std::cout << "[" << GetTimestamp(time(NULL)) << "] " << "User: " << username << " register failed" << std::endl;
        data->async_senders->operator[](sock)->SendAsync(message_factory.GenerateTextMessage(std::string("*SERVER*"), std::string("Could not register user (username already exists?)")));
    }
}

//log out from this socket
void ServerThread::HandleLogout(bool lock = true){
    if(status != STSTATUS_ACTIVE) return;
    if(lock) data->users_mutex->lock();
    if(data->users->find(user) != data->users->end()){
        data->users->erase(user);
    }
    status = STSTATUS_INIT;
    std::cout << "[" << GetTimestamp(time(NULL)) << "] " << "User: " << user << " logged out" << std::endl; 
    data->offline_msg_mutex->lock();
    data->offline_msg_queue->operator[](user) = new std::queue<message_t*>();
    data->offline_msg_mutex->unlock();
    data->async_senders->operator[](sock)->SendAsync(message_factory.GenerateTextMessage(std::string("*SERVER*"), std::string("You have been logged out!")));
    if(lock) data->users_mutex->unlock();
    user = "";
}

//process the incomming text message (and send it to others)
void ServerThread::HandleTextMessage(std::string dest, std::string message, unsigned int time){
    if(status != STSTATUS_ACTIVE){
        data->senders_mutex->lock();
        data->async_senders->operator[](sock)->SendAsync(message_factory.GenerateTextMessage(std::string("*SERVER*"), std::string("You must be logged in to do that!")));
        data->senders_mutex->unlock();
        return;
    }
    const char* dcstr = dest.c_str();
    std::cout << "[" << GetTimestamp(time) << "] " << user << " to " << dest << ": " << message << std::endl;
    if(dcstr[0] == '@'){ //send to all registered people
        data->senders_mutex->lock();
        for(auto pair : *data->users){
            if(pair.second < 1) continue;
            data->async_senders->operator[](pair.second) ->SendAsync(AlterTimestamp(message_factory.GenerateTextMessage(user, message), time));
        }
        data->senders_mutex->unlock();
        data->offline_msg_mutex->lock();
        for(auto pair : *data->offline_msg_queue){
            pair.second->push(AlterTimestamp(message_factory.GenerateTextMessage(user, message), time));
        }
        data->offline_msg_mutex->unlock();
        return;
    }
    else if(dcstr[0] == '$'){ //send to all registered people, but anonymously
        data->users_mutex->lock();
        data->senders_mutex->lock();
        data->offline_msg_mutex->lock();
        for(auto pair : *data->users) {
            if(pair.second > 1) data->async_senders->operator[](pair.second)->SendAsync(AlterTimestamp(message_factory.GenerateTextMessage("Anonymous", message), time));
            else data->offline_msg_queue->operator[](pair.first)->push(AlterTimestamp(message_factory.GenerateTextMessage("Anonymous", message), time));
        }
        data->offline_msg_mutex->unlock();
        data->senders_mutex->unlock();
        data->users_mutex->unlock();
        return;
    }
    else if(dcstr[0] == '?'){ //send to a specific person, but anonymously
        std::string to = std::string(dcstr + 1);
        data->users_mutex->lock();
        if(data->users->find(to) != data->users->end()){
            if(data->users->at(to) > 1){
                data->senders_mutex->lock();
                data->async_senders->operator[](data->users->at(to))->SendAsync(AlterTimestamp(message_factory.GenerateTextMessage("Anonymous", message), time));
                data->senders_mutex->unlock();
            }
            else{
                data->offline_msg_mutex->lock();
                data->offline_msg_queue->operator[](to)->push(AlterTimestamp(message_factory.GenerateTextMessage("Anonymous", message), time));
                data->offline_msg_mutex->unlock();
            }
        }
        data->users_mutex->unlock();

    }
    else{ //send to a specific person
        std::string to = std::string(dcstr);
        data->users_mutex->lock();
        if(data->users->find(to) != data->users->end()){
            if(data->users->at(to) > 1){
                data->senders_mutex->lock();
                data->async_senders->operator[](data->users->at(to))->SendAsync(AlterTimestamp(message_factory.GenerateTextMessage("Anonymous", message), time));
                data->senders_mutex->unlock();
            }
            else{
                data->offline_msg_mutex->lock();
                data->offline_msg_queue->operator[](to)->push(AlterTimestamp(message_factory.GenerateTextMessage("Anonymous", message), time));
                data->offline_msg_mutex->unlock();
            }
        }
        data->users_mutex->unlock();
    }
    
}

//send a file message to others (and save it here too)
void ServerThread::HandleFileMessage(std::string dest, std::string filename, char* file, size_t file_size, unsigned int time){
    if(status != STSTATUS_ACTIVE){
        data->senders_mutex->lock();
        data->async_senders->operator[](sock)->SendAsync(message_factory.GenerateTextMessage(std::string("*SERVER*"), std::string("You must be logged in to do that!")));
        data->senders_mutex->unlock();
        return;
    }
    const char* dcstr = dest.c_str();
    PrintFileMessage(user, dest, filename, file, file_size, time);
    if(dcstr[0] == '@'){ //send to all registered people
        data->senders_mutex->lock();
        for(auto pair : *data->users){
            if(pair.second < 1) continue;
            data->async_senders->operator[](pair.second) ->SendAsync(AlterTimestamp(message_factory.GenerateFileMessage(user, filename), time));
        }
        data->senders_mutex->unlock();
        data->offline_msg_mutex->lock();
        for(auto pair : *data->offline_msg_queue){
            pair.second->push(AlterTimestamp(message_factory.GenerateFileMessage(user, filename), time));
        }
        data->offline_msg_mutex->unlock();
        return;
    }
    else{ //send to a specific person
        std::string to = std::string(dcstr);
        data->users_mutex->lock();
        if(data->users->find(to) != data->users->end()){
            if(data->users->at(to) > 0){
                data->senders_mutex->lock();
                data->async_senders->operator[](data->users->at(to))->SendAsync(AlterTimestamp(message_factory.GenerateFileMessage(user, filename), time));
                data->senders_mutex->unlock();
            }
            else{
                data->offline_msg_mutex->lock();
                data->offline_msg_queue->operator[](to)->push(AlterTimestamp(message_factory.GenerateFileMessage(user, filename), time));
                data->offline_msg_mutex->unlock();
            }
            data->users_mutex->unlock();
        }
        else{
            data->users_mutex->unlock();
            data->senders_mutex->lock();
            data->async_senders->operator[](sock)->SendAsync(message_factory.GenerateTextMessage(std::string("*SERVER*"), std::string("User not registered. Make sure you spelled their username correctly.")));
            data->senders_mutex->unlock();
        }
    }
    
}

//get a list of online users and send it back to the user who requested the list
void ServerThread::HandleListRequest(){
    data->credentials_mutex->lock();
    std::string list_output = "";
    for(auto pair : *data->users){
        if(pair.second > 0) list_output += "\n" + pair.first;
    }
    data->credentials_mutex->unlock();
    data->async_senders->operator[](sock)->SendAsync(message_factory.GenerateTextMessage(std::string("ONLINE USERS"), list_output));
}

//handle the user logging in (check credentials, logout of other clients)
void ServerThread::HandleLogin(std::string username, size_t password){
    if(user != ""){
        data->async_senders->operator[](sock)->SendAsync(message_factory.GenerateTextMessage(std::string("*SERVER*"), std::string("You must log out before you can log in again!")));
        return;
    }
    data->credentials_mutex->lock();
    if(data->credentials->find(username) != data->credentials->end()){ //this username exists
        if(data->credentials->operator[](username) == password){ //username and password match
            data->users_mutex->lock();
            if(data->users->operator[](username) > 0){ //if the user is logged in somewhere else, log them out there
                data->reader_threads->operator[](data->users->operator[](username))->HandleLogout(false);
            }
            data->users->operator[](username) = sock;
            status = STSTATUS_ACTIVE;
            user = username;
            std::cout << "[" << GetTimestamp(time(NULL)) << "] " << "User: " << user << " logged in" << std::endl; 
            data->users_mutex->unlock();
            data->credentials_mutex->unlock();
            data->offline_msg_mutex->lock();
            while(data->offline_msg_queue->operator[](user)->size() > 0){
                message_t* msg = data->offline_msg_queue->operator[](user)->front();
                data->offline_msg_queue->operator[](user)->pop();
                data->async_senders->operator[](sock)->SendAsync(msg);
            }
            delete data->offline_msg_queue->operator[](user);
            data->offline_msg_queue->erase(user);
            data->offline_msg_mutex->unlock();
        }
        else{
            data->credentials_mutex->unlock();
            std::cout << "[" << GetTimestamp(time(NULL)) << "] " << "User: " << user << " log in failed" << std::endl; 
            data->async_senders->operator[](sock)->SendAsync(message_factory.GenerateTextMessage(std::string("*SERVER*"), std::string("Username and password do not match")));
        }
    }
    else{
        data->credentials_mutex->unlock();
        std::cout << "[" << GetTimestamp(time(NULL)) << "] " << "User: " << user << " log in failed" << std::endl;
        data->async_senders->operator[](sock)->SendAsync(message_factory.GenerateTextMessage(std::string("*SERVER*"), std::string("An account with that username was not found")));
    }
}


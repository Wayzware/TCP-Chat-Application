// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#include "gchat_client.h"

Client::Client(std::string ip, int port, std::string script){
    script_file = script;
    server_ip = ip;
    this->port = port;
    connected = 0;
    sock = 0;
    kill_listener = 0;
    async_sender = NULL;
    //HandleMessage(message_factory.GenerateTextMessage("hello", "this is a test"));
}

void Client::Run(){
    std::ifstream script_istream (script_file, std::ifstream::in);
    if(!script_istream.is_open()){
        std::cout << "Could not open script file" << std::endl;
        return;
    }
    char buffer[4096] = {};
    buffer[4096] = '\0';
    std::vector<std::string> commands; //stores the commands to be executed

    while(script_istream.getline(buffer, 4095)){
        commands.push_back(std::string(buffer));
    }
    script_istream.close();

    StartListener(); //starts the listener thread so we can listen to messages while we are sending our own

    for(std::string command : commands) Execute(command); //execute the commands
    return;

}

void Client::Execute(std::string command){
    std::vector<std::string> tokenized = TokenizeString(command, " ");
    std::string cmd = tokenized.at(0); //get the first word in the command
    dprint(cmd);
    if(cmd.compare("REGISTER") == 0){ //REGISTER <username> <password>
        if(tokenized.size() != 3){
            std::cout << "Invalid command: \"" << cmd << "\"" << std::endl;
            return;
        }
        message_t* msg = message_factory.GenerateRegisterRequest(tokenized.at(1), tokenized.at(2));
        if(SendMessage(msg)) std::cout << "Send " << cmd << " message failed" << std::endl;
    }
    else if(cmd.compare("LOGIN") == 0){ //LOGIN <username> <password>
        if(tokenized.size() != 3){
            std::cout << "Invalid command: \"" << cmd << "\"" << std::endl;
            return;
        }
        message_t* msg = message_factory.GenerateLoginRequest(tokenized.at(1), tokenized.at(2));
        if(SendMessage(msg)) std::cout << "Send " << cmd << " message failed" << std::endl;
    }
    else if(cmd.compare("LOGOUT") == 0){ //LOGOUT
        message_t* msg = message_factory.GenerateLogoutRequest();
        if(SendMessage(msg)) std::cout << "Send " << cmd << " message failed" << std::endl;
    }
    else if(cmd.compare("SEND") == 0){ //SEND <msg>
        std::string text;
        if(tokenized.size() < 2){
            std::cout << "Invalid command: \"" << cmd << "\"" << std::endl;
            return;
        }
        for(size_t i = 1; i < tokenized.size(); i++){
            text += " " + tokenized.at(i);
        }
        message_t* msg = message_factory.GenerateTextMessage("@", text);
        if(SendMessage(msg)) std::cout << "Send " << cmd << " message failed" << std::endl;
    }
    else if(cmd.compare("SEND2") == 0){ //SEND2 <username> <msg>
        std::string text;
        if(tokenized.size() < 3){
            std::cout << "Invalid command: \"" << cmd << "\"" << std::endl;
            return;
        }
        for(size_t i = 2; i < tokenized.size(); i++){
            text += " " + tokenized.at(i);
        }
        message_t* msg = message_factory.GenerateTextMessage(tokenized.at(1), text);
        if(SendMessage(msg)) std::cout << "Send " << cmd << " message failed" << std::endl;
    }
    else if(cmd.compare("SENDA") == 0){ //SENDA <msg>
        std::string text;
        if(tokenized.size() < 2){
            std::cout << "Invalid command: \"" << cmd << "\"" << std::endl;
            return;
        }
        for(size_t i = 1; i < tokenized.size(); i++){
            text += " " + tokenized.at(i);
        }
        message_t* msg = message_factory.GenerateTextMessage("$", text);
        if(SendMessage(msg)) std::cout << "Send " << cmd << " message failed" << std::endl;
    }
    else if(cmd.compare("SENDA2") == 0){ //SENDA2 <username> <msg>
        std::string text;
        if(tokenized.size() < 3){
            std::cout << "Invalid command: \"" << cmd << "\"" << std::endl;
            return;
        }
        for(size_t i = 2; i < tokenized.size(); i++){
            text += " " + tokenized.at(i);
        }
        message_t* msg = message_factory.GenerateTextMessage("?" + tokenized.at(1), text);
        if(SendMessage(msg)) std::cout << "Send " << cmd << " message failed" << std::endl;
    }
    else if(cmd.compare("SENDF") == 0){ //SENDF <localfile>
        if(tokenized.size() != 2){
            std::cout << "Invalid command: \"" << cmd << "\"" << std::endl;
            return;
        }
        message_t* msg = message_factory.GenerateFileMessage("@", tokenized.at(1));
        if(SendMessage(msg)) std::cout << "Send " << cmd << " message failed" << std::endl;
    }
    else if(cmd.compare("SENDF2") == 0){ //SENDF2 <username> <localfile>
        if(tokenized.size() != 3){
            std::cout << "Invalid command: \"" << cmd << "\"" << std::endl;
            return;
        }
        message_t* msg = message_factory.GenerateFileMessage(tokenized.at(1), tokenized.at(2));
        if(SendMessage(msg)) std::cout << "Send " << cmd << " message failed" << std::endl;
    }
    else if(cmd.compare("LIST") == 0){ //LIST
        message_t* msg = message_factory.GenerateListRequest();
        if(SendMessage(msg)) std::cout << "Send " << cmd << " message failed" << std::endl;
    }
    else if(cmd.compare("DELAY") == 0){ //DELAY <seconds>
        if(tokenized.size() != 2){
            std::cout << "Invalid command: \"" << cmd << "\"" << std::endl;
            return;
        }
        sleep(strtoul(tokenized.at(1).c_str(), nullptr, 10));
    }
    else{
        std::cout << "Invalid command: \"" << cmd << "\"" << std::endl;
    }
}


int Client::SendMessage(message_t* message){
    //send message here
    if(!connected) {
        std::cout << "No connection" << std::endl;
        delete message->payload;
        delete message;
        return 1;
    }

    if(async_sender == NULL){
        async_sender = new AsyncSender(sock, &connected);
    }
    async_sender->SendAsync(message); //send the message non-blocking (for this thread at least)

    return 0;
}

void Client::InitSocket(){
    struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons((unsigned short) port);
    inet_pton(AF_INET, server_ip.c_str(), &serverAddr.sin_addr);
    sock = socket(AF_INET, SOCK_STREAM, 0);
}

int Client::Connect(){
    struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons((unsigned short) port);
    inet_pton(AF_INET, server_ip.c_str(), &serverAddr.sin_addr);
    int ret_val = connect(sock, (const struct sockaddr*) &serverAddr, sizeof(serverAddr));
    //std::cout << "CONNECT:" << ret_val << std::endl;
    connected = !ret_val;
    return ret_val;
}

void Client::ListenerMain(){
    sigset_t mask;
    sigfillset(&mask);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    struct pollfd pfd;
    pfd.fd = sock;
    pfd.events = POLLIN | POLLRDHUP | POLLHUP;
    if(false){
        printf("Listener could not establish connection to server\n");
        return;
    }
    while(!kill_listener){
        int ret = poll(&pfd, 1, 100); //listen to the socket for 100 ms or until we detect a message
        if(kill_listener) return; //exit listener
        if(!ret){ //if we timed out
            if(!connected){ //make sure we are still connected
                if(!RebootConnection()) continue;
                printf("Listener lost connection to server and could not re-establish connection (code: 1)\n");
                exit(0);
                return;
            }
        }
        if(pfd.revents & POLLIN){ //there is something to read on the socket
            message_t* msg = ReadMessage(sock);
            if(!msg) { //check to see if we were able to read a valid message
                connected = false; //if we werent, reconnect and make sure this is a valid connection
                if(!RebootConnection()) continue;
                printf("Listener lost connection to server and could not re-establish connection (code: 3)\n");
                exit(0);
                return;
            }
            HandleMessage(msg); //since the message was valid, handle it
        }
        else if(pfd.revents & (POLLRDHUP | POLLHUP)){ //if the connection was interrupted
            if(!RebootConnection()) continue; //try to reboot the connection
            std::cout << "Listener lost connection to server and could not re-establish connection (code: 4)\n" << std::endl;
            exit(0);
            return;
        }
        else{
            if(!connected){
                if(!RebootConnection()) continue;
                printf("Listener lost connection to server and could not re-establish connection (code: 2)\n");
                exit(0);
                return;
            }
        }
    }
    exit(0);
    return;
}

int Client::RebootConnection(){ //restart the connection
    if(sock) close(sock);
    sock = 0;
    InitSocket();
    return Connect();
}

void Client::HandleMessage(message_t* message){ //after we recv a message, we need to process it
    if(!message) return;
    switch(message->type){
        case TEXT_PAYLOAD:{ //if we got a text message, extract the payload and print it
            TextMessage* tmsg = (TextMessage*) message_factory.ExtractPayload(message);
            if(!tmsg) goto error;
            PrintTextMessage(tmsg, GetTimestamp(message));
            delete tmsg;
            break;
        }
        case FILE_PAYLOAD:{ //if we got a file message, extract the payload and print it
            FileMessage* fmsg = (FileMessage*) message_factory.ExtractPayload(message);
            if(!fmsg) goto error;
            PrintFileMessage(fmsg, message->payload_size - MAX_CRED_SIZE - MAX_FILENAME_SIZE, GetTimestamp(message));
            delete[] fmsg->file;
            delete fmsg;
            break;
        }
        default:{
            error: //if we errored out earlier, end up here too
            printf("Invalid message or message type recv'd\n");
            break;
        }
    }
    delete message;
    return;
}

int Client::StartListener(){
    InitSocket();
    Connect();
    listener_thread = new std::thread(&Client::ListenerMain, this);
    return !!listener_thread;
}

Client::~Client(){
    kill_listener = true;
    if(async_sender) delete async_sender;
    close(sock);
    listener_thread->join();
    delete listener_thread;

}

// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#include "gchat_server.h"

Server::Server(unsigned short port){
    this->port = port;
    shutdown = 0;
    reader_threads = {};
    async_senders = {};
    users = {};
    offline_msg_queue = {};
    credentials = {};

    data.async_senders = &async_senders;
    data.senders_mutex = &senders_mutex;
    data.users = &users;
    data.users_mutex = &users_mutex;
    data.offline_msg_queue = &offline_msg_queue;
    data.offline_msg_mutex = &offline_msg_mutex;
    data.credentials = &credentials;
    data.credentials_mutex = &credentials_mutex;
    data.port = &port;
    data.shutdown = &shutdown;
    data.reader_threads = &reader_threads;
}

Server::~Server(){
    SaveData();
    if(readers_mutex.try_lock()) {
        dprint("reader");
        for(auto pair : reader_threads) delete pair.second;
        readers_mutex.unlock();
    }
    if(senders_mutex.try_lock()){
        dprint("sender");
        for(auto pair : async_senders) delete pair.second;
        senders_mutex.unlock();
    }
    if(offline_msg_mutex.try_lock()){
        dprint("offline");
        for(auto pair : offline_msg_queue){
            for(int size = pair.second->size(); size > 0; size--){
                message_t* msg = pair.second->front();
                delete[] msg->payload;
                delete msg;
                pair.second->pop();
            }
            delete pair.second;
        }
        offline_msg_mutex.unlock();
    }
}

void Server::LoadData(){
    credentials_mutex.lock();
    credentials.clear();
    std::vector<std::string> lines = ReadLines(SERVER_DATAFILE); //read the datafile
    for(std::string line : lines){
        std::vector<std::string> parts = TokenizeString(line, "|"); //tokenize the line
        if(parts.size() != 2){
            std::cout << "Invalid line in datafile" << std::endl;
            continue;
        }
        credentials[parts.at(0)] = std::stoul(parts.at(1)); //place it into the credentials dictionary
    }
    std::cout << "Loaded " << credentials.size() << " credentials" << std::endl;
    credentials_mutex.unlock();
}

void Server::SaveData(){
    credentials_mutex.lock();
    char buff[65536] = {};
    char* buff_offset = buff;
    for(auto pair : credentials){ //build the credential datafile string with each line formatted as: username|password_hash
        buff_offset += sprintf(buff_offset, "%s|%lu\n", pair.first.c_str(), pair.second);
    }
    FILE* fp = fopen(SERVER_DATAFILE, "w");
    fwrite(buff, buff_offset - buff, 1, fp); //save the file
    fclose(fp);
    std::cout << "\nSaved " << credentials.size() << " credentials to file" << std::endl;
    credentials_mutex.unlock();
}

//runs on main thread
void Server::Run(){
    LoadData();
    for(auto pair : credentials){
        offline_msg_queue[pair.first] = new std::queue<message_t*>();
    }
    if(InitListenSocket()){
        std::cout << "Could not listen on port " << port << ", aborting" << std::endl;
        return;
    }
    std::cout << "Server started listening on port " << port << std::endl;
    struct pollfd pfd;
    pfd.fd = listen_sock;
    pfd.events = POLLIN | POLLRDHUP | POLLHUP;
    size_t save_ticks = 0;

    for(;;){
        int ret = poll(&pfd, 1, SERVER_POLL_TIMEOUT); //listen for new connection requests
        if(shutdown) return;
        if(!pfd.revents){
            if(++save_ticks > 6000){ //save the credentials every 10 minutes
                SaveData(); 
                save_ticks = 0;
            }
            continue;
        }
        if(pfd.revents & POLLIN){
            AcceptConnection(); //accept the connection and create a new ServerThread to listen on it
        }
        else if(pfd.revents & (POLLRDHUP | POLLHUP)){
            std::cout << "Listening socket closed, closing" << std::endl;
            return;
        }
        else{
            std::cout << pfd.revents << std::endl;
        }
    }
}

int Server::InitListenSocket(){
    struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(struct sockaddr_in));	
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//Create a "listening" socket, binds it with the above address/port
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	bind(listen_sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr));	
	return listen(listen_sock, 16);
}

void Server::AcceptConnection(){
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);			
    int sock = accept(listen_sock, (struct sockaddr *)&clientAddr, &clientAddrLen);
    readers_mutex.lock();
    reader_threads[sock] = new ServerThread(&data, sock); //give the socket its own threads and return back to listening for new connections on this one
    readers_mutex.unlock();
}
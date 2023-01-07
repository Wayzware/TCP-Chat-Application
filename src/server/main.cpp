// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#include "gchat_server.h"

Server* server = NULL;

void reset_server(){
    std::remove(SERVER_DATAFILE);
    exit(0);
}

void sigint_handler(int sig){
    delete server;
    exit(0);
}

int main(int argc, char** argv){
    signal(SIGINT, sigint_handler);
    if(argc != 2){
        std::cout << "Invalid arguments (Usage: ./server <port> or ./server reset)" << std::endl;
        return 1;
    }

    std::string arg(argv[1]);
    if(arg.compare("reset") == 0){
        reset_server();
    }

    int intport = std::stoi(arg);
    if((unsigned int) intport > 65536){
        std::cout << "Invalid port supplied as argument" << std::endl;
        return 1;
    }

    server = new Server((unsigned short) intport);
    
    server->Run();
    delete server;
    return 0;
}
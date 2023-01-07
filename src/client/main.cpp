// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#include "gchat_client.h"

Client* client = NULL;

void sigint_handler(int sig){
    exit(0);
}

int main(int argc, char** argv){
    if(argc != 4){
        std::cout << "Invalid arguments (Usage: ./client <server ip> <server port> <script path>)" << std::endl;
        return 1;
    }
    Client lclient(std::string(argv[1]), std::stoi(std::string(argv[2])), argv[3]);
    client = &lclient;
    client->Run();
    return 0;
}
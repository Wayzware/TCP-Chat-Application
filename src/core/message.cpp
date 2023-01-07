// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#include "message.h"
#include "debug.h"


void PrintFileMessage(FileMessage* payload, size_t file_size, std::string timestamp){
    FILE* file = fopen(payload->filename, "w");
    if(!file){
        std::cout << "**Could not open file for writing**" << std::endl;
        return;
    }
    fwrite(payload->file, file_size, 1, file);
    fclose(file);
    std::cout << "[" << timestamp << "] NEW FILE MESSAGE RECEIVED FROM: " << payload->dest << std::endl;
    std::cout << "File name: " << payload->filename << std::endl;
    std::cout << "File size: " << file_size << std::endl;

}

void PrintFileMessage(std::string user, std::string dest, std::string filename, char* file, size_t file_size, unsigned int timestamp){
    FILE* fil = fopen(filename.c_str(), "w");
    if(!file){
        std::cout << "**Could not open file for writing**" << std::endl;
        return;
    }
    fwrite(file, file_size, 1, fil);
    fclose(fil);
    std::cout << "[" << GetTimestamp(timestamp) << "] NEW FILE MESSAGE RECEIVED FROM: " << user << " to " << dest << std::endl;
    std::cout << "File name: " << filename << std::endl;
    std::cout << "File size: " << file_size << std::endl;
}

void PrintListMessage(ListMessage* payload, std::string timestamp){
    std::cout << "[" << timestamp << "] Currently online:" << std::endl;
    std::cout << payload->list << std::endl;
}

void PrintTextMessage(TextMessage* payload, std::string timestamp){
    std::cout << "[" << timestamp << "] " << payload->dest << ": " << payload->message << std::endl;
}
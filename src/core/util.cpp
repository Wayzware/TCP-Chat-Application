// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#include "util.h"

//makes strtok easier to use
std::vector<std::string> TokenizeString(std::string command, std::string delim){
    std::vector<std::string> tokenized;
    char command_cstr[4096] = {};
    strncpy(command_cstr, command.c_str(), 4095); 
    char* word = strtok(command_cstr, delim.c_str());
    while(word != NULL){
        tokenized.push_back(std::string(word));
        word = strtok(NULL, delim.c_str());
    }
    return tokenized;
}

//read all the lines from a file
std::vector<std::string> ReadLines(std::string filepath){
    std::ifstream script_istream (filepath, std::ifstream::in);
    if(!script_istream.is_open()){
        std::cout << "Could not open file " << filepath << std::endl;
        return std::vector<std::string>();
    }
    char* buffer = new char[10500000];
    memset(buffer, 0, 10500000);
    std::vector<std::string> commands;

    while(script_istream.getline(buffer, 10499999)){
        commands.push_back(std::string(buffer));
    }
    script_istream.close();
    delete[] buffer;
    return commands;
}

//read all the bytes from a file
size_t ReadFile(char* dest, std::string filepath, size_t max_bytes){
    std::ifstream script_istream (filepath, std::ifstream::in);
        if(!script_istream.is_open()){
        std::cout << "Could not open file " << filepath << std::endl;
        return 0;
    }
    script_istream.read(dest, max_bytes);
    size_t bytes_read = script_istream.gcount();
    script_istream.close();
    return bytes_read;
}
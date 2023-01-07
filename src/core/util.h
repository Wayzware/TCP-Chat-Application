// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

#ifndef UTIL_H
#define UTIL_H

#include <cstring>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

std::vector<std::string> TokenizeString(std::string command, std::string delim); //strtok wrapper
size_t ReadFile(char* dest, std::string filepath, size_t max_bytes); //read all the bytes from a file
std::vector<std::string> ReadLines(std::string filepath); //read all the lines from a file

#endif
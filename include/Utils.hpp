#ifndef UTILS_HPP
#define UTILS_HPP

#include <algorithm>
#include <limits.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sstream>

#define LOG_FILE "./logs/error.log"

std::string getMimeType(const std::string &path);
std::string normalizePath(const std::string &path);
std::string trim(const std::string &str);
void logError(const std::string &message);

// C++98 호환을 위한 int to string 변환 함수 선언
std::string intToString(int number);

#endif // UTILS_HPP

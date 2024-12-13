#ifndef UTILS_HPP
#define UTILS_HPP

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

std::string getMimeType(const std::string &path);
std::string normalizePath(const std::string &path);
std::string trim(const std::string &str);
void logError(const std::string &message);

// C++98 호환을 위한 int to string 변환 함수 선언
std::string intToString(int number);

#endif // UTILS_HPP

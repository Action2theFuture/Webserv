#ifndef UTILS_HPP
#define UTILS_HPP

#include "Define.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <cstring>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <netinet/in.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

std::string getMimeType(const std::string &path);
std::string normalizePath(const std::string &path);
std::string trim(const std::string &str);

// C++98 호환을 위한 int to string 변환 함수 선언
std::string intToString(int number);
std::string toLower(const std::string &str);
std::string sanitizeFilename(const std::string &filename);

bool isAllowedExtension(const std::string &filename, const std::vector<std::string> &allowed_extensions);
bool iequals(const std::string &a, const std::string &b);
bool readFromSocket(int client_socket, std::string &data, size_t content_length);

void trimString(std::string &str);

#endif // UTILS_HPP

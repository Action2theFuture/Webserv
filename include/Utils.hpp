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

#define LOG_FILE "./logs/error.log"

std::string getMimeType(const std::string &path);
std::string normalizePath(const std::string &path);
std::string trim(const std::string &str);
void logError(const std::string &message);

#endif // UTILS_HPP

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

std::string getMimeType(const std::string &path);
std::string normalizePath(const std::string &path);
void logError(const std::string &message);

#endif // UTILS_HPP

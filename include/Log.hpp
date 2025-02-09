#ifndef LOG_HPP
#define LOG_HPP

#include "Configuration.hpp"
#include "Define.hpp"
#include "ServerConfig.hpp"
#include <errno.h> // errno
#include <fstream> // ofstream
#include <iostream>
#include <string.h>   // strerror
#include <sys/stat.h> // stat 구조체와 mkdir

class LogConfig
{
  public:
    static std::string getCurrentTimeStr();
    // 서버 설정을 출력하는 함수
    static void printServerConfig(const ServerConfig &server, size_t index, std::ofstream &ofs);

    // 모든 서버 설정을 출력하는 함수
    static void printAllConfigurations(const std::vector<ServerConfig> &servers);

    static void ensureLogDirectoryExists();

    static void reportSuccess(int status, const std::string &message);
    static void reportError(int status, const std::string &message);
    static void reportInternalError(const std::string &message);
    static void printColoredMessage(const std::string &message, const char *time_str, const std::string &color);
};

#endif // LOG_HPP

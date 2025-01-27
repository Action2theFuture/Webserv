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
    // 서버 설정을 출력하는 함수
    static void printServerConfig(const ServerConfig &server, size_t index);

    // 모든 서버 설정을 출력하는 함수
    static void printAllConfigurations(const std::vector<ServerConfig> &servers);

    static void logError(const std::string &message);
    static void ensureLogDirectoryExists();
};

#endif // LOG_HPP

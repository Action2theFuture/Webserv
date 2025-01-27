#ifndef SOCKETMANAGER_HPP
#define SOCKETMANAGER_HPP

#include "Define.hpp"
#include "Log.hpp"
#include "Utils.hpp"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

class SocketManager
{
  public:
    static int createSocket(int port);
    static void setSocketOptions(int sockfd, int port);
    static void setSocketNonBlocking(int sockfd, int port);
    static void bindSocket(int sockfd, int port);
    static void startListening(int sockfd, int port);
    static bool readFromSocketOnce(int client_socket, std::string &data);
};

#endif // SOCKETMANAGER_HPP

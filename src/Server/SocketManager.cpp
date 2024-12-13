#include "SocketManager.hpp"
#include "Utils.hpp"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int SocketManager::createSocket(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        logError("socket() failed for port " + std::to_string(port));
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void SocketManager::setSocketOptions(int sockfd, int port)
{
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        logError("setsockopt() failed for port " + std::to_string(port));
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

void SocketManager::setSocketNonBlocking(int sockfd, int port)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
    {
        logError("fcntl(F_GETFL) failed for port " + std::to_string(port));
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        logError("fcntl(F_SETFL) failed for port " + std::to_string(port));
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

void SocketManager::bindSocket(int sockfd, int port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        logError("bind() failed for port " + std::to_string(port));
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

void SocketManager::startListening(int sockfd, int port)
{
    if (listen(sockfd, SOMAXCONN) < 0)
    {
        logError("listen() failed for port " + std::to_string(port));
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

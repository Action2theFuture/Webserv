#include "SocketManager.hpp"

int SocketManager::createSocket(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::string errMsg = "socket() failed for port " + intToString(port);
        LogConfig::reportInternalError(errMsg);
        throw std::runtime_error(errMsg);
    }
    return sockfd;
}

void SocketManager::setSocketNonBlocking(int sockfd, int port)
{
    (void)port;
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
    {
        std::string errMsg = "fcntl(F_GETFL) failed: " + std::string(strerror(errno));
        LogConfig::reportInternalError(errMsg);
        close(sockfd);
        throw std::runtime_error(errMsg);
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        std::string errMsg = "fcntl(F_SETFL) failed: " + std::string(strerror(errno));
        LogConfig::reportInternalError(errMsg);
        close(sockfd);
        throw std::runtime_error(errMsg);
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
        std::string errMsg = "bind() failed for port " + intToString(port) + ": " + strerror(errno);
        if (errno == EADDRINUSE)
            errMsg += " (Port already in use)";
        LogConfig::reportInternalError(errMsg);
        close(sockfd);
        throw std::runtime_error(errMsg);
    }
}

void SocketManager::startListening(int sockfd, int port)
{
    if (listen(sockfd, SOMAXCONN) < 0)
    {
        std::string errMsg = "listen() failed for port " + intToString(port) + ": " + strerror(errno);
        LogConfig::reportInternalError(errMsg);
        close(sockfd);
        throw std::runtime_error(errMsg);
    }
}

bool SocketManager::readFromSocketOnce(int client_socket, std::string &data)
{
    char buf[BUFFER_SIZE];
    ssize_t ret = recv(client_socket, buf, sizeof(buf), 0);
    if (ret > 0)
    {
        data.append(buf, ret);
        return true; // 데이터 읽음
    }
    else if (ret == 0)
    {
        // 클라이언트 종료
        return false;
    }
    else
    {
        return false;
    }
}
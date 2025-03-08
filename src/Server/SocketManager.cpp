#include "SocketManager.hpp"

int SocketManager::createSocket(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        LogConfig::reportInternalError("socket() failed for port " + intToString(port));
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void SocketManager::setSocketNonBlocking(int sockfd, int port)
{
    (void)port;
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
    {
        LogConfig::reportInternalError("fcntl(F_GETFL) failed: " + std::string(strerror(errno)));
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        LogConfig::reportInternalError("fcntl(F_SETFL) failed: " + std::string(strerror(errno)));
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
        std::string errorMsg = "bind() failed for port " + intToString(port) + ": " + strerror(errno);
        if (errno == EADDRINUSE)
            errorMsg += " (Port already in use)";
        LogConfig::reportInternalError(errorMsg);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

void SocketManager::startListening(int sockfd, int port)
{
    if (listen(sockfd, SOMAXCONN) < 0)
    {
        LogConfig::reportInternalError("listen() failed for port " + intToString(port) + ": " + strerror(errno));
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

bool SocketManager::readFromSocketOnce(int client_socket, std::string &data)
{
    char buf[4096];
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
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            // 더 읽을 데이터 없음
            // 특별히 false를 주면 상위 로직에서 close할 수도 있으니 주의
            return true;
        // 오류
        return false;
    }
}
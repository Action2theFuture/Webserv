#include "Server.hpp"
#include <cstring>
#include <errno.h>
#include <iostream>

void Server::processEvents(const std::vector<Event> &events)
{
    for (size_t i = 0; i < events.size(); ++i)
    {
        int fd = events[i].fd;
        if (fd < 0)
        {
            LogConfig::reportInternalError("Invalid file descriptor in events[" + intToString(i) + "]");
            continue;
        }
        if (events[i].events & POLLER_READ)
        {
            ServerConfig *matched_server = 0;
            if (isServerSocket(fd, &matched_server))
                handleNewConnection(fd);
            else
                handleClientRead(fd, findMatchingServerConfig(fd));
        }
        if (events[i].events & POLLER_WRITE)
            handleClientWrite(fd);
    }
}

void Server::handleNewConnection(int server_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        LogConfig::reportInternalError("accept() failed: " + std::string(strerror(errno)));
        return;
    }
    if (!setNonBlocking(client_fd))
    {
        LogConfig::reportInternalError("Failed to set non-blocking mode for client_fd " + intToString(client_fd));
        close(client_fd);
        return;
    }
    if (!_poller->add(client_fd, POLLER_READ))
    {
        LogConfig::reportInternalError("Failed to add client_fd " + intToString(client_fd) + " to poller");
        close(client_fd);
    }
}

void Server::handleClientRead(int client_fd, const ServerConfig &server_config)
{
    std::string &buffer = _partialRequests[client_fd];
    if (!readClientData(client_fd, buffer))
    {
        safelyCloseClient(client_fd);
        _partialRequests.erase(client_fd);
        return;
    }
    if (!handleReceivedData(client_fd, server_config, buffer))
    {
        return;
    }
}

bool Server::readClientData(int client_fd, std::string &buffer)
{
    while (true)
    {
        char tmp[BUFFER_SIZE];
        ssize_t bytes_read = recv(client_fd, tmp, sizeof(tmp), 0);
        if (bytes_read > 0)
        {
            buffer.append(tmp, bytes_read);
        }
        else if (bytes_read == 0)
        {
            return false;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else if (errno == ECONNRESET)
                return false;
            else
            {
                std::cerr << "recv() failed on fd " << client_fd << ": " << strerror(errno) << std::endl;
                return false;
            }
        }
    }
    return true;
}

bool Server::handleReceivedData(int client_fd, const ServerConfig &server_config, std::string &buffer)
{
    while (true)
    {
        int consumed = 0;
        bool ok = processClientRequest(client_fd, server_config, buffer, consumed);
        if (!ok)
        {
            // 에러 응답 or 즉시 닫아야 할 경우
            break;
        }
        else if (consumed == 0)
        {
            // partial → 더 수신 필요 or 추가 요청 없음
            break;
        }
        else
        {
            // 제대로 한 요청 분량을 처리했으므로, 그만큼 지운다.
            buffer.erase(0, consumed);
            // 비워졌으면 추가 요청 없음
            if (buffer.empty())
                break;
        }
    }
    if (_partialRequests.find(client_fd) != _partialRequests.end())
    {
        // 만약 close가 필요한 상황이면(응답 끝), safelyCloseClient
        // → 예: processClientRequest가 false 반환 시
        //       => break로 넘어왔을 때 close
        //       => map erase
        safelyCloseClient(client_fd);
        _partialRequests.erase(client_fd);
    }

    if (_requestMap.find(client_fd) != _requestMap.end())
        _requestMap.erase(client_fd);

    if (_outgoingData.find(client_fd) != _outgoingData.end())
        _outgoingData.erase(client_fd);

    // 최종적으로 handleClientRead가 false 반환 시 상위에서 뭔가 할 수도 있지만,
    // 여기서는 true로 반환 (더 이상 읽을게 없다는 뜻)
    return true;
}

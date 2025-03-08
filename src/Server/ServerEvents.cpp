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
        safelyCloseClient(client_fd);
        _partialRequests.erase(client_fd);
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
        if (!processClientRequest(client_fd, server_config, buffer, consumed))
            return false;
        else if (consumed == 0)
            break;
        else
        {
            buffer.erase(0, consumed);
            if (buffer.empty())
                break;
        }
    }
    return true;
}

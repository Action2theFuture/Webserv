#include "Server.hpp"
#include <cstring>
#include <errno.h>
#include <iostream>

void Server::processEvents(const std::vector<Event> &events)
{
    std::set<int>& closed_fds = getClosedFds();
    for (size_t i = 0; i < events.size(); ++i)
    {
        int fd = events[i].fd;
        // FD가 이미 닫힌 경우는 건너뜁니다.
        if (closed_fds.find(fd) != closed_fds.end())
            continue;
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
    static std::map<int, time_t> closed_fds;
    time_t now = time(NULL);
    for (std::map<int, time_t>::iterator it = closed_fds.begin(); it != closed_fds.end(); ) {
        if (now - it->second > 600)
            closed_fds.erase(it++); // C++98 방식: erase 후 it 증가
        else
            ++it;
    }
    if (closed_fds.find(client_fd) != closed_fds.end())
        return true;
 

    while (true)
    {
        int consumed = 0;
        bool ok = processClientRequest(client_fd, server_config, buffer, consumed);
        if (!ok)
        {
            // 치명적 에러가 발생하면 해당 연결을 종료합니다.
            safelyCloseClient(client_fd);
            _partialRequests.erase(client_fd);
            if (_requestMap.find(client_fd) != _requestMap.end())
                _requestMap.erase(client_fd);
            if (_outgoingData.find(client_fd) != _outgoingData.end())
                _outgoingData.erase(client_fd);
            closed_fds[client_fd] = now;
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
    if (_requestMap.find(client_fd) != _requestMap.end())
    {
        Request req = _requestMap[client_fd];
        std::map<std::string, std::string> headers = req.getHeaders();
        std::string connHeader = "";
        if (headers.find("Connection") != headers.end())
            connHeader = headers["Connection"];
        connHeader = toLower(connHeader);

        bool keepAlive = false;
        std::string httpVersion = req.getHTTPVersion();

        if (httpVersion == "HTTP/1.1")
            keepAlive = (connHeader != "close");
        else if (httpVersion == "HTTP/1.0")
            keepAlive = (connHeader == "keep-alive");
        else
            keepAlive = false;

        if (!keepAlive)
        {
            safelyCloseClient(client_fd);
            _partialRequests.erase(client_fd);
            _requestMap.erase(client_fd);
            _outgoingData.erase(client_fd);
            closed_fds[client_fd] = now;
        }
        else
        {
            _partialRequests[client_fd].clear();
            _requestMap.erase(client_fd);
            _outgoingData.erase(client_fd);
        }
    }
    else
    {
        // requestMap에 해당 FD의 요청 정보가 없는 경우,
        // 이는 요청이 완전히 파싱되지 않았음을 의미할 수 있으므로,
        // 연결은 유지하고 추가 데이터를 기다립니다.
        // 필요에 따라 로깅을 추가할 수 있습니다.
        // 여기서는 아무런 추가 작업 없이 연결을 유지합니다.
    }
    return true;
}

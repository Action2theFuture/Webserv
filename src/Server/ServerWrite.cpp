#include "Server.hpp"
#include "ServerWriteHelper.hpp"
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <set>

void Server::writePendingData(int client_fd)
{
    static std::set<int> closed_fds;
    if (closed_fds.find(client_fd) != closed_fds.end())
        return;
    std::string &buf = _outgoingData[client_fd];
    if (!writePendingDataHelper(_poller.get(), client_fd, buf))
    {
        safelyCloseClient(client_fd);
        _outgoingData.erase(client_fd);
        // closed_fds.insert(client_fd);
        return;
    }
    if (!buf.empty())
        return;
    if (checkKeepAliveNeeded(client_fd))
    {
        if (!_poller->modify(client_fd, POLLER_READ))
        {
            if (errno != ENOENT)
                // 만약 errno가 ENOENT이면 이미 제거된 것으로 간주하고 무시할 수 있습니다.
                LogConfig::reportInternalError("writePendingData: Failed to reset to READ event for client_fd " +
                                               intToString(client_fd));
            safelyCloseClient(client_fd);
            _outgoingData.erase(client_fd);
            // closed_fds.insert(client_fd);
        }
    }
    else
    {
        safelyCloseClient(client_fd);
        _outgoingData.erase(client_fd);
    }
}

bool Server::checkKeepAliveNeeded(int client_fd)
{
    if (_requestMap.find(client_fd) == _requestMap.end())
        return false;
    Request &req = _requestMap[client_fd];
    std::string httpVersion = req.getHTTPVersion();
    std::map<std::string, std::string> headers = req.getHeaders();
    std::string connHeader = (headers.find("Connection") != headers.end()) ? headers["Connection"] : "";
    connHeader = toLower(connHeader);
    if (httpVersion == "HTTP/1.1" || httpVersion == "HTTP/2.0")
        return (connHeader != "close");
    else if (httpVersion == "HTTP/1.0")
        return (connHeader == "keep-alive");
    return false;
}

void Server::handleClientWrite(int client_fd)
{
    if (_outgoingData.find(client_fd) == _outgoingData.end() || _outgoingData[client_fd].empty())
    {
        _poller->modify(client_fd, POLLER_READ);
        return;
    }
    writePendingData(client_fd);
}

bool Server::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        return false;
    return true;
}

bool Server::isServerSocket(int fd, ServerConfig **matched_server) const
{
    for (size_t s = 0; s < _server_configs.size(); ++s)
    {
        for (size_t sock = 0; sock < _server_configs[s].server_sockets.size(); ++sock)
        {
            if (fd == _server_configs[s].server_sockets[sock])
            {
                if (matched_server)
                    *matched_server = const_cast<ServerConfig *>(&_server_configs[s]);
                return true;
            }
        }
    }
    return false;
}

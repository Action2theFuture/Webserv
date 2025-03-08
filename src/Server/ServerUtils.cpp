#include "Server.hpp"
#include <set>

extern const LocationConfig *matchLocationConfig(const Request &request, const ServerConfig &server_config);

ServerConfig &Server::findMatchingServerConfig(int fd)
{
    for (size_t s = 0; s < _server_configs.size(); ++s)
    {
        for (size_t sock = 0; sock < _server_configs[s].server_sockets.size(); ++sock)
        {
            if (fd == _server_configs[s].server_sockets[sock])
                return _server_configs[s];
        }
    }
    return _server_configs[0];
}

void Server::safelyCloseClient(int client_fd)
{
    static std::set<int> closed_fds;
    if (closed_fds.find(client_fd) != closed_fds.end())
        return;
    _poller->remove(client_fd);
    close(client_fd);
    closed_fds.insert(client_fd);
}

bool Server::processClientRequest(int client_fd, const ServerConfig &server_config, const std::string &request_str,
                                  int &consumed)
{
    consumed = 0;
    bool isPartial = false;
    Request request;
    if (!request.parse(request_str, consumed, isPartial))
    {
        sendBadRequestResponse(client_fd, server_config);
        return false;
    }
    if (isPartial)
    {
        consumed = 0;
        return true;
    }
    _requestMap[client_fd] = request;
    const LocationConfig *matched_location = matchLocationConfig(request, server_config);
    if (matched_location == 0)
    {
        LogConfig::reportInternalError("No matching location found for path: " + request.getPath());
        Response res = Response::createErrorResponse(404, server_config);
        res.setHeader("Connection", "close");
        sendResponse(client_fd, res);
        return false;
    }
    Response res = Response::buildResponse(request, server_config, matched_location);
    res.setHeader("Connection", "close");
    sendResponse(client_fd, res);
    consumed = request_str.size();
    return true;
}

void Server::sendResponse(int client_fd, const Response &response)
{
    std::string response_str = response.toString();
    _outgoingData[client_fd] += response_str;
    writePendingData(client_fd);
}

void Server::sendBadRequestResponse(int client_fd, const ServerConfig &server_config)
{
    Response res;
    res.setStatus(BAD_REQUEST_404);
    std::string error_body = "<h1>400 Bad Request</h1>";
    if (server_config.error_pages.find(400) != server_config.error_pages.end())
        error_body = server_config.error_pages.at(400);
    res.setBody(error_body);
    res.setHeader("Content-Length", intToString(error_body.length()));
    res.setHeader("Content-Type", "text/html");
    sendResponse(client_fd, res);
}

#include "RequestHandler.hpp"
#include "Utils.hpp"
#include "CGIHandler.hpp"


void RequestHandler::sendResponse(int client_fd, const Response &response) {
    std::string response_str = response.toString();
    ssize_t total_sent = 0;
    ssize_t to_send = response_str.size();

    while (total_sent < to_send) {
        ssize_t sent = send(client_fd, response_str.c_str() + total_sent, to_send - total_sent, 0);
        if (sent == -1) {
            // C++98 방식으로 숫자 변환
            std::cerr << "send() failed for client_fd " << client_fd << ": " << strerror(errno) << std::endl;
            logError("send() failed for client_fd " + intToString(client_fd) + ": " + strerror(errno));
            break;
        }
        total_sent += sent;
    }
}

void RequestHandler::sendBadRequestResponse(int client_fd) {
    Response res;
    res.setStatus("400 Bad Request");
    res.setBody("<h1>400 Bad Request</h1>");
    res.setHeader("Content-Length", "17");
    res.setHeader("Content-Type", "text/html");
    sendResponse(client_fd, res);
}


bool RequestHandler::processClientRequest(int client_fd, const ServerConfig &server_config, const std::string &request_str) {
    Request request;
    if (!request.parse(request_str)) {
        sendBadRequestResponse(client_fd);
        return false;
    }

    const LocationConfig *matched_location = matchLocationConfig(request, server_config);
    Response response = generateResponse(request, server_config, matched_location);
    sendResponse(client_fd, response);
    return true;
}

const LocationConfig *RequestHandler::matchLocationConfig(const Request &request, const ServerConfig &server_config) {
    for (size_t loc = 0; loc < server_config.locations.size(); ++loc) {
        if (request.getPath().find(server_config.locations[loc].path) == 0) {
            return &server_config.locations[loc];
        }
    }
    return nullptr;
}

Response RequestHandler::generateResponse(const Request &request, const ServerConfig &server_config, const LocationConfig *location_config) {
    if (location_config) {
        return Response::createResponse(request, *location_config);
    }

    // 기본 위치 설정에 따른 처리
    LocationConfig default_location;
    default_location.path = "/";
    default_location.methods.push_back("GET");
    default_location.root = server_config.root;
    default_location.directory_listing = false;
    default_location.index = "./www/html/index.html";
    return Response::createResponse(request, default_location);
}

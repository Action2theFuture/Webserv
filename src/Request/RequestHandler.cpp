#include "RequestHandler.hpp"

void RequestHandler::sendResponse(int client_fd, const Response &response)
{
    std::string response_str = response.toString();
    ssize_t total_sent = 0;
    ssize_t to_send = response_str.size();

    while (total_sent < to_send)
    {
        ssize_t sent = send(client_fd, response_str.c_str() + total_sent, to_send - total_sent, 0);
        if (sent == -1)
        {
            // C++98 방식으로 숫자 변환
            std::cerr << "send() failed for client_fd " << client_fd << ": " << strerror(errno) << std::endl;
            LogConfig::logError("send() failed for client_fd " + numberToString(client_fd) + ": " + strerror(errno));
            break;
        }
        total_sent += sent;
    }
}

void RequestHandler::sendBadRequestResponse(int client_fd, const ServerConfig &server_config)
{
    Response res = ResponseHelpers::createErrorResponse(400, server_config);
    sendResponse(client_fd, res);
}

bool RequestHandler::processClientRequest(int client_fd, const ServerConfig &server_config,
                                          const std::string &request_str, int &consumed)
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
        // 아직 불완전 -> consumed=0
        consumed = 0;
        return true; // 소켓 유지
    }

    const LocationConfig *matched_location = matchLocationConfig(request, server_config);
    if (matched_location == NULL)
    {
        std::cerr << "No matching location found for path: " << request.getPath() << std::endl;
        Response res = ResponseHelpers::createErrorResponse(404, server_config);
        sendResponse(client_fd, res);
        return false;
    }
    // std::cout << "mached location path : " << matched_location->path << std::endl;
    Response response = generateResponse(request, server_config, matched_location);
    sendResponse(client_fd, response);

    consumed = request_str.size();
    return true;
}

const LocationConfig *RequestHandler::matchLocationConfig(const Request &request, const ServerConfig &server_config)
{
    const LocationConfig *matched_location = NULL;
    size_t longest_match = 0;
    std::string req_path_lower = toLower(request.getPath());

    for (size_t loc = 0; loc < server_config.locations.size(); ++loc)
    {
        std::string loc_path_lower = toLower(server_config.locations[loc].path);
        const std::string &req_path = req_path_lower;
        std::cout << "Matching request path: " << req_path << " with location path: " << loc_path_lower << std::endl;

        if (req_path.compare(0, loc_path_lower.length(), loc_path_lower) == 0)
        {
            // 추가 조건: 정확한 매칭 또는 다음 문자가 '/'
            if (req_path.length() == loc_path_lower.length() || req_path[loc_path_lower.length()] == '/')
            {
                // 가장 긴 매칭 경로를 선택하여 우선순위 처리
                if (loc_path_lower.length() > longest_match)
                {
                    matched_location = &server_config.locations[loc];
                    longest_match = loc_path_lower.length();
                    std::cout << "New matched location: " << loc_path_lower << std::endl;
                }
            }
        }
    }
    if (matched_location)
        std::cout << "Final matched location: " << matched_location->path << std::endl;
    else
        std::cout << "No matching location found for path: " << request.getPath() << std::endl;

    return matched_location;
}

Response RequestHandler::generateResponse(const Request &request, const ServerConfig &server_config,
                                          const LocationConfig *location_config)
{
    if (location_config)
    {
        return Response::createResponse(request, *location_config, server_config);
    }

    // 기본 위치 설정에 따른 처리
    LocationConfig default_location;
    default_location.path = "/";
    default_location.methods.push_back("GET");
    default_location.directory_listing = false;
    default_location.index = DEFAULT_INDEX_PATH;
    return Response::createResponse(request, default_location, server_config);
}
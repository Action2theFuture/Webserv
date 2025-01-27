#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "CGIHandler.hpp"
#include "Define.hpp"
#include "Log.hpp"
#include "Parser.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "ServerConfig.hpp"
#include "Utils.hpp"

class RequestHandler
{
  public:
    static bool processClientRequest(int client_fd, const ServerConfig &server_config, const std::string &request_str,
                                     int &consumed);
    static Response generateResponse(const Request &request, const ServerConfig &server_config,
                                     const LocationConfig *location_config);

  private:
    static const LocationConfig *matchLocationConfig(const Request &request, const ServerConfig &server_config);
    static void sendBadRequestResponse(int client_fd, const ServerConfig &server_config);
    static void sendResponse(int client_fd, const Response &response);
};

#endif // REQUESTHANDLER_HPP

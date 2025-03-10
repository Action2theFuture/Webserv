#ifndef RESPONSEHANDLERS_HPP
#define RESPONSEHANDLERS_HPP

#include "Configuration.hpp"
#include "Define.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "ServerConfig.hpp"
#include "Utils.hpp"

class ResponseHandler
{
  public:
    static Response handleRedirection(const LocationConfig &location_config);
    static Response handleCGI(const Request &request, const std::string &real_path, const ServerConfig &server_config);
    static Response handleStaticFile(const std::string &real_path, const ServerConfig &server_config);
    static Response handleUpload(const std::string &real_path, const Request &request,
                                 const LocationConfig &location_config, const ServerConfig &server_config);
    static Response handleFileList(const Request &request, const LocationConfig &location_config,
                                   const ServerConfig &server_config);
    static Response handleDeleteFile(const Request &request, const LocationConfig &location_config,
                                     const ServerConfig &server_config);
    static Response handleDeleteAllFiles(const LocationConfig &location_config, const ServerConfig &server_config);
    static Response handleGetFileList(const LocationConfig &location_config, const ServerConfig &server_config);
    static Response handleMethodNotAllowed(const LocationConfig &location_config, const ServerConfig &server_config);
    static Response handleQuery(const std::string &real_path, const Request &request, const ServerConfig &server_config);
    static Response handleCookieAndSession(const Request &request);
    static Response handlePost(const Request &request, const LocationConfig &location_config, const ServerConfig &server_config);
    // 추가: 메서드 유효성 검사 및 CGI 요청 여부 판단
    static bool validateMethod(const Request &request, const LocationConfig &location_config);
    static bool isCGIRequest(const std::string &real_path, const LocationConfig &location_config);
};

#endif // RESPONSEHANDLERS_HPP

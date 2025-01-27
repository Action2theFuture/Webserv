#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "CGIHandler.hpp" // For cgi_handler.execute if needed
#include "Configuration.hpp"
#include "Define.hpp"
#include "Log.hpp"
#include "Request.hpp"
#include "ResponseUtils.hpp"
#include "ServerConfig.hpp"
#include "Utils.hpp" // trim, normalizePath, etc.>

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <map>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

class Response
{
  public:
    Response();
    ~Response();

    // 정적 메서드로 응답 생성
    static Response createResponse(const Request &request, const LocationConfig &location_config,
                                   const ServerConfig &server_config);
    static Response buildResponse(const Request &request, const ServerConfig &server_config,
                                  const LocationConfig *location_config);
    static Response createErrorResponse(int status, const ServerConfig &server_config);
    // 응답을 문자열로 변환
    std::string toString() const;
    std::string getStatus() const;
    // 상태, 헤더, 본문 설정 메서드
    void setStatus(const std::string &status_code);
    void setHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &content);

    void setCookie(const std::string &key, const std::string &value, const std::string &path = "/", int max_age = 0);

  private:
    std::string status;
    std::map<std::string, std::string> headers;
    std::string body;
    static std::string readErrorPageFromFile(const std::string &file_path, int status);
    static bool validateMethod(const Request &request, const LocationConfig &location_config);
    static bool getRealPath(const std::string &path, const LocationConfig &location_config,
                            const ServerConfig &server_config, std::string &real_path);
    static bool isCGIRequest(const std::string &real_path, const LocationConfig &location_config);
    Response handleGetFileList(const LocationConfig &location_config, const ServerConfig &server_config);
    Response handleDeleteFile(const Request &request, const LocationConfig &location_config,
                              const ServerConfig &server_config);
};

#endif // RESPONSE_HPP

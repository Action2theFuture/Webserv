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
    static Response generateResponse(const Request &request, const ServerConfig &server_config,
                                     const LocationConfig *location_config);
    // createErrorResponse: 에러 응답 생성 (긴 함수, 내부적으로 서브함수 나눌 수도
    // 있음)
    static Response createErrorResponse(int status, const ServerConfig &server_config);

    // 응답을 문자열로 변환
    std::string toString() const;

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
};

#endif // RESPONSE_HPP

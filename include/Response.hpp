#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "CGIHandler.hpp" // For cgi_handler.execute if needed
#include "CGIHandler.hpp"
#include "Configuration.hpp"
#include "Define.hpp"
#include "Log.hpp"
#include "Request.hpp"
#include "ServerConfig.hpp"
#include "Utils.hpp" // trim, normalizePath, etc.>
#include "Utils.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <limits.h>
#include <map>
#include <sstream>
#include <string>
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
};

namespace ResponseHelpers
{
// createErrorResponse: 에러 응답 생성 (긴 함수, 내부적으로 서브함수 나눌 수도
// 있음)
Response createErrorResponse(int status, const ServerConfig &server_config);
// 메서드 허용 검사
bool isMethodAllowed(const std::string &method, const LocationConfig &location_config);
// 리디렉션 처리
Response handleRedirection(const LocationConfig &location_config);
// CGI 처리
Response handleCGI(const Request &request, const std::string &real_path, const ServerConfig &server_config);
// 정적 파일 처리
Response handleStaticFile(const std::string &real_path, const ServerConfig &server_config);
// 파일 업로드 처리
Response handleUpload(const std::string &real_path, const Request &request, const LocationConfig &location_config,
                      const ServerConfig &server_config);

std::string buildRequestedPath(const std::string &path, const LocationConfig &location_config,
                               const ServerConfig &server_config);
// 업로드 디렉토리 생성
bool ensureDirectoryExists(const std::string &fullPath);
bool createSingleDir(const std::string &path);

} // namespace ResponseHelpers

#endif // RESPONSE_HPP

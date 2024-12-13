#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Request.hpp"
#include "ServerConfig.hpp"

#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <string>
#include <map>

#define ROOT_DIRECTORY "./www"

class Response {
public:
    Response();
    ~Response();

    // 정적 메서드로 응답 생성
    static Response createResponse(const Request &request, const LocationConfig &location_config, const ServerConfig &server_config);

    // 응답을 문자열로 변환
    std::string toString() const;

    // 상태, 헤더, 본문 설정 메서드
    void setStatus(const std::string &status_code);
    void setHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &content);

private:
    std::string status;
    std::map<std::string, std::string> headers;
    std::string body;
};

#endif // RESPONSE_HPP

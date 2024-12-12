#include "Request.hpp"
#include <sstream>

Request::Request() : method("GET"), path("/"), query_string(""), headers() {}
Request::~Request() {}

std::string Request::getMethod() const {
    return method;
}

std::string Request::getPath() const {
    return path;
}

std::string Request::getQueryString() const {
    return query_string;
}

std::map<std::string, std::string> Request::getHeaders() const {
    return headers;
}

bool Request::parse(const std::string &data) {
    std::istringstream stream(data);
    std::string line;
    
    // 요청 라인 파싱
    if (!getline(stream, line))
        return false;
    if (!parseRequestLine(line))
        return false;
    
    // 헤더 파싱
    while (getline(stream, line) && line != "\r") {
        if (!parseHeaderLine(line))
            return false;
    }
    
    // 바디 파싱
    if (headers.find("Content-Length") != headers.end()) {
        int content_length = atoi(headers["Content-Length"].c_str());
        body.resize(content_length);
        stream.read(&body[0], content_length);
    }
    
    return true;
}

bool Request::parseRequestLine(const std::string &line) {
    std::istringstream iss(line);
    if (!(iss >> method >> path))
        return false;
    // 추가 파싱 (HTTP 버전 등)
    return true;
}

bool Request::parseHeaderLine(const std::string &line) {
    size_t pos = line.find(":");
    if (pos == std::string::npos)
        return false;
    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);
    // 공백 제거
    key.erase(key.find_last_not_of(" \t\r\n") + 1);
    value.erase(0, value.find_first_not_of(" \t\r\n"));
    value.erase(value.find_last_not_of(" \t\r\n") + 1);
    headers[key] = value;
    return true;
}
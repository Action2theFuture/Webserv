#include "Parser.hpp"
#include <cstdlib>

bool Parser::parse(const std::string &data, std::string &method, std::string &path, std::string &query_string,
                   std::map<std::string, std::string> &headers, std::string &body)
{
    std::istringstream stream(data);
    std::string line;

    // 요청 라인 파싱
    if (!getline(stream, line))
        return false;
    if (!parseRequestLine(line, method, path, query_string))
        return false;

    // 헤더 파싱
    while (getline(stream, line) && line != "\r")
    {
        if (!parseHeaderLine(line, headers))
            return false;
    }

    // 바디 파싱
    if (headers.find("Content-Length") != headers.end())
    {
        int content_length = atoi(headers["Content-Length"].c_str());
        body.resize(content_length);
        stream.read(&body[0], content_length);
    }

    return true;
}

bool Parser::parseRequestLine(const std::string &line, std::string &method, std::string &path,
                              std::string &query_string)
{
    std::istringstream iss(line);
    if (!(iss >> method >> path))
        return false;
    (void)query_string;
    // 추가 파싱 (HTTP 버전 등)
    return true;
}

bool Parser::parseHeaderLine(const std::string &line, std::map<std::string, std::string> &headers)
{
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

#include "HttpRequestParser.hpp"
#include "Utils.hpp" // trimString, urlDecode 등 유틸 함수 포함
#include <cstdlib>
#include <iostream>
#include <sstream>

Parser::Parser()
{
}
Parser::~Parser()
{
}

bool Parser::parse(const std::string &data, ParsedRequest &req)
{
    req.consumed = 0;
    req.isPartial = false;
    size_t line_end = data.find("\r\n");
    if (line_end == std::string::npos)
    {
        req.isPartial = true;
        return true;
    }
    std::string request_line = data.substr(0, line_end);
    if (!parseRequestLine(request_line, req))
        return false;
    size_t offset = line_end + 2;
    if (!parseHeaders(data, offset, req.headers, req.isPartial))
        return false;
    int content_length = 0;
    if (req.headers.find("Content-Length") != req.headers.end())
    {
        content_length = std::atoi(req.headers["Content-Length"].c_str());
        if (content_length < 0)
            content_length = 0;
    }
    size_t total_needed = offset + content_length;
    if (data.size() < total_needed)
    {
        req.isPartial = true;
        return true;
    }
    req.body = data.substr(offset, content_length);

    // 디버그: Content-Type 값 출력
    if (req.headers.find("Content-Type") != req.headers.end())
    {
        std::string ct = req.headers["Content-Type"];
        // 대소문자 무시, 앞뒤 공백 제거
        std::string ct_lower = toLower(trim(ct));
        if (ct_lower.find("multipart/form-data") != std::string::npos)
        {
            std::string boundary;
            if (!extractBoundary(ct, boundary))
                return false;
            if (!parseMultipartFormData(req.body, boundary, req))
                return false;
        }
    }
    req.consumed = total_needed;
    return true;
}

/* bool Parser::parseRequestLine(const std::string &line, ParsedRequest &req)
{
    size_t firstSpace = line.find(' ');
    if (firstSpace == std::string::npos)
        return false;
    size_t secondSpace = line.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos)
    {
        secondSpace = line.size();
        req.httpVersion = "HTTP/1.0";
    }
    else
    {
        req.httpVersion = line.substr(secondSpace + 1);
    }
    req.method = line.substr(0, firstSpace);
    std::string url = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    size_t qmark = url.find('?');
    if (qmark != std::string::npos)
    {
        req.query_string = url.substr(qmark + 1);
        req.path = url.substr(0, qmark);
        std::stringstream ss(req.query_string);
        std::string pair;
        while (std::getline(ss, pair, '&'))
        {
            size_t eq = pair.find('=');
            if (eq != std::string::npos)
            {
                std::string key = urlDecode(pair.substr(0, eq));
                std::string value = urlDecode(pair.substr(eq + 1));
                req.queryParams[key] = value;
            }
            else
            {
                std::string key = urlDecode(pair);
                req.queryParams[key] = "";
            }
        }
    }
    else
    {
        req.path = url;
        req.query_string.clear();
    }
    return true;
} */

bool Parser::parseRequestLine(const std::string &line, ParsedRequest &req)
{
    size_t firstSpace = line.find(' ');
    if (firstSpace == std::string::npos)
        return false;
    size_t secondSpace = line.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos)
    {
        secondSpace = line.size();
        req.httpVersion = "HTTP/1.0";
    }
    else
    {
        req.httpVersion = line.substr(secondSpace + 1);
    }
    req.method = line.substr(0, firstSpace);
    std::string url = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    size_t qmark = url.find('?');
    if (qmark != std::string::npos)
    {
        req.query_string = url.substr(qmark + 1);
        url = url.substr(0, qmark);
    }
    else
    {
        req.query_string.clear();
    }

    // Split URL into req.path and PATH_INFO
    size_t dot_pos = url.find_last_of('.');
    if (dot_pos != std::string::npos)
    {
        size_t slash_pos = url.find('/', dot_pos);
        if (slash_pos != std::string::npos)
        {
            std::string path_info = url.substr(slash_pos);
            setenv("PATH_INFO", path_info.c_str(), 1);
            req.path = url.substr(0, slash_pos);
        }
        else
        {
            setenv("PATH_INFO", "", 1); // Set PATH_INFO to empty if no slash found
            req.path = url;
        }
    }
    else
    {
        setenv("PATH_INFO", "", 1); // Set PATH_INFO to empty if no dot found
        req.path = url;
    }

    // Parse query parameters if present
    if (!req.query_string.empty())
    {
        std::stringstream ss(req.query_string);
        std::string pair;
        while (std::getline(ss, pair, '&'))
        {
            size_t eq = pair.find('=');
            if (eq != std::string::npos)
            {
                std::string key = urlDecode(pair.substr(0, eq));
                std::string value = urlDecode(pair.substr(eq + 1));
                req.queryParams[key] = value;
            }
            else
            {
                std::string key = urlDecode(pair);
                req.queryParams[key] = "";
            }
        }
    }

    return true;
}


bool Parser::parseHeaders(const std::string &data, size_t &offset, std::map<std::string, std::string> &headers,
                          bool &isPartial)
{
    while (1)
    {
        size_t next_end = data.find("\r\n", offset);
        if (next_end == std::string::npos)
        {
            isPartial = true;
            return true;
        }
        if (next_end == offset)
        {
            offset += 2;
            break;
        }
        std::string line = data.substr(offset, next_end - offset);
        if (!parseHeaderLine(line, headers))
            return false;
        offset = next_end + 2;
    }
    return true;
}

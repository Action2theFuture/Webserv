#include "HttpRequestParser.hpp"
#include "Utils.hpp" // trimString 함수 포함

bool Parser::parseHeaderLine(const std::string &line, std::map<std::string, std::string> &headers)
{
    size_t pos = line.find(":");
    if (pos == std::string::npos)
        return false;
    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);
    trimString(key);
    trimString(value);
    headers[key] = value;
    return true;
}

bool Parser::extractBoundary(const std::string &content_type, std::string &boundary)
{
    std::string ct = content_type;
    if (!ct.empty() && ct[ct.size() - 1] == ';')
        ct.erase(ct.size() - 1);
    trimString(ct);
    std::string prefix = "boundary=";
    size_t pos = ct.find(prefix);
    if (pos == std::string::npos)
        return false;
    boundary = ct.substr(pos + prefix.size());
    if (boundary.size() >= 2 && boundary[0] == '"' && boundary[boundary.size() - 1] == '"')
        boundary = boundary.substr(1, boundary.size() - 2);
    trimString(boundary);
    return true;
}

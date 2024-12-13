#include "Request.hpp"
#include "Parser.hpp"

Request::Request() : method("GET"), path("/"), query_string(""), headers()
{
}
Request::~Request()
{
}

std::string Request::getMethod() const
{
    return method;
}

std::string Request::getPath() const
{
    return path;
}

std::string Request::getQueryString() const
{
    return query_string;
}

std::map<std::string, std::string> Request::getHeaders() const
{
    return headers;
}

std::string Request::getBody() const
{
    return body;
}

bool Request::parse(const std::string &data)
{
    Parser parser;
    return parser.parse(data, method, path, query_string, headers, body);
}

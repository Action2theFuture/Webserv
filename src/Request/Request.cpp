#include "Request.hpp"

const std::vector<UploadedFile> &Request::getUploadedFiles() const
{
    return uploaded_files;
}

const std::map<std::string, std::string> &Request::getFormFields() const
{
    return form_fields;
}

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

std::map<std::string, std::string> Request::getQueryParams() const
{
    return queryParams;
}

std::map<std::string, std::string> Request::getHeaders() const
{
    return headers;
}

std::string Request::getBody() const
{
    return body;
}

bool Request::parse(const std::string &data, int &consumed, bool &isPartial)
{
    Parser parser;
    return parser.parse(data, method, path, query_string, queryParams, headers, body, uploaded_files, form_fields,
                        consumed, isPartial);
}
#include "Request.hpp"
#include "HttpRequestParser.hpp" // 새 Parser 인터페이스 포함

Request::Request() : _method("GET"), _path("/"), _query_string(""), _httpVersion("HTTP/1.0")
{
}

Request::~Request()
{
}

const std::vector<UploadedFile> &Request::getUploadedFiles() const
{
    return _uploaded_files;
}

const std::map<std::string, std::string> &Request::getFormFields() const
{
    return _form_fields;
}

std::string Request::getMethod() const
{
    return _method;
}

std::string Request::getPath() const
{
    return _path;
}

std::string Request::getQueryString() const
{
    return _query_string;
}

std::string Request::getHTTPVersion() const
{
    return _httpVersion;
}

std::map<std::string, std::string> Request::getQueryParams() const
{
    return _queryParams;
}

std::map<std::string, std::string> Request::getHeaders() const
{
    return _headers;
}

std::string Request::getBody() const
{
    return _body;
}

void Request::setUploadedFiles(const std::vector<UploadedFile> &files)
{
    _uploaded_files = files;
}

void Request::setFormFields(const std::map<std::string, std::string> &fields)
{
    _form_fields = fields;
}

void Request::setBody(const std::string &body_data)
{
    _body = body_data;
}

bool Request::parse(const std::string &data, int &consumed, bool &isPartial)
{
    // 새 HttpRequestParser 인터페이스를 사용하여 파싱합니다.
    Parser parser;
    ParsedRequest parsed;
    bool result = parser.parse(data, parsed);
    if (result)
    {
        _method = parsed.method;
        _path = parsed.path;
        _query_string = parsed.query_string;
        _queryParams = parsed.queryParams;
        _headers = parsed.headers;
        _body = parsed.body;
        _uploaded_files = parsed.uploaded_files;
        _form_fields = parsed.form_fields;
        _httpVersion = parsed.httpVersion;
        consumed = parsed.consumed;
        isPartial = parsed.isPartial;
    }
    return result;
}

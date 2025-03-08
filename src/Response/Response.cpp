#include "Response.hpp"
#include "Define.hpp"
#include "ResponseHandlers.hpp"
#include "ResponseUtils.hpp"
#include <iostream>
#include <limits.h>
#include <sstream>
#include <string>
#include <unistd.h>

Response::Response() : _status("200 OK"), _headers(), _body("")
{
}

Response::~Response()
{
}

void Response::setStatus(const std::string &status_code)
{
    _status = status_code;
}

void Response::setHeader(const std::string &key, const std::string &value)
{
    _headers[key] = value;
}

void Response::setBody(const std::string &content)
{
    _body = content;
}

std::string Response::getStatus() const
{
    return _status;
}

std::string Response::toString() const
{
    std::stringstream response_stream;
    response_stream << "HTTP/1.1 " << _status << "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
    {
        response_stream << it->first << ": " << it->second << "\r\n";
    }
    response_stream << "\r\n" << _body;
    return response_stream.str();
}

void Response::setCookie(const std::string &key, const std::string &value, const std::string &path, int max_age)
{
    std::stringstream ss;
    ss << key << "=" << value;
    ss << "; Path=" << path;
    if (max_age > 0)
        ss << "; Max-Age=" << max_age;
    ss << "; HttpOnly";
    ss << "; Secure";
    ss << "; SameSite=Strict";
    setHeader("Set-Cookie", ss.str());
}

Response Response::buildResponse(const Request &request, const ServerConfig &server_config,
                                 const LocationConfig *location_config)
{
    if (location_config)
    {
        return Response::createResponse(request, *location_config, server_config);
    }
    LocationConfig default_location;
    default_location.path = "/";
    default_location.methods.push_back("GET");
    default_location.directory_listing = false;
    default_location.index = DEFAULT_INDEX_PATH;
    return Response::createResponse(request, default_location, server_config);
}

Response Response::createResponse(const Request &request, const LocationConfig &location_config,
                                  const ServerConfig &server_config)
{

    std::string method = request.getMethod();
    std::string path = request.getPath();

    if (!ResponseHandler::validateMethod(request, location_config))
        return ResponseHandler::handleMethodNotAllowed(location_config, server_config);
    if (!location_config.redirect.empty())
        return ResponseHandler::handleRedirection(location_config);
    std::string real_path;
    if (!getRealPath(path, location_config, server_config, real_path))
        return createErrorResponse(404, server_config);
    if (ResponseHandler::isCGIRequest(real_path, location_config))
        return ResponseHandler::handleCGI(request, real_path, server_config);
    if (path == "/upload" && iequals(method, "post"))
        return ResponseHandler::handleUpload(real_path, request, location_config, server_config);
    if (path == "/filelist")
        return ResponseHandler::handleFileList(request, location_config, server_config);
    if (path == "/filelist/all" && iequals(method, "delete"))
        return ResponseHandler::handleDeleteAllFiles(location_config, server_config);
    return ResponseHandler::handleStaticFile(real_path, server_config);
}

bool Response::getRealPath(const std::string &path, const LocationConfig &location_config,
                           const ServerConfig &server_config, std::string &real_path)
{
    std::string normalized_path = ResponseUtil::buildRequestedPath(path, location_config, server_config);
    char real_path_cstr[PATH_MAX];
    if (realpath(normalized_path.c_str(), real_path_cstr) == NULL)
        return false;
    real_path = std::string(real_path_cstr);
    return true;
}

bool Response::isCGIRequest(const std::string &real_path, const LocationConfig &location_config)
{
    return ResponseHandler::isCGIRequest(real_path, location_config);
}

std::string Response::readErrorPageFromFile(const std::string &file_path, int status)
{
    int fd = open(file_path.c_str(), O_RDONLY);
    if (fd == -1)
    {
        std::stringstream ss;
        ss << "Error " << status << " (Failed to open file: " << file_path << ")";
        LogConfig::reportInternalError("Failed to open file " + file_path + ": " + strerror(errno));
        return ss.str();
    }
    std::string file_content;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0)
    {
        file_content.append(buffer, bytes_read);
    }
    close(fd);
    if (bytes_read == -1)
    {
        std::stringstream ss;
        ss << "Error " << status << " (Failed to read file: " << file_path << ")";
        LogConfig::reportInternalError("Failed to read file " + file_path + ": " + strerror(errno));
        return ss.str();
    }
    return file_content;
}

Response Response::createErrorResponse(const int status, const ServerConfig &server_config)
{
    Response res;
    std::string status_text;
    if (status == 400)
        status_text = NOT_FOUND_400;
    else if (status == 404)
        status_text = BAD_REQUEST_404;
    else if (status == 405)
        status_text = METHOD_NOT_ALLOWED_405;
    else if (status == 500)
        status_text = INTERNAL_SERVER_ERROR_500;
    else
    {
        std::stringstream ss;
        ss << status;
        status_text = ss.str() + " Error";
    }
    res.setStatus(status_text);
    res.setHeader("Content-Type", "text/html");

    std::map<int, std::string>::const_iterator serv_it = server_config.error_pages.find(status);
    if (serv_it == server_config.error_pages.end())
    {
        // 키가 없음 -> 기본 메시지
        std::stringstream ss;
        LogConfig::reportInternalError("(No error page found in server_config) / status : " + intToString(status) +
                                       ": " + strerror(errno));
        std::string default_error = ss.str();
        res.setBody(default_error);

        std::stringstream ss_len;
        ss_len << default_error.size();
        res.setHeader("Content-Length", ss_len.str());
        return res;
    }

    std::string error_file_path = serv_it->second;
    std::string file_content = readErrorPageFromFile(error_file_path, status);

    res.setBody(file_content);
    {
        std::stringstream ss_len;
        ss_len << file_content.size();
        res.setHeader("Content-Length", ss_len.str());
    }
    LogConfig::reportError(status, status_text);
    return res;
}

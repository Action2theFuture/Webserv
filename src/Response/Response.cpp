#include "Response.hpp"
#include "CGIHandler.hpp" // For cgi_handler.execute if needed
#include "Utils.hpp"      // trim, normalizePath, etc.
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <limits.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

Response::Response() : status("200 OK"), headers(), body("")
{
}
Response::~Response()
{
}

void Response::setStatus(const std::string &status_code)
{
    status = status_code;
}

void Response::setHeader(const std::string &key, const std::string &value)
{
    headers[key] = value;
}

void Response::setBody(const std::string &content)
{
    body = content;
}

std::string Response::toString() const
{
    std::stringstream response_stream;
    response_stream << "HTTP/1.1 " << status << "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
    {
        response_stream << it->first << ": " << it->second << "\r\n";
    }
    response_stream << "\r\n" << body;
    return response_stream.str();
}

// 메인 함수: createResponse
Response Response::createResponse(const Request &request, const LocationConfig &location_config,
                                  const ServerConfig &server_config)
{
    using namespace ResponseHelpers; // namespace import

    std::string method = request.getMethod();
    std::string path = request.getPath();

    // 메서드 확인
    if (!isMethodAllowed(method, location_config))
    {
        Response res = createErrorResponse(405, server_config);
        std::string allow_methods;
        for (size_t m = 0; m < location_config.methods.size(); ++m)
        {
            allow_methods += location_config.methods[m];
            if (m != location_config.methods.size() - 1)
            {
                allow_methods += ", ";
            }
        }
        res.setHeader("Allow", allow_methods);
        return res;
    }

    // 리디렉션 처리
    if (!location_config.redirect.empty())
    {
        return handleRedirection(location_config);
    }

    // 요청 경로 정규화
    std::string requested_path;
    if (path == "/")
    {
        // root + index
        if (!server_config.root.empty())
        {
            if (server_config.root[server_config.root.size() - 1] == '/')
                requested_path = server_config.root + location_config.index;
            else
                requested_path = server_config.root + "/" + location_config.index;
        }
        else
        {
            requested_path = location_config.index;
        }
    }
    else
    {
        // root + path
        if (!server_config.root.empty())
        {
            if (server_config.root[server_config.root.size() - 1] == '/')
                requested_path = server_config.root + path;
            else
                requested_path = server_config.root + "/" + path;
        }
        else
        {
            requested_path = path;
        }
    }

    std::string normalized_path = normalizePath(requested_path);
    char real_path_cstr[PATH_MAX];
    if (realpath(normalized_path.c_str(), real_path_cstr) == NULL)
    {
        return createErrorResponse(404, server_config);
    }
    std::string real_path(real_path_cstr);

    // CGI 처리
    if (!location_config.cgi_extension.empty() && real_path.size() >= location_config.cgi_extension.size())
    {
        std::string ext = real_path.substr(real_path.size() - location_config.cgi_extension.size());
        if (ext == location_config.cgi_extension)
        {
            return handleCGI(request, real_path, server_config);
        }
    }

    // 정적 파일 처리
    return handleStaticFile(real_path, server_config);
}

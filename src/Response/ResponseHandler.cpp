#include "CGIHandler.hpp"
#include "Response.hpp"
#include "Utils.hpp"
#include <fcntl.h>
#include <sstream>
#include <unistd.h>

namespace ResponseHelpers
{

bool isMethodAllowed(const std::string &method, const LocationConfig &location_config)
{
    for (size_t m = 0; m < location_config.methods.size(); ++m)
    {
        if (method == location_config.methods[m])
        {
            return true;
        }
    }
    return false;
}

Response handleRedirection(const LocationConfig &location_config)
{
    Response res;
    res.setStatus("301 Moved Permanently");
    res.setHeader("Location", location_config.redirect);

    std::string body = "<h1>301 Moved Permanently</h1>";
    res.setBody(body);
    {
        std::stringstream ss_len;
        ss_len << body.size();
        res.setHeader("Content-Length", ss_len.str());
    }
    res.setHeader("Content-Type", "text/html");
    return res;
}

Response handleCGI(const Request &request, const std::string &real_path, const ServerConfig &server_config)
{
    Response res;
    CGIHandler cgi_handler;
    std::string cgi_output, cgi_content_type;

    if (!cgi_handler.execute(request, real_path, cgi_output, cgi_content_type))
    {
        return createErrorResponse(500, server_config);
    }

    size_t pos = cgi_output.find("\r\n\r\n");
    if (pos != std::string::npos)
    {
        std::string headers = cgi_output.substr(0, pos);
        std::string body = cgi_output.substr(pos + 4);

        std::istringstream header_stream(headers);
        std::string header_line;
        while (std::getline(header_stream, header_line))
        {
            if (!header_line.empty() && header_line.back() == '\r')
            {
                header_line.pop_back();
            }
            size_t colon = header_line.find(':');
            if (colon != std::string::npos)
            {
                std::string key = trim(header_line.substr(0, colon));
                std::string value = trim(header_line.substr(colon + 1));
                res.setHeader(key, value);
            }
        }
        res.setStatus("200 OK");
        res.setBody(body);

        std::stringstream ss_len;
        ss_len << body.size();
        res.setHeader("Content-Length", ss_len.str());
        if (!cgi_content_type.empty())
        {
            res.setHeader("Content-Type", cgi_content_type);
        }
    }
    else
    {
        // 헤더 구분자 "\r\n\r\n" 없다면 전체를 body로 처리
        res.setStatus("200 OK");
        res.setBody(cgi_output);

        std::stringstream ss_len;
        ss_len << cgi_output.size();
        res.setHeader("Content-Length", ss_len.str());
        res.setHeader("Content-Type", "text/html");
    }
    return res;
}

Response handleStaticFile(const std::string &real_path, const ServerConfig &server_config)
{
    Response res;
    int fd = open(real_path.c_str(), O_RDONLY);
    if (fd == -1)
    {
        perror("open");
        return createErrorResponse(404, server_config);
    }

    const int BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    std::string file_content;
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0)
    {
        file_content.append(buffer, bytes_read);
    }
    close(fd);

    if (bytes_read == -1)
    {
        perror("read");
        return createErrorResponse(500, server_config);
    }

    std::string content_type = getMimeType(real_path);
    res.setStatus("200 OK");
    res.setBody(file_content);
    {
        std::stringstream ss_len;
        ss_len << file_content.size();
        res.setHeader("Content-Length", ss_len.str());
    }
    res.setHeader("Content-Type", content_type);

    return res;
}
} // namespace ResponseHelpers
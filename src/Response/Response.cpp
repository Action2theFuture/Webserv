#include "Response.hpp"

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

std::string Response::getStatus() const
{
    return status;
}

std::string Response::toString() const
{
    std::stringstream response_stream;
    response_stream << "HTTP/1.1 " << this->status << "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = this->headers.begin(); it != this->headers.end(); ++it)
    {
        response_stream << it->first << ": " << it->second << "\r\n";
    }
    response_stream << "\r\n" << this->body;
    return response_stream.str();
}

// 쿠키 설정 예시 (HttpOnly, Secure)
void Response::setCookie(const std::string &key, const std::string &value, const std::string &path, int max_age)
{
    std::stringstream ss;
    ss << key << "=" << value;
    ss << "; Path=" << path;
    if (max_age > 0)
    {
        ss << "; Max-Age=" << max_age;
    }
    ss << "; HttpOnly";        // JavaScript에서 접근 불가
    ss << "; Secure";          // HTTPS에서만 전송
    ss << "; SameSite=Strict"; // CSRF 방지
    setHeader("Set-Cookie", ss.str());
}

Response Response::buildResponse(const Request &request, const ServerConfig &server_config,
                                 const LocationConfig *location_config)
{

    if (location_config)
    {
        return Response::createResponse(request, *location_config, server_config);
    }

    // 기본 위치 설정에 따른 처리
    LocationConfig default_location;
    default_location.path = "/";
    default_location.methods.push_back("GET");
    default_location.directory_listing = false;
    default_location.index = DEFAULT_INDEX_PATH;
    return Response::createResponse(request, default_location, server_config);
}

// 메인 함수: createResponse
Response Response::createResponse(const Request &request, const LocationConfig &location_config,
                                  const ServerConfig &server_config)
{
    using namespace ResponseHandlers;

    std::string method = request.getMethod();
    std::string path = request.getPath();

    std::cout << "Request Path : " << path << std::endl;

    bool isMethodValid = validateMethod(request, location_config);
    if (!isMethodValid)
        return handleMethodNotAllowed(location_config, server_config);

    if (!location_config.redirect.empty())
        return handleRedirection(location_config);

    std::string real_path;
    bool pathSuccess = getRealPath(path, location_config, server_config, real_path);
    if (!pathSuccess)
        return createErrorResponse(404, server_config);

    bool isCGI = isCGIRequest(real_path, location_config);
    if (isCGI)
        return handleCGI(request, real_path, server_config);

    if (path == "/upload" && iequals(method, "post"))
    {
        std::cout << "Request Path in condition : " << path << std::endl;
        return handleUpload(real_path, request, location_config, server_config);
    }

    if (path == "/filelist")
    {
        std::cout << "Request Path in condition : " << path << std::endl;
        return handleFileList(request, location_config, server_config);
    }

    if (path == "/filelist/all" && iequals(method, "DELETE"))
    {
        std::cout << "Request Path in condition : " << path << std::endl;
        return handleDeleteAllFiles(location_config, server_config);
    }
    return handleStaticFile(real_path, server_config);
}

bool Response::validateMethod(const Request &request, const LocationConfig &location_config)
{
    using namespace ResponseUtils;
    std::string method = request.getMethod();

    // 메서드가 허용된 목록에 있는지 확인
    for (size_t i = 0; i < location_config.methods.size(); ++i)
    {
        if (iequals(method, location_config.methods[i]))
        {
            return true; // 메서드가 허용됨
        }
    }

    // 메서드가 허용되지 않음
    std::string errorMsg = "Method not allowed: " + method;
    LogConfig::logError(errorMsg);
    return false;
}

bool Response::getRealPath(const std::string &path, const LocationConfig &location_config,
                           const ServerConfig &server_config, std::string &real_path)
{
    using namespace ResponseUtils;
    std::string normalized_path = buildRequestedPath(path, location_config, server_config);
    char real_path_cstr[PATH_MAX];
    if (realpath(normalized_path.c_str(), real_path_cstr) == NULL)
    {
        return false;
    }
    real_path = std::string(real_path_cstr);
    return true;
}

bool Response::isCGIRequest(const std::string &real_path, const LocationConfig &location_config)
{
    using namespace ResponseUtils;
    if (!location_config.cgi_extension.empty() && real_path.size() >= location_config.cgi_extension.size())
    {
        std::string ext = real_path.substr(real_path.size() - location_config.cgi_extension.size());
        if (iequals(ext, location_config.cgi_extension))
        {
            std::cout << "CGI request detected." << std::endl;
            return true;
        }
    }
    return false;
}

std::string Response::readErrorPageFromFile(const std::string &file_path, int status)
{
    int fd = open(file_path.c_str(), O_RDONLY);
    if (fd == -1)
    {
        // 파일 열기 실패
        std::stringstream ss;
        ss << "Error " << status << " (Failed to open file: " << file_path << ")";
        LogConfig::logError("(Failed to open file" + file_path + ": " + strerror(errno));
        return ss.str(); // 에러 메시지 반환
    }

    std::string file_content;
    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, 4096)) > 0)
    {
        file_content.append(buffer, bytes_read);
    }
    close(fd);

    if (bytes_read == -1)
    {
        std::stringstream ss;
        ss << "Error " << status << " (Failed to read file: " << file_path << ")";
        LogConfig::logError("(Failed to read file" + file_path + ": " + strerror(errno));
        return ss.str();
    }

    return file_content; // 정상적으로 읽은 HTML 코드
}

// createErrorResponse 메인
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
        ss << "Error " << status << " (No error page found in server_config)";
        LogConfig::logError("(No error page found in server_config) / status : " + intToString(status) + ": " +
                            strerror(errno));
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
    LogConfig::logError(status_text + ": " + strerror(errno));
    return res;
}
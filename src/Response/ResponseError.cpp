#include "Response.hpp"
#include <fcntl.h>
#include <sstream>
#include <unistd.h>

static std::string readErrorPageFromFile(const std::string &file_path, int status)
{
    int fd = open(file_path.c_str(), O_RDONLY);
    if (fd == -1)
    {
        // 파일 열기 실패
        std::stringstream ss;
        ss << "Error " << status << " (Failed to open file: " << file_path << ")";
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
        return ss.str();
    }

    return file_content; // 정상적으로 읽은 HTML 코드
}
namespace ResponseHelpers
{
// createErrorResponse 메인
Response createErrorResponse(const int status, const ServerConfig &server_config)
{
    Response res;
    std::string status_text;
    if (status == 404)
        status_text = "404 Not Found";
    else if (status == 405)
        status_text = "405 Method Not Allowed";
    else if (status == 500)
        status_text = "500 Internal Server Error";
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
        std::string default_error = ss.str();
        res.setBody(default_error);

        std::stringstream ss_len;
        ss_len << default_error.size();
        res.setHeader("Content-Length", ss_len.str());
        return res;
    }

    std::string error_file_path = serv_it->second;
    // 서브함수로 파일 읽기
    std::string file_content = readErrorPageFromFile(error_file_path, status);

    res.setBody(file_content);
    {
        std::stringstream ss_len;
        ss_len << file_content.size();
        res.setHeader("Content-Length", ss_len.str());
    }
    return res;
}

} // namespace ResponseHelpers
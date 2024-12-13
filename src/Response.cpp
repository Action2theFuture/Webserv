#include "Response.hpp"
#include "Utils.hpp"
#include "CGIHandler.hpp"
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

// 생성자 및 소멸자
Response::Response() : status("200 OK"), headers(), body("") {}
Response::~Response() {}

// Setter 메서드
void Response::setStatus(const std::string &status_code) {
    status = status_code;
}

void Response::setHeader(const std::string &key, const std::string &value) {
    headers[key] = value;
}

void Response::setBody(const std::string &content) {
    body = content;
}

// toString 메서드
std::string Response::toString() const {
    std::stringstream response_stream;
    response_stream << "HTTP/1.1 " << status << "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        response_stream << it->first << ": " << it->second << "\r\n";
    }
    response_stream << "\r\n" << body;
    return response_stream.str();
}

namespace {
    Response createErrorResponse(const int status, const ServerConfig &server_config) {
        Response res;
        std::string status_text;
        if (status == 404) {
            status_text = "404 Not Found";
        }
        else if (status == 405) {
            status_text = "405 Method Not Allowed";
        }
        else if (status == 500) {
            status_text = "500 Internal Server Error";
        }
        else {
            // 다른 상태 코드 처리 또는 기본값 설정
            std::stringstream ss;
            ss << status;
            status_text = ss.str() + " Error";
        }
        res.setStatus(status_text);
        res.setHeader("Content-Type", "text/html");
        
        std::map<int, std::string>::const_iterator serv_it = 
            server_config.error_pages.find(status);
        if (serv_it == server_config.error_pages.end()) {
            // 키가 없음 -> 기본 에러 메시지
            std::stringstream ss;
            ss << "Error " << status << " (No error page found in server_config)";
            std::string default_error = ss.str();
            res.setBody(default_error);

            std::stringstream ss_len;
            ss_len << default_error.size();
            res.setHeader("Content-Length", ss_len.str());
            return res;
        }
        // loc_it->second에는 error_page "파일 경로"가 들어 있다고 가정
        std::string error_file_path = serv_it->second;
        // std::cerr << "error_page path: " << error_file_path << std::endl;

        // 파일 열기
        int fd = open(error_file_path.c_str(), O_RDONLY);
        if (fd == -1) {
            // 파일 열기 실패 -> 기본 에러 메시지
            std::stringstream ss;
            ss << "Error " << status << " (Failed to open file: " << error_file_path << ")";
            std::string default_error = ss.str();
            res.setBody(default_error);

            std::stringstream ss_len;
            ss_len << default_error.size();
            res.setHeader("Content-Length", ss_len.str());
            return res;
        }

        // 파일 내용 읽기
        std::string file_content;
        const int BUFFER_SIZE = 4096;
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;
        while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
            file_content.append(buffer, bytes_read);
        }
        close(fd);

        if (bytes_read == -1) {
            // read 실패
            std::stringstream ss;
            ss << "Error " << status << " (Failed to read file: " << error_file_path << ")";
            std::string default_error = ss.str();
            res.setBody(default_error);

            std::stringstream ss_len;
            ss_len << default_error.size();
            res.setHeader("Content-Length", ss_len.str());
            return res;
        }

        // 정상적으로 파일 내용 읽음
        res.setBody(file_content);

        // Content-Length 설정 (C++98에서는 std::stringstream 사용)
        std::stringstream ss_len;
        ss_len << file_content.size();
        res.setHeader("Content-Length", ss_len.str());

        return res;
    }

    bool isMethodAllowed(const std::string &method, const LocationConfig &location_config) {
        for (size_t m = 0; m < location_config.methods.size(); ++m) {
            if (method == location_config.methods[m]) {
                return true;
            }
        }
        return false;
    }

    Response handleRedirection(const LocationConfig &location_config) {
        Response res;
        res.setStatus("301 Moved Permanently");
        res.setHeader("Location", location_config.redirect);
        res.setBody("<h1>301 Moved Permanently</h1>");
        res.setHeader("Content-Length", "28");
        res.setHeader("Content-Type", "text/html");
        return res;
    }

    Response handleCGI(const Request &request, const std::string &real_path, const ServerConfig &server_config) {
        Response res;
        CGIHandler cgi_handler;
        std::string cgi_output, cgi_content_type;

        if (!cgi_handler.execute(request, real_path, cgi_output, cgi_content_type)) {
            return createErrorResponse(500, server_config);
        }

        size_t pos = cgi_output.find("\r\n\r\n");
        if (pos != std::string::npos) {
            std::string headers = cgi_output.substr(0, pos);
            std::string body = cgi_output.substr(pos + 4);

            std::istringstream header_stream(headers);
            std::string header_line;
            while (std::getline(header_stream, header_line)) {
                if (!header_line.empty() && header_line.back() == '\r') {
                    header_line.pop_back();
                }
                size_t colon = header_line.find(':');
                if (colon != std::string::npos) {
                    std::string key = trim(header_line.substr(0, colon));
                    std::string value = trim(header_line.substr(colon + 1));
                    res.setHeader(key, value);
                }
            }

            res.setStatus("200 OK");
            res.setBody(body);
            res.setHeader("Content-Length", std::to_string(body.size()));
            if (!cgi_content_type.empty()) {
                res.setHeader("Content-Type", cgi_content_type);
            }
        } else {
            res.setStatus("200 OK");
            res.setBody(cgi_output);
            res.setHeader("Content-Length", std::to_string(cgi_output.size()));
            res.setHeader("Content-Type", "text/html");
        }

        return res;
    }

    Response handleStaticFile(const std::string &real_path, const ServerConfig &server_config) {
        Response res;
        int fd = open(real_path.c_str(), O_RDONLY);
        if (fd == -1) {
            perror("open");
            return createErrorResponse(404, server_config);
        }

        const int BUFFER_SIZE = 4096;
        char buffer[BUFFER_SIZE];
        std::string file_content;
        ssize_t bytes_read;

        while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
            file_content.append(buffer, bytes_read);
        }

        close(fd);

        if (bytes_read == -1) {
            perror("read");
            return createErrorResponse(500, server_config);
        }

        std::string content_type = getMimeType(real_path);
        res.setStatus("200 OK");
        res.setBody(file_content);
        res.setHeader("Content-Length", std::to_string(file_content.size()));
        res.setHeader("Content-Type", content_type);

        return res;
    }
}

Response Response::createResponse(const Request &request, const LocationConfig &location_config, const ServerConfig &server_config) {
    std::string method = request.getMethod();
    std::string path = request.getPath();

    // HTTP 메서드 확인
    if (!isMethodAllowed(method, location_config)) {
        Response res = createErrorResponse(405, server_config);
        std::string allow_methods;
        for (size_t m = 0; m < location_config.methods.size(); ++m) {
            allow_methods += location_config.methods[m];
            if (m != location_config.methods.size() - 1) {
                allow_methods += ", ";
            }
        }
        res.setHeader("Allow", allow_methods);
        return res;
    }

    // 리디렉션 처리
    if (!location_config.redirect.empty()) {
        return handleRedirection(location_config);
    }

    // 요청 경로 정규화
    std::string requested_path; // 위치 블록의 루트를 기준으로 설정
    if (path == "/") {
        // root + index
        if (!server_config.root.empty()) {
            // join root + index
            if (server_config.root[server_config.root.size() - 1] == '/')
                requested_path = server_config.root + location_config.index;
            else
                requested_path = server_config.root + "/" + location_config.index;
        } else {
            // fallback
            requested_path = location_config.index;
        }
    } else {
        // root + path
        if (!server_config.root.empty()) {
            if (server_config.root[server_config.root.size() - 1] == '/')
                requested_path = server_config.root + path;
            else
                requested_path = server_config.root + "/" + path;
        } else {
            requested_path = path; 
        }
    }
    std::string normalized_path = normalizePath(requested_path);
    //std::cerr << "normalize pth " << normalized_path << std::endl;
    // realpath 확인
    char real_path_cstr[PATH_MAX];
    if (realpath(normalized_path.c_str(), real_path_cstr) == NULL) {
        //std::cerr << "realpath failed: " << strerror(errno) << std::endl;
        return createErrorResponse(404, server_config);
    }
    std::string real_path(real_path_cstr);
    // std::cerr << "real pth cstr " << real_path_cstr << std::endl;
    // CGI 처리
    if (!location_config.cgi_extension.empty() && real_path.size() >= location_config.cgi_extension.size()) {
        std::string ext = real_path.substr(real_path.size() - location_config.cgi_extension.size());
        if (ext == location_config.cgi_extension) {
            return handleCGI(request, real_path, server_config);
        }
    }
    // 정적 파일 처리
    return handleStaticFile(real_path, server_config);
}
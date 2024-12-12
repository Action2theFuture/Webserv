#include "Response.hpp"
#include "CGIHandler.hpp"
#include "Utils.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sstream>

Response::Response() : status("200 OK"), body("") {}

Response::~Response() {}

// private 메서드 구현
void Response::setStatus(const std::string &status_code) {
    status = status_code;
}

void Response::setHeader(const std::string &key, const std::string &value) {
    headers[key] = value;
}

void Response::setBody(const std::string &content) {
    body = content;
}


Response Response::createResponse(const Request &request) {
    Response res;
    std::string method = request.getMethod();
    std::string path = request.getPath();

    // 경로 정규화
    std::string requested_path = "." + path;
    std::string normalized_path = normalizePath(requested_path);

    // 서버 루트 디렉토리 내에 있는지 확인
    char real_path_cstr[PATH_MAX];
    if (realpath(normalized_path.c_str(), real_path_cstr) == NULL) {
        // 경로 변환 실패: 404 응답
        res.setStatus("404 Not Found");
        res.setBody("<h1>404 Not Found</h1>");
        res.setHeader("Content-Length", "15");
        res.setHeader("Content-Type", "text/html");
        return res;
    }

    std::string real_path(real_path_cstr);
    char server_root_cstr[PATH_MAX];
    if (realpath(".", server_root_cstr) == NULL) {
        // 서버 루트 경로 변환 실패: 500 응답
        res.setStatus("500 Internal Server Error");
        res.setBody("<h1>500 Internal Server Error</h1>");
        res.setHeader("Content-Length", "29");
        res.setHeader("Content-Type", "text/html");
        return res;
    }

    std::string server_root(server_root_cstr);
    if (real_path.find(server_root) != 0) {
        // 서버 루트 외부 접근 시도: 403 Forbidden 응답
        res.setStatus("403 Forbidden");
        res.setBody("<h1>403 Forbidden</h1>");
        res.setHeader("Content-Length", "16");
        res.setHeader("Content-Type", "text/html");
        return res;
    }

    // CGI 경로 설정 (예: .py 파일)
    if (path.find(".py") != std::string::npos) {
        std::string script_path = real_path;

        // CGIHandler 객체 생성
        CGIHandler cgi_handler;
        std::string cgi_output;
        std::string content_type;

        bool success = cgi_handler.execute(request, script_path, cgi_output, content_type);
        if (!success) {
            // 에러 응답 설정
            res.setStatus("500 Internal Server Error");
            res.setBody("<h1>500 Internal Server Error</h1>");
            res.setHeader("Content-Length", "29");
            res.setHeader("Content-Type", "text/html");
            return res;
        }

        // CGI 출력이 헤더와 본문을 포함하는 경우
        size_t pos = cgi_output.find("\r\n\r\n");
        if (pos != std::string::npos) {
            std::string headers = cgi_output.substr(0, pos);
            std::string body = cgi_output.substr(pos + 4);

            // 헤더 파싱
            std::istringstream header_stream(headers);
            std::string header_line;
            while (std::getline(header_stream, header_line)) {
                if (header_line.find("Content-Type:") != std::string::npos) {
                    size_t colon = header_line.find(':');
                    if (colon != std::string::npos && colon + 2 < header_line.size()) {
                        std::string parsed_content_type = header_line.substr(colon + 2);
                        res.setHeader("Content-Type", parsed_content_type);
                    }
                }
                // 추가 헤더 파싱 가능
            }

            res.setStatus("200 OK");
            res.setBody(body);
            res.setHeader("Content-Length", std::to_string(body.size()));
        }
        else {
            // 헤더가 없는 경우 기본 설정
            res.setStatus("200 OK");
            res.setBody(cgi_output);
            res.setHeader("Content-Length", std::to_string(cgi_output.size()));
            res.setHeader("Content-Type", "text/html"); // 기본 타입
        }
    }
    else {
        // 정적 파일 처리
        int fd = open(real_path.c_str(), O_RDONLY);
        if (fd != -1) {
            const int BUFFER_SIZE = 4096;
            char buffer[BUFFER_SIZE];
            std::string file_content;
            ssize_t bytes_read;

            while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
                file_content.append(buffer, bytes_read);
            }

            if (bytes_read == -1) {
                // 읽기 실패
                perror("read");
                res.setStatus("500 Internal Server Error");
                res.setBody("<h1>500 Internal Server Error</h1>");
                res.setHeader("Content-Length", "29");
                res.setHeader("Content-Type", "text/html");
                close(fd);
                return res;
            }

            // 파일 닫기
            close(fd);

            // MIME 타입 설정
            std::string content_type = getMimeType(real_path);

            // 응답 설정
            res.setStatus("200 OK");
            res.setBody(file_content);
            res.setHeader("Content-Length", std::to_string(file_content.size()));
            res.setHeader("Content-Type", content_type); // 동적으로 설정
        }
        else {
            // 파일 열기 실패
            perror("open");
            res.setStatus("404 Not Found");
            res.setBody("<h1>404 Not Found</h1>");
            res.setHeader("Content-Length", "15");
            res.setHeader("Content-Type", "text/html");
        }
    }

    return res;
}

std::string Response::toString() const {
    std::stringstream response_stream;
    response_stream << "HTTP/1.1 " << status << "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        response_stream << it->first << ": " << it->second << "\r\n";
    }
    response_stream << "\r\n" << body;
    return response_stream.str();
}

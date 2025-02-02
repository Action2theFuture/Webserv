#include "CGIHandler.hpp"
#include "Log.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Utils.hpp"

CGIHandler::CGIHandler()
{
    // 생성자: 필요한 초기화 수행
}

CGIHandler::~CGIHandler()
{
    // 소멸자: 필요한 정리 작업 수행
}

void CGIHandler::setEnvironmentVariables(const Request &request, const std::string &script_path)
{
    // Set standard CGI environment variables
    setenv("REQUEST_METHOD", request.getMethod().c_str(), 1);
    setenv("SCRIPT_FILENAME", script_path.c_str(), 1);
    setenv("QUERY_STRING", request.getQueryString().c_str(), 1);

    std::map<std::string, std::string> headers = request.getHeaders();
    if (headers.find("Content-Length") != headers.end())
    {
        setenv("CONTENT_LENGTH", headers["Content-Length"].c_str(), 1);
    }
    else
    {
        setenv("CONTENT_LENGTH", "0", 1);
    }

    if (headers.find("Content-Type") != headers.end())
    {
        setenv("CONTENT_TYPE", headers["Content-Type"].c_str(), 1);
    }
    else
    {
        setenv("CONTENT_TYPE", "text/plain", 1);
    }

    setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
    setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
    setenv("SERVER_SOFTWARE", "Webserv/1.0", 1);
    // Add more environment variables as needed
}

bool CGIHandler::execute(const Request &request, const std::string &script_path, std::string &cgi_output,
                         std::string &content_type)
{
    // 파이프 생성
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        LogConfig::logError("Pipe creation failed: " + std::string(strerror(errno)));
        return false;
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        // fork 실패
        perror("fork");
        LogConfig::logError("Fork failed: " + std::string(strerror(errno)));
        close(pipefd[0]);
        close(pipefd[1]);
        return false;
    }

    if (pid == 0)
    {
        // 자식 프로세스: CGI 실행

        // 파이프의 읽기 끝을 닫음
        close(pipefd[0]);

        // 표준 출력(1)을 파이프의 쓰기 끝으로 리다이렉션
        if (dup2(pipefd[1], STDOUT_FILENO) == -1)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(pipefd[1]);

        // 환경 변수 설정
        setEnvironmentVariables(request, script_path);

        // CGI 프로그램 실행 (Python 스크립트)
        execl("/usr/bin/python3", "python3", script_path.c_str(), NULL);

        // execve 실패 시 종료
        perror("execl");
        LogConfig::logError("Exec failed: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }
    else
    {
        // 부모 프로세스: CGI 출력 읽기

        // 파이프의 쓰기 끝을 닫음
        close(pipefd[1]);

        // 자식 프로세스의 출력을 읽기 위한 버퍼 설정
        char buffer[4096];
        ssize_t bytes_read;

        // 파이프에서 데이터 읽기
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0)
        {
            cgi_output.append(buffer, bytes_read);
        }

        if (bytes_read == -1)
        {
            perror("read");
            LogConfig::logError("Read failed: " + std::string(strerror(errno)));
            close(pipefd[0]);
            return false;
        }

        // 파이프 닫기
        close(pipefd[0]);

        // 자식 프로세스 종료 대기
        int status;
        waitpid(pid, &status, 0);

        // CGI 출력 파싱 (헤더와 본문 분리)
        size_t pos = cgi_output.find("\r\n\r\n");
        if (pos != std::string::npos)
        {
            std::string headers = cgi_output.substr(0, pos);
            std::string body = cgi_output.substr(pos + 4);

            // 헤더 파싱
            std::istringstream header_stream(headers);
            std::string header_line;
            while (std::getline(header_stream, header_line))
            {
                if (header_line.find("Content-Type:") != std::string::npos)
                {
                    // "Content-Type: "의 길이는 14 (공백 포함)
                    size_t colon = header_line.find(':');
                    if (colon != std::string::npos && colon + 2 < header_line.size())
                    {
                        std::string content_type = header_line.substr(colon + 2);
                        content_type = header_line.substr(colon + 2);
                    }
                }
                // 추가 헤더 파싱 가능
            }
        }
        else
        {
            // 헤더가 없는 경우 기본 설정
            content_type = "text/html"; // 기본 타입
        }

        return true;
    }

    return true;
}

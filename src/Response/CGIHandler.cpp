#include "CGIHandler.hpp"
#include "Log.hpp"
#include "Request.hpp"
#include <iostream>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

CGIHandler::CGIHandler()
{
}

CGIHandler::~CGIHandler()
{
}

void CGIHandler::setEnvironmentVariables(const Request &request, const std::string &script_path)
{
    setenv("REQUEST_METHOD", request.getMethod().c_str(), 1);
    setenv("SCRIPT_FILENAME", script_path.c_str(), 1);
    setenv("QUERY_STRING", request.getQueryString().c_str(), 1);
    std::map<std::string, std::string> headers = request.getHeaders();
    if (headers.find("Content-Length") != headers.end())
        setenv("CONTENT_LENGTH", headers["Content-Length"].c_str(), 1);
    else
        setenv("CONTENT_LENGTH", "0", 1);
    if (headers.find("Content-Type") != headers.end())
        setenv("CONTENT_TYPE", headers["Content-Type"].c_str(), 1);
    else
        setenv("CONTENT_TYPE", "text/plain", 1);
    setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
    setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
    setenv("SERVER_SOFTWARE", "Webserv/1.0", 1);
}

bool CGIHandler::execute(const Request &request, const std::string &script_path, std::string &cgi_output,
                         std::string &content_type)
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        LogConfig::reportInternalError("Pipe creation failed: " + std::string(strerror(errno)));
        return false;
    }
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        LogConfig::reportInternalError("Fork failed: " + std::string(strerror(errno)));
        close(pipefd[0]);
        close(pipefd[1]);
        return false;
    }
    if (pid == 0)
    {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) == -1)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(pipefd[1]);
        setEnvironmentVariables(request, script_path);

        // execve를 사용하여 Python 스크립트 실행 (argv 배열 생성)
        char *args[] = {const_cast<char *>("python"), const_cast<char *>(script_path.c_str()), NULL};
        extern char **environ;
        execve(PYTHON_PATH, args, environ);

        // execve 실패 시
        perror("execve");
        LogConfig::reportInternalError("Execve failed: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }
    else
    {
        close(pipefd[1]);
        char buffer[4096];
        ssize_t bytes_read;
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0)
        {
            cgi_output.append(buffer, bytes_read);
        }
        if (bytes_read == -1)
        {
            perror("read");
            LogConfig::reportInternalError("Read failed: " + std::string(strerror(errno)));
            close(pipefd[0]);
            return false;
        }
        close(pipefd[0]);
        int status;
        waitpid(pid, &status, 0);
        size_t pos = cgi_output.find("\r\n\r\n");
        if (pos != std::string::npos)
        {
            std::string headers_part = cgi_output.substr(0, pos);
            std::string body_part = cgi_output.substr(pos + 4);
            std::istringstream header_stream(headers_part);
            std::string header_line;
            while (std::getline(header_stream, header_line))
            {
                if (!header_line.empty() && header_line[header_line.size() - 1] == '\r')
                    header_line.resize(header_line.size() - 1);
                size_t colon = header_line.find(':');
                if (colon != std::string::npos && colon + 2 < header_line.size())
                {
                    content_type = header_line.substr(colon + 2);
                }
            }
        }
        else
        {
            content_type = "text/html";
        }
        return true;
    }
    return true;
}

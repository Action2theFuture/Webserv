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
    std::string path = request.getPath();
    setenv("REQUEST_METHOD", path.c_str(), 1);
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

    ////
    //use PATH_INFO for CGI
    /* setenv("PATH_INFO", request.getPath().c_str(), 1);
    std::cout << "DEBUG) Path Info: " << request.getPath().c_str() << std::endl; */
    ////
}

bool CGIHandler::execute(const Request &request, const std::string &script_path, std::string &cgi_output,
                         std::string &content_type)
{

    // Check extension validity
    std::cout << "DEBUG) Script Path: " << script_path << std::endl;
    std::string extension = script_path.substr(script_path.find_last_of('.') + 1);

    if (extension != "py" && extension != "php" && extension != "sh" && extension != "pl")
    {
        LogConfig::reportInternalError("Unsupported CGI Script Extension: " + extension);
        return false;
    }

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

        //execution based on extension
        if (extension == "py")
        {
            char *args[] = {const_cast<char *>("python"), const_cast<char *>(script_path.c_str()), NULL};
            if (execve(PYTHON_PATH, args, environ) == -1)
            {
                perror("execve");
                LogConfig::reportInternalError("Execve failed for Python: " + std::string(strerror(errno)));
                exit(EXIT_FAILURE);
            }
        }

        else if (extension == "sh")
        {
            char *args[] = {const_cast<char *>("bash"), const_cast<char *>(script_path.c_str()), NULL};
            if (execve("/bin/bash", args, environ) == -1)
            {
                perror("execve");
                LogConfig::reportInternalError("Execve failed for Bash: " + std::string(strerror(errno)));
                exit(EXIT_FAILURE);
            }
        }

        else if (extension == "pl")
        {
            char *args[] = {const_cast<char *>("perl"), const_cast<char *>(script_path.c_str()), NULL};
            if (execve("/usr/bin/perl", args, environ) == -1)
            {
                perror("execve");
                LogConfig::reportInternalError("Execve failed for Perl: " + std::string(strerror(errno)));
                exit(EXIT_FAILURE);
            }
        }
    }

    else
    {
        close(pipefd[1]);
        char buffer[BUFFER_SIZE];
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

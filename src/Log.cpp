#include "Log.hpp"
#include "Utils.hpp" // for intToString
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>

// ANSI escape codes
const std::string RESET_COLOR = "\033[0m";
const std::string WHITE_COLOR = "\033[37m";
const std::string YELLOW_COLOR = "\033[33m";
const std::string RED_COLOR = "\033[31m";
const std::string GREEN_COLOR = "\033[32m";
const std::string MAGENTA_COLOR = "\033[35m";

void LogConfig::printColoredMessage(const std::string &message, const char *time_str, const std::string &color)
{
    std::cerr << color << "[" << time_str << "] " << message << RESET_COLOR << std::endl;
}

std::string LogConfig::getCurrentTimeStr()
{
    time_t now = time(NULL);
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buffer);
}

void LogConfig::ensureLogDirectoryExists()
{
#ifdef DEV_MODE
    const char *logDir = LOG_DIR;
    struct stat st;
    if (stat(logDir, &st) == -1)
    {
        if (mkdir(logDir, 0755) == -1)
        {
            std::cerr << "Error: Failed to create log directory " << logDir << ": " << strerror(errno) << std::endl;
        }
        else
        {
            std::cerr << "Log directory created: " << logDir << std::endl;
        }
    }
#endif
}

void LogConfig::printServerConfig(const ServerConfig &server, size_t index, std::ofstream &ofs)
{
    std::string time_str = getCurrentTimeStr();

    ofs << "[" << time_str << "] Server " << index + 1 << ":" << std::endl;
    ofs << "  Listen Port: " << server.port << std::endl;
    ofs << "  Server Name: " << server.server_name << std::endl;
    ofs << "  Root: " << server.root << std::endl;
    ofs << "  Client Max Body Size: " << server.client_max_body_size << std::endl;
    ofs << "  Error Pages:" << std::endl;
    for (std::map<int, std::string>::const_iterator it = server.error_pages.begin(); it != server.error_pages.end(); ++it)
    {
        ofs << "    " << it->first << " -> " << it->second << std::endl;
    }
    ofs << "  Locations:" << std::endl;
    for (size_t j = 0; j < server.locations.size(); ++j)
    {
        const LocationConfig &location = server.locations[j];
        ofs << "    Location " << j + 1 << ":" << std::endl;
        ofs << "      Path: " << location.path << std::endl;
        ofs << "      Allowed Methods: ";
        for (size_t k = 0; k < location.methods.size(); ++k)
        {
            ofs << location.methods[k];
            if (k != location.methods.size() - 1)
                ofs << ", ";
        }
        ofs << std::endl;
        ofs << "      Root: " << location.root << std::endl;
        ofs << "      Upload Directory: " << location.upload_directory << std::endl;
        ofs << "      Allowed Extensions: ";
        for (size_t k = 0; k < location.allowed_extensions.size(); ++k)
        {
            ofs << location.allowed_extensions[k];
            if (k != location.allowed_extensions.size() - 1)
                ofs << ", ";
        }
        ofs << std::endl;
        ofs << "      Directory Listing: " << (location.directory_listing ? "on" : "off") << std::endl;
        ofs << "      Index: " << location.index << std::endl;
        ofs << "      CGI Extension: " << location.cgi_extension << std::endl;
        ofs << "      CGI Path: " << location.cgi_path << std::endl;
        ofs << std::endl;
    }
}

void LogConfig::printAllConfigurations(const std::vector<ServerConfig> &servers)
{
#ifdef DEV_MODE
    ensureLogDirectoryExists();

    std::ofstream ofs(SERVER_CONFIG_LOG_FILE, std::ios::app);
    if (!ofs.is_open())
    {
        std::cerr << "Error: Failed to open ./logs/server_config.log" << std::endl;
        return;
    }

    std::string time_str = getCurrentTimeStr();

    ofs << "[" << time_str << "] ==== Parsed Server Configurations ====" << std::endl;
    for (size_t i = 0; i < servers.size(); ++i)
    {
        printServerConfig(servers[i], i, ofs);
    }
    ofs << "[" << time_str << "] ======================================" << std::endl << std::endl;

    ofs.close();
#else
    (void)servers;
    // 디버그 모드가 아닐 때는 콘솔에만 출력
    std::string time_str = getCurrentTimeStr();
    std::cerr << "[" << time_str << "] Parsed Server Configurations" << std::endl;
#endif
}

void LogConfig::reportSuccess(int status, const std::string &message)
{
    std::string time_str = getCurrentTimeStr();
#ifdef DEV_MODE
    ensureLogDirectoryExists();

    std::ofstream log_file(STATUS_SUCCESS_LOG_FILE, std::ios::app);
    if (!log_file.is_open())
    {
        std::ofstream create_file(STATUS_SUCCESS_LOG_FILE);
        if (!create_file.is_open())
        {
            std::cerr << WHITE_COLOR << "[" << time_str << "] Error: Failed to create log file "
                      << STATUS_SUCCESS_LOG_FILE << RESET_COLOR << std::endl;
            printColoredMessage("Status: " + intToString(status) + ", Message: " + message, time_str.c_str(), GREEN_COLOR);
            return;
        }
        create_file.close();
        log_file.open(STATUS_SUCCESS_LOG_FILE, std::ios::app);
        if (!log_file.is_open())
        {
            std::cerr << WHITE_COLOR << "[" << time_str << "] Error: Failed to open newly created log file "
                      << STATUS_SUCCESS_LOG_FILE << RESET_COLOR << std::endl;
            printColoredMessage("Status: " + intToString(status) + ", Message: " + message, time_str.c_str(), GREEN_COLOR);
            return;
        }
    }

    log_file << "[" << time_str << "] "
             << "Status: " << status << ", Message: " << message << std::endl;
    log_file.close();
#endif
    printColoredMessage("Status: " + intToString(status) + ", Message: " + message, time_str.c_str(), GREEN_COLOR);
}

void LogConfig::reportError(int status, const std::string &message)
{
    std::string time_str = getCurrentTimeStr();
#ifdef DEV_MODE
    ensureLogDirectoryExists();

    std::ofstream log_file(STATUS_ERROR_LOG_FILE, std::ios::app);
    if (!log_file.is_open())
    {
        std::ofstream create_file(STATUS_ERROR_LOG_FILE);
        if (!create_file.is_open())
        {
            std::cerr << WHITE_COLOR << "[" << time_str << "] Error: Failed to create log file "
                      << STATUS_ERROR_LOG_FILE << RESET_COLOR << std::endl;
            std::string color = WHITE_COLOR;
            if (status >= 500)
                color = RED_COLOR;
            else if (status >= 400 && status < 500)
                color = YELLOW_COLOR;
            printColoredMessage("Status: " + intToString(status) + ", Message: " + message, time_str.c_str(), color);
            return;
        }
        create_file.close();
        log_file.open(STATUS_ERROR_LOG_FILE, std::ios::app);
        if (!log_file.is_open())
        {
            std::cerr << WHITE_COLOR << "[" << time_str << "] Error: Failed to open newly created log file "
                      << STATUS_ERROR_LOG_FILE << RESET_COLOR << std::endl;
            std::string color = WHITE_COLOR;
            if (status >= 500)
                color = RED_COLOR;
            else if (status >= 400 && status < 500)
                color = YELLOW_COLOR;
            printColoredMessage("Status: " + intToString(status) + ", Message: " + message, time_str.c_str(), color);
            return;
        }
    }

    log_file << "[" << time_str << "] "
             << "Status: " << status << ", Message: " << message << std::endl;
    log_file.close();
#endif
    std::string color = WHITE_COLOR;
    if (status >= 500)
        color = RED_COLOR;
    else if (status >= 400 && status < 500)
        color = YELLOW_COLOR;
    printColoredMessage("Status: " + intToString(status) + ", Message: " + message, time_str.c_str(), color);
}

void LogConfig::reportInternalError(const std::string &message)
{
    std::string time_str = getCurrentTimeStr();
#ifdef DEV_MODE
    ensureLogDirectoryExists();

    std::ofstream log_file(INTERNAL_ERROR_LOG_FILE, std::ios::app);
    if (!log_file.is_open())
    {
        std::ofstream create_file(INTERNAL_ERROR_LOG_FILE);
        if (!create_file.is_open())
        {
            std::cerr << WHITE_COLOR << "[" << time_str << "] Error: Failed to create log file "
                      << INTERNAL_ERROR_LOG_FILE << RESET_COLOR << std::endl;
            printColoredMessage(message, time_str.c_str(), MAGENTA_COLOR);
            return;
        }
        create_file.close();
        log_file.open(INTERNAL_ERROR_LOG_FILE, std::ios::app);
        if (!log_file.is_open())
        {
            std::cerr << WHITE_COLOR << "[" << time_str << "] Error: Failed to open newly created log file "
                      << INTERNAL_ERROR_LOG_FILE << RESET_COLOR << std::endl;
            printColoredMessage(message, time_str.c_str(), MAGENTA_COLOR);
            return;
        }
    }

    log_file << "[" << time_str << "] INTERNAL ERROR: " << message << std::endl;
    log_file.close();
#endif
    printColoredMessage("INTERNAL ERROR: " + message, time_str.c_str(), MAGENTA_COLOR);
}


#include "Log.hpp"

// ANSI escape codes
const std::string RESET_COLOR = "\033[0m";
const std::string WHITE_COLOR = "\033[37m";
const std::string YELLOW_COLOR = "\033[33m";
const std::string RED_COLOR = "\033[31m";
const std::string GREEN_COLOR = "\033[32m]";
const std::string MAGENTA_COLOR = "\033[35m]";

void LogConfig::printColoredMessage(const std::string &message, const char *time_str, const std::string &color)
{
    std::cerr << color << "[" << time_str << "] " << message << RESET_COLOR << std::endl;
}

std::string LogConfig::getCurrentTimeStr()
{
    std::time_t now = std::time(nullptr);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buffer);
}

// 서버 설정을 출력하는 함수
void LogConfig::printServerConfig(const ServerConfig &server, size_t index, std::ofstream &ofs)
{
    std::string time_str = getCurrentTimeStr();

    ofs << "[" << time_str << "] Server " << index + 1 << ":" << std::endl;
    ofs << "  Listen Port: " << server.port << std::endl;
    ofs << "  Server Name: " << server.server_name << std::endl;
    ofs << "  Root: " << server.root << std::endl;
    ofs << "  Client Max Body Size: " << server.client_max_body_size << std::endl;
    ofs << "  Error Pages:" << std::endl;
    for (std::map<int, std::string>::const_iterator it = server.error_pages.begin(); it != server.error_pages.end();
         ++it)
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
            {
                ofs << ", ";
            }
        }
        ofs << std::endl;
        ofs << "      Root: " << location.root << std::endl;
        ofs << "      Upload Directory: " << location.upload_directory << std::endl;
        ofs << "      Allowed Extensions: ";
        for (size_t k = 0; k < location.allowed_extensions.size(); ++k)
        {
            ofs << location.allowed_extensions[k];
            if (k != location.allowed_extensions.size() - 1)
            {
                ofs << ", ";
            }
        }
        ofs << std::endl;
        ofs << "      Directory Listing: " << (location.directory_listing ? "on" : "off") << std::endl;
        ofs << "      Index: " << location.index << std::endl;
        ofs << "      CGI Extension: " << location.cgi_extension << std::endl;
        ofs << "      CGI Path: " << location.cgi_path << std::endl;
        ofs << std::endl;
    }
}

// 모든 서버 설정을 출력하는 함수
void LogConfig::printAllConfigurations(const std::vector<ServerConfig> &servers)
{
    // 로그 디렉토리 생성 확인
    ensureLogDirectoryExists();

    // 로그 파일 열기 (append 모드)
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
}
// 성공(200번대) 로그를 기록하는 함수
void LogConfig::reportSuccess(int status, const std::string &message)
{
    ensureLogDirectoryExists();

    std::string time_str = getCurrentTimeStr();
    std::ofstream log_file(STATUS_SUCCESS_LOG_FILE, std::ios::app);
    if (!log_file.is_open())
    {
        // 파일 생성 시도
        std::ofstream create_file(STATUS_SUCCESS_LOG_FILE);
        if (!create_file.is_open())
        {
            std::cerr << WHITE_COLOR << "[" << time_str << "] Error: Failed to create log file "
                      << STATUS_SUCCESS_LOG_FILE << RESET_COLOR << std::endl;
            printColoredMessage("Status: " + std::to_string(status) + ", Message: " + message, time_str.c_str(),
                                GREEN_COLOR);
            return;
        }
        create_file.close();
        log_file.open(STATUS_SUCCESS_LOG_FILE, std::ios::app);
        if (!log_file.is_open())
        {
            std::cerr << WHITE_COLOR << "[" << time_str << "] Error: Failed to open newly created log file "
                      << STATUS_SUCCESS_LOG_FILE << RESET_COLOR << std::endl;
            printColoredMessage("Status: " + std::to_string(status) + ", Message: " + message, time_str.c_str(),
                                GREEN_COLOR);
            return;
        }
    }

    // 로그 파일에 상태 코드, 메시지와 시간을 기록
    log_file << "[" << time_str.c_str() << "] "
             << "Status: " << status << ", Message: " << message << std::endl;
    log_file.close();

    // 성공 로그는 초록색으로 콘솔에 출력
    printColoredMessage("Status: " + std::to_string(status) + ", Message: " + message, time_str.c_str(), GREEN_COLOR);
}

void LogConfig::reportError(int status, const std::string &message)
{
    ensureLogDirectoryExists();

    std::string time_str = getCurrentTimeStr();
    std::ofstream log_file(STATUS_ERROR_LOG_FILE, std::ios::app);
    if (!log_file.is_open())
    {
        // 파일 생성 시도
        std::ofstream create_file(STATUS_ERROR_LOG_FILE);
        if (!create_file.is_open())
        {
            std::cerr << WHITE_COLOR << "[" << time_str << "] Error: Failed to create log file "
                      << STATUS_ERROR_LOG_FILE << RESET_COLOR << std::endl;
            // 에러 코드에 따른 색상 결정
            std::string color = WHITE_COLOR;
            if (status >= 500)
                color = RED_COLOR;
            else if (status >= 400 && status < 500)
                color = YELLOW_COLOR;
            printColoredMessage("Status: " + std::to_string(status) + ", Message: " + message, time_str.c_str(), color);
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
            printColoredMessage("Status: " + std::to_string(status) + ", Message: " + message, time_str.c_str(), color);
            return;
        }
    }

    // 로그 파일에 상태 코드, 메시지와 시간을 기록
    log_file << "[" << time_str << "] "
             << "Status: " << status << ", Message: " << message << std::endl;
    log_file.close();

    // 에러 로그의 색상 결정: 500 이상은 빨간색, 400대는 노란색, 그 외는 기본 흰색
    std::string color = WHITE_COLOR;
    if (status >= 500)
        color = RED_COLOR;
    else if (status >= 400 && status < 500)
        color = YELLOW_COLOR;

    printColoredMessage("Status: " + std::to_string(status) + ", Message: " + message, time_str.c_str(), color);
}

void LogConfig::reportInternalError(const std::string &message)
{
    ensureLogDirectoryExists();

    std::time_t now = std::time(nullptr);
    char time_str[100];
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    std::ofstream log_file(INTERNAL_ERROR_LOG_FILE, std::ios::app);
    if (!log_file.is_open())
    {
        std::ofstream create_file(INTERNAL_ERROR_LOG_FILE);
        if (!create_file.is_open())
        {
            std::cerr << WHITE_COLOR << "[" << time_str << "] Error: Failed to create log file "
                      << INTERNAL_ERROR_LOG_FILE << RESET_COLOR << std::endl;
            printColoredMessage(message, time_str, MAGENTA_COLOR);
            return;
        }
        create_file.close();
        log_file.open(INTERNAL_ERROR_LOG_FILE, std::ios::app);
        if (!log_file.is_open())
        {
            std::cerr << WHITE_COLOR << "[" << time_str << "] Error: Failed to open newly created log file "
                      << INTERNAL_ERROR_LOG_FILE << RESET_COLOR << std::endl;
            printColoredMessage(message, time_str, MAGENTA_COLOR);
            return;
        }
    }

    // 내부 에러 로그 파일에 기록
    log_file << "[" << time_str << "] INTERNAL ERROR: " << message << std::endl;
    log_file.close();

    // 콘솔에 보라색(MAGENTA)으로 내부 에러 메시지 출력
    printColoredMessage("INTERNAL ERROR: " + message, time_str, MAGENTA_COLOR);
}

void LogConfig::ensureLogDirectoryExists()
{
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
}
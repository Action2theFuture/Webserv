#include "Log.hpp"

// 서버 설정을 출력하는 함수
void LogConfig::printServerConfig(const ServerConfig &server, size_t index)
{
    std::cout << "Server " << index + 1 << ":" << std::endl;
    std::cout << "  Listen Port: " << server.port << std::endl;
    std::cout << "  Server Name: " << server.server_name << std::endl;
    std::cout << "  Root: " << server.root << std::endl;
    std::cout << "  Client Max Body Size: " << numberToString(server.client_max_body_size) << std::endl;
    std::cout << "  Error Pages:" << std::endl;
    for (std::map<int, std::string>::const_iterator it = server.error_pages.begin(); it != server.error_pages.end();
         ++it)
    {
        std::cout << "    " << it->first << " -> " << it->second << std::endl;
    }
    std::cout << "  Locations:" << std::endl;
    for (size_t j = 0; j < server.locations.size(); ++j)
    {
        const LocationConfig &location = server.locations[j];
        std::cout << "    Location " << j + 1 << ":" << std::endl;
        std::cout << "      Path: " << location.path << std::endl;
        std::cout << "      Allowed Methods: ";
        for (size_t k = 0; k < location.methods.size(); ++k)
        {
            std::cout << location.methods[k];
            if (k != location.methods.size() - 1)
            {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
        std::cout << "      Root: " << location.root << std::endl;
        std::cout << "      Upload Directory: " << location.upload_directory << std::endl;
        std::cout << "      Allowed Extensions: ";
        for (size_t k = 0; k < location.allowed_extensions.size(); ++k)
        {
            std::cout << location.allowed_extensions[k];
            if (k != location.allowed_extensions.size() - 1)
            {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
        std::cout << "      Client Max Body Size: " << numberToString(location.client_max_body_size) << std::endl;
        std::cout << "      Directory Listing: " << (location.directory_listing ? "on" : "off") << std::endl;
        std::cout << "      Index: " << location.index << std::endl;
        std::cout << "      CGI Extension: " << location.cgi_extension << std::endl;
        std::cout << "      CGI Path: " << location.cgi_path << std::endl;
        std::cout << std::endl;
    }
}

// 모든 서버 설정을 출력하는 함수
void LogConfig::printAllConfigurations(const std::vector<ServerConfig> &servers)
{
    std::cout << "==== Parsed Server Configurations ====" << std::endl;
    for (size_t i = 0; i < servers.size(); ++i)
    {
        printServerConfig(servers[i], i);
    }
    std::cout << "======================================" << std::endl;
}

void LogConfig::logError(const std::string &message)
{
    ensureLogDirectoryExists();
    // 로그 파일 열기
    std::ofstream log_file(LOG_FILE, std::ios::app);

    // 로그 파일 열기에 실패한 경우 파일 생성 시도
    if (!log_file.is_open())
    {
        // 파일 생성 시도
        std::ofstream create_file(LOG_FILE);
        if (!create_file.is_open())
        {
            std::cerr << "Error: Failed to create log file " << LOG_FILE << std::endl;
            // 표준 에러로만 출력
            std::cerr << message << std::endl;
            return;
        }
        create_file.close();
        log_file.open(LOG_FILE, std::ios::app);
        if (!log_file.is_open())
        {
            std::cerr << "Error: Failed to open newly created log file " << LOG_FILE << std::endl;
            // 표준 에러로만 출력
            std::cerr << message << std::endl;
            return;
        }
    }

    // 로그 파일에 기록
    log_file << message << std::endl;
    log_file.close();

    // 표준 에러에도 출력
    std::cerr << message << std::endl;
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
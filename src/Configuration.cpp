// Configuration.cpp
#include "Configuration.hpp"
#include "Utils.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

// 공백 및 주석 제거 함수 (Utils.cpp에 구현)
extern std::string trim(const std::string &str);

ServerConfig Configuration::initializeServerConfig()
{
    ServerConfig server;
    server.client_max_body_size = 1048576; // 기본값 1MB
    return server;
}

LocationConfig Configuration::initializeLocationConfig(const std::string &line)
{
    LocationConfig location;
    size_t start = line.find(' ') + 1;
    size_t end = line.find('{');
    if (start < end && end != std::string::npos)
    {
        location.path = trim(line.substr(start, end - start));
    }
    else
    {
        location.path = "/";
    }
    return location;
}

// 위치 설정 파싱
void Configuration::parseLocationConfig(const std::string &line, LocationConfig &location_config)
{
    std::istringstream iss(line);
    std::string key;
    iss >> key;

    if (key == "methods")
    {
        std::string method;
        while (iss >> method)
        {
            location_config.methods.push_back(method);
        }
    }
    else if (key == "redirect")
    {
        iss >> location_config.redirect;
    }
    else if (key == "directory_listing")
    {
        std::string value;
        iss >> value;
        location_config.directory_listing = (value == "on") ? true : false;
    }
    else if (key == "index")
    {
        iss >> location_config.index;
    }
    else if (key == "cgi_extension")
    {
        iss >> location_config.cgi_extension;
    }
    else if (key == "cgi_path")
    {
        iss >> location_config.cgi_path;
    }
    // 추가적인 위치 설정 항목 파싱
}

// 서버 설정 파싱
void Configuration::parseServerConfig(const std::string &line, ServerConfig &server_config)
{
    std::istringstream iss(line);
    std::string key;
    iss >> key;
    if (key == "listen")
    {
        iss >> server_config.port;
    }
    else if (key == "server_name")
    {
        iss >> server_config.server_name;
    }
    else if (key == "root")
    {
        iss >> server_config.root;
    }
    else if (key == "error_page")
    {
        int error_code;
        std::string error_path;
        while (iss >> error_code >> error_path)
        {
            std::string full_path;
            if (!server_config.root.empty())
            {
                // root 끝에 '/'가 없으면 추가 (선택적 로직)
                if (server_config.root[server_config.root.size() - 1] == '/')
                {
                    full_path = server_config.root + error_path;
                }
                else
                {
                    full_path = server_config.root + "/" + error_path;
                }
            }
            else
            {
                // root가 비어있다면 그대로 사용
                full_path = error_path;
            }
            server_config.error_pages[error_code] = full_path;
        }
    }
    else if (key == "client_max_body_size")
    {
        size_t size;
        iss >> size;
        server_config.client_max_body_size = size;
    }
    // 추가적인 서버 설정 항목 파싱
}

bool Configuration::parseConfigFile(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
    {
        std::cerr << "Failed to open config file: " << filename << std::endl;
        return false;
    }

    std::string line;
    ServerConfig current_server;
    LocationConfig current_location;
    bool in_server_block = false;
    bool in_location_block = false;

    while (getline(file, line))
    {
        // 공백 및 주석 제거
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        // 세미콜론으로 끝나는 라인 처리
        if (!line.empty() && line[line.size() - 1] == ';')
        {
            line.erase(line.size() - 1); // 세미콜론 제거
        }

        if (line.find("server {") != std::string::npos)
        {
            in_server_block = true;
            current_server = initializeServerConfig();
            continue;
        }

        if (in_server_block)
        {
            if (line.find("}") != std::string::npos)
            {
                if (in_location_block)
                {
                    // 위치 블록 종료
                    current_server.locations.push_back(current_location);
                    in_location_block = false;
                }
                else
                {
                    // 서버 블록 종료
                    servers.push_back(current_server);
                    in_server_block = false;
                }
                continue;
            }

            if (line.find("location") == 0 && line.find("{") != std::string::npos)
            {
                in_location_block = true;
                current_location = initializeLocationConfig(line);
                continue;
            }

            if (in_location_block)
            {
                if (line.find("}") != std::string::npos)
                {
                    // 위치 블록 종료
                    current_server.locations.push_back(current_location);
                    in_location_block = false;
                    continue;
                }
                parseLocationConfig(line, current_location);
            }
            else
            {
                // 서버 블록 내 설정 파싱
                parseServerConfig(line, current_server);
            }
        }
    }

    file.close();
    return true;
}

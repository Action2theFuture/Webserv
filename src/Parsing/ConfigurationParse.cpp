#include "Configuration.hpp"
#include "Utils.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

void Configuration::parseLocationConfig(const std::string &line, LocationConfig &location_config)
{
    std::istringstream iss(line);
    std::string key;
    iss >> key;
    if (key == "methods")
    {
        std::string method;
        while (iss >> method)
            location_config.methods.push_back(method);
    }
    else if (key == "root")
    {
        iss >> location_config.root;
    }
    else if (key == "redirect")
    {
        iss >> location_config.redirect;
    }
    else if (key == "directory_listing")
    {
        std::string value;
        iss >> value;
        location_config.directory_listing = (value == "on");
    }
    else if (key == "index")
    {
        iss >> location_config.index;
    }
    else if (key == "default_file")
    {
        iss >> location_config.default_file;
    }
    else if (key == "cgi_extension")
    {
        std::string ext;
        location_config.cgi_extension.clear();
        while (iss >> ext)
            location_config.cgi_extension.push_back(ext);
        std::cout << "DEBUG) CGI extensions parsed: ";
        std::vector<std::string>::iterator ite = location_config.cgi_extension.end();
        for (std::vector<std::string>::iterator it = location_config.cgi_extension.begin(); it != ite; ++it)
            std::cout << *it <<std::endl;
        std::cout << std::endl;
    }
    else if (key == "cgi_path")
    {
        std::string path;
        location_config.cgi_path.clear();
        while (iss >> path)
            location_config.cgi_path.push_back(path);
        std::cout << "DEBUG) CGI paths parsed: ";
        std::vector<std::string>::iterator ite = location_config.cgi_path.end();
        for (std::vector<std::string>::iterator it = location_config.cgi_path.begin(); it != ite; ++it)
            std::cout << *it <<std::endl;
        std::cout << std::endl;
    }
    else if (key == "upload_directory")
    {
        iss >> location_config.upload_directory;
    }
    else if (key == "allowed_extensions")
    {
        std::string ext;
        location_config.allowed_extensions.clear();
        while (iss >> ext)
            location_config.allowed_extensions.push_back(ext);
    }
}

// 서버 설정 한 줄 파싱 (약13줄)
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
        int code;
        std::string path;
        while (iss >> code >> path)
        {
            std::string full;
            if (!server_config.root.empty())
            {
                if (server_config.root[server_config.root.size() - 1] == '/')
                    full = server_config.root + path;
                else
                    full = server_config.root + "/" + path;
            }
            else
            {
                full = path;
            }
            server_config.error_pages[code] = full;
        }
    }
    else if (key == "client_max_body_size")
    {
        size_t sz;
        iss >> sz;
        server_config.client_max_body_size = sz;
    }
}

void Configuration::processServerLine(const std::string &line, ServerConfig &server_config)
{
    parseServerConfig(line, server_config);
}

void Configuration::processLocationLine(const std::string &line, LocationConfig &location_config)
{
    parseLocationConfig(line, location_config);
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
    bool in_server = false, in_location = false;
    while (getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        if (line[line.size() - 1] == ';')
            line.erase(line.size() - 1);
        if (line.find("server {") != std::string::npos)
        {
            in_server = true;
            current_server = initializeServerConfig();
            continue;
        }
        if (in_server)
        {
            if (line.find("}") != std::string::npos)
            {
                if (in_location)
                {
                    current_server.locations.push_back(current_location);
                    in_location = false;
                }
                else
                {
                    servers.push_back(current_server);
                    in_server = false;
                }
                continue;
            }
            if (line.find("location") == 0 && line.find("{") != std::string::npos)
            {
                in_location = true;
                current_location = initializeLocationConfig(line);
                continue;
            }
            if (in_location)
            {
                if (line.find("}") != std::string::npos)
                {
                    current_server.locations.push_back(current_location);
                    in_location = false;
                    continue;
                }
                processLocationLine(line, current_location);
            }
            else
            {
                processServerLine(line, current_server);
            }
        }
    }
    file.close();
    printConfiguration();
    return true;
}

#include "Configuration.hpp"
#include "Utils.hpp" // trim() 함수가 구현된 곳
#include <iostream>
#include <sstream>

static const std::string default_allowed_extensions_array[] = {".jpg", ".jpeg", ".png", ".gif", ".txt", ".pdf"};
static const std::vector<std::string> DEFAULT_ALLOWED_EXTENSIONS(default_allowed_extensions_array,
                                                                 default_allowed_extensions_array +
                                                                     sizeof(default_allowed_extensions_array) /
                                                                         sizeof(default_allowed_extensions_array[0]));

void Configuration::printConfiguration() const
{
    LogConfig::printAllConfigurations(servers);
}

ServerConfig Configuration::initializeServerConfig()
{
    ServerConfig server;
    server.client_max_body_size = 1048576; // 1MB 기본값
    return server;
}

LocationConfig Configuration::initializeLocationConfig(const std::string &line)
{
    LocationConfig location;
    size_t start = line.find(' ') + 1;
    size_t end = line.find('{');
    if (start < end && end != std::string::npos)
        location.path = trim(line.substr(start, end - start));
    else
        location.path = "/";
    location.allowed_extensions = DEFAULT_ALLOWED_EXTENSIONS;
    return location;
}

const LocationConfig *findBestMatchingLocation(const std::string &path, const ServerConfig &server_config)
{
    const LocationConfig *best_match = 0;
    size_t max_length = 0;
    for (size_t i = 0; i < server_config.locations.size(); ++i)
    {
        const LocationConfig &loc = server_config.locations[i];
        if (path.find(loc.path) == 0 && loc.path.length() > max_length)
        {
            max_length = loc.path.length();
            best_match = &server_config.locations[i];
        }
    }
    return best_match;
}

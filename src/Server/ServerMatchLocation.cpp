#include "Server.hpp"

const LocationConfig *matchLocationConfig(const Request &request, const ServerConfig &server_config)
{
    std::string req_path = request.getPath();
    const LocationConfig *matched_location = 0;
    size_t longest_match = 0;
    for (size_t loc = 0; loc < server_config.locations.size(); ++loc)
    {
        std::string loc_path = server_config.locations[loc].path;
        if (req_path.compare(0, loc_path.length(), loc_path) == 0)
        {
            if (loc_path.length() > longest_match)
            {
                matched_location = &server_config.locations[loc];
                longest_match = loc_path.length();
            }
        }
    }
    return matched_location;
}

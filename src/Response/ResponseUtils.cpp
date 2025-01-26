#include "ResponseUtils.hpp"

namespace ResponseUtils
{

// What To do (1 / 23)
// To ensure that the path is only called with the get method when fetching and executing a static file, you can use
// the
std::string buildRequestedPath(const std::string &path, const LocationConfig &location_config,
                               const ServerConfig &server_config)
{
    std::string requested_path;

    if (path == "/")
    {
        // root + index
        if (!location_config.root.empty())
        {
            if (location_config.root.back() == '/')
                requested_path = location_config.root + location_config.index;
            else
                requested_path = location_config.root + "/" + location_config.index;
        }
        else if (!server_config.root.empty())
        {
            if (server_config.root.back() == '/')
                requested_path = server_config.root + location_config.index;
            else
                requested_path = server_config.root + "/" + location_config.index;
        }
        else
        {
            requested_path = location_config.index;
        }
    }
    else
    {
        // root + path
        std::cerr << "path : " << path << std::endl;
        // Check if the requested path exactly matches the location path
        if (path == location_config.path && !location_config.index.empty())
        {
            // root + index (for exact location match)
            if (!location_config.root.empty())
            {
                if (location_config.root.back() == '/')
                    requested_path = location_config.root + location_config.index;
                else
                    requested_path = location_config.root + "/" + location_config.index;
            }
            else if (!server_config.root.empty())
            {
                if (server_config.root.back() == '/')
                    requested_path = server_config.root + location_config.index;
                else
                    requested_path = server_config.root + "/" + location_config.index;
            }
            else
            {
                requested_path = location_config.index;
            }
        }
        else
        {
            // root + path
            if (!location_config.root.empty())
            {
                if (location_config.root.back() == '/')
                    requested_path = location_config.root + path.substr(location_config.path.length());
                else
                    requested_path = location_config.root + "/" + path.substr(location_config.path.length());
            }
            else if (!server_config.root.empty())
            {
                if (server_config.root.back() == '/')
                    requested_path = server_config.root + path;
                else
                    requested_path = server_config.root + "/" + path;
            }
            else
            {
                requested_path = path;
            }
        }
    }

    // 경로 정규화 (필요 시)
    requested_path = normalizePath(requested_path);
    std::cerr << "requeted path : " << requested_path << std::endl;

    return requested_path;
}

std::string generateFileListJSON(const std::vector<std::string> &files)
{
    std::ostringstream json;
    json << "{ \"files\": [";
    for (size_t i = 0; i < files.size(); ++i)
    {
        json << "\"" << files[i] << "\"";
        if (i != files.size() - 1)
            json << ", ";
    }
    json << "] }";
    return json.str();
}

bool createSingleDir(const std::string &path)
{
    // 이미 존재하면 true, 에러면 false
    struct stat sb;
    if (stat(path.c_str(), &sb) == 0)
    {
        // 존재
        if (!S_ISDIR(sb.st_mode))
        {
            std::cerr << "Path exists but is not a directory: " << path << std::endl;
            return false;
        }
        // 디렉토리면 OK
        return true;
    }
    else
    {
        // 존재 안함 -> mkdir 시도
        if (mkdir(path.c_str(), 0755) < 0)
        {
            std::cerr << "mkdir() failed for " << path << ": " << strerror(errno) << std::endl;
            return false;
        }
        return true;
    }
}

bool ensureDirectoryExists(const std::string &fullPath)
{
    if (fullPath.empty())
        return false;

    // 구분자 '/' 기준으로 토큰화 (유닉스 경로 가정)
    std::vector<std::string> tokens;
    {
        std::string::size_type start = 0, pos;
        while ((pos = fullPath.find('/', start)) != std::string::npos)
        {
            if (pos > start)
                tokens.push_back(fullPath.substr(start, pos - start));
            start = pos + 1;
        }
        // 마지막 토큰
        if (start < fullPath.size())
            tokens.push_back(fullPath.substr(start));
    }

    // '.' 혹은 '' 처리가 필요할 수도 있음 (상대 경로)
    // 예: "./uploads" -> 첫 토큰이 '.'이 될 수도
    // 여기서는 간단히 무시 처리
    std::string pathSoFar;
    if (fullPath[0] == '/')
    {
        // 절대 경로면 시작 '/'
        pathSoFar = "/";
    }

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        if (!pathSoFar.empty() && pathSoFar[pathSoFar.size() - 1] != '/')
            pathSoFar += "/";
        pathSoFar += tokens[i];

        if (!createSingleDir(pathSoFar))
        {
            return false;
        }
    }
    return true;
}

bool isMethodAllowed(const std::string &method, const LocationConfig &location_config)
{
    std::string trimmed_method = trim(method);

    std::cout << "Path : " << location_config.path << std::endl;
    std::cout << "Request Method: [" << trimmed_method << "]" << std::endl;
    std::cout << "Allowed Methods: [";
    for (size_t i = 0; i < location_config.methods.size(); ++i)
    {
        std::cout << location_config.methods[i];
        if (i != location_config.methods.size() - 1)
        {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;

    for (size_t i = 0; i < location_config.methods.size(); ++i)
    {
        if (iequals(trimmed_method, location_config.methods[i]))
        {
            std::cout << "Method [" << trimmed_method << "] is allowed." << std::endl;
            return true;
        }
    }
    std::cout << "Method [" << trimmed_method << "] is NOT allowed." << std::endl;
    return false;
}

} // namespace ResponseUtils
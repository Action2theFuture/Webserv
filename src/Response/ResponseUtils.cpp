#include "ResponseUtils.hpp"

namespace ResponseUtils
{

// What To do (1 / 23)
// To ensure that the path is only called with the get method when fetching and executing a static file, you can use
// the
std::string buildRequestedPath(const std::string &original_path, const LocationConfig &location_config,
                               const ServerConfig &server_config)
{
    std::string requested_path;

    std::cerr << "path : " << original_path << std::endl;
    std::string path = normalizePath(original_path);
    if (path == "/")
    {
        // root + index
        if (!location_config.root.empty())
        {
            if (location_config.root[location_config.root.size() - 1] == '/')
                requested_path = location_config.root + location_config.index;
            else
                requested_path = location_config.root + "/" + location_config.index;
        }
        else if (!server_config.root.empty())
        {
            if (server_config.root[server_config.root.size() - 1] == '/')
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
        // Check if the requested path exactly matches the location path
        if (path == location_config.path && !location_config.index.empty())
        {
            // root + index (for exact location match)
            if (!location_config.root.empty())
            {
                if (location_config.root[location_config.root.size() - 1] == '/')
                    requested_path = location_config.root + location_config.index;
                else
                    requested_path = location_config.root + "/" + location_config.index;
            }
            else if (!server_config.root.empty())
            {
                if (server_config.root[server_config.root.size() - 1] == '/')
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
                if (location_config.root[location_config.root.size() - 1] == '/')
                    requested_path = location_config.root + path.substr(location_config.path.length());
                else
                    requested_path = location_config.root + "/" + path.substr(location_config.path.length());
            }
            else if (!server_config.root.empty())
            {
                if (server_config.root[server_config.root.size() - 1] == '/')
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
    std::cerr << "requested path : " << requested_path << std::endl;

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

bool validateUploadedFiles(const std::vector<UploadedFile> &files)
{
    return !files.empty();
}

bool getUploadDirectory(const LocationConfig &location_config, const ServerConfig &server_config,
                        std::string &upload_dir)
{
    upload_dir = location_config.upload_directory;
    if (upload_dir.empty())
    {
        upload_dir = server_config.root; // 기본 루트 디렉토리 사용
    }

    if (!ensureDirectoryExists(upload_dir))
    {
        std::string errorMsg = "Failed to ensure upload directory exists: " + upload_dir;
        LogConfig::logError(errorMsg + ": " + strerror(errno));
        return false;
    }

    return true;
}

bool isFileExtensionAllowed(const std::string &filename, const std::vector<std::string> &allowed_extensions)
{
    return isAllowedExtension(filename, allowed_extensions);
}

bool saveUploadedFile(const std::string &upload_dir, const UploadedFile &file, std::string &sanitized_filename)
{
    // 파일 이름 정제
    sanitized_filename = sanitizeFilename(file.filename);

    // 파일 경로 생성
    std::string file_path = upload_dir + "/" + sanitized_filename;

    // 파일 저장
    std::ofstream ofs(file_path.c_str(), std::ios::binary);
    if (!ofs.is_open())
    {
        std::string errorMsg = "Failed to open file for writing: " + file_path;
        LogConfig::logError(errorMsg + ": " + strerror(errno));
        return false;
    }

    if (!file.data.empty())
    {
        ofs.write(&file.data[0], file.data.size());
    }
    ofs.close();

    return true;
}

bool deleteUploadedFile(const std::string &upload_dir, const std::string &filename)
{
    std::string filePath = upload_dir + "/" + filename;

    // 파일 존재 여부 확인
    if (access(filePath.c_str(), F_OK) == -1)
    {
        std::string errorMsg = "File does not exist: " + filePath;
        LogConfig::logError(errorMsg + ": " + strerror(errno));
        return false;
    }

    // 파일 삭제
    if (remove(filePath.c_str()) != 0)
    {
        std::string errorMsg = "Failed to delete file: " + filePath;
        LogConfig::logError(errorMsg + ": " + strerror(errno));
        return false;
    }

    return true;
}

bool deleteAllUploadedFiles(const std::string &upload_dir)
{
    DIR *dir;
    struct dirent *ent;
    bool allDeleted = true;

    if ((dir = opendir(upload_dir.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            // 현재 디렉토리와 상위 디렉토리는 제외
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;

            // 파일인지 디렉토리인지 확인
            std::string filePath = upload_dir + "/" + ent->d_name;
            struct stat st;
            if (stat(filePath.c_str(), &st) == 0)
            {
                if (S_ISREG(st.st_mode))
                {
                    if (!deleteUploadedFile(upload_dir, ent->d_name))
                    {
                        allDeleted = false;
                        // 삭제 실패한 파일에 대한 로그는 deleteUploadedFile에서 이미 기록됨
                    }
                }
            }
        }
        closedir(dir);
        return allDeleted;
    }
    else
    {
        std::string errorMsg = "Failed to open directory for deletion: " + upload_dir;
        LogConfig::logError(errorMsg + ": " + strerror(errno));
        return false;
    }
}

bool listUploadedFiles(const std::string &upload_dir, std::vector<std::string> &files)
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(upload_dir.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            // 현재 디렉토리와 상위 디렉토리는 제외
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;

            // 파일인지 디렉토리인지 확인
            std::string filePath = upload_dir + "/" + ent->d_name;
            struct stat st;
            if (stat(filePath.c_str(), &st) == 0)
            {
                if (S_ISREG(st.st_mode))
                {
                    files.push_back(ent->d_name);
                }
            }
        }
        closedir(dir);
        return true;
    }
    else
    {
        std::string errorMsg = "Failed to open directory: " + upload_dir;
        LogConfig::logError(errorMsg + ": " + strerror(errno));
        return false;
    }
}

std::string generateSuccessResponse(const std::string &jsonContent)
{
    // 성공 응답 생성 (JSON 형식)
    std::string responseBody = jsonContent;
    return responseBody;
}
} // namespace ResponseUtils

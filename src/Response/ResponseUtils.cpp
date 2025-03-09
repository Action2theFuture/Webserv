#include "ResponseUtils.hpp"
#include "Log.hpp"
#include "Utils.hpp"
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

std::string ResponseUtil::buildRequestedPath(const std::string &original_path, const LocationConfig &location_config,
                                             const ServerConfig &server_config)
{
    std::string requested_path;
    std::string path = normalizePath(original_path);
    if (path == "/")
    {
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
        if (path == location_config.path && !location_config.index.empty())
        {
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
    requested_path = normalizePath(requested_path);
    return requested_path;
}

std::string ResponseUtil::generateFileListJSON(const std::vector<std::string> &files)
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

bool ResponseUtil::createSingleDir(const std::string &path)
{
    struct stat sb;
    if (stat(path.c_str(), &sb) == 0)
    {
        if (!S_ISDIR(sb.st_mode))
        {
            std::cerr << "Path exists but is not a directory: " << path << std::endl;
            return false;
        }
        return true;
    }
    else
    {
        if (mkdir(path.c_str(), 0755) < 0)
        {
            std::cerr << "mkdir() failed for " << path << ": " << strerror(errno) << std::endl;
            return false;
        }
        return true;
    }
}

bool ResponseUtil::ensureDirectoryExists(const std::string &fullPath)
{
    if (fullPath.empty())
        return false;
    std::vector<std::string> tokens;
    std::string::size_type start = 0, pos;
    while ((pos = fullPath.find('/', start)) != std::string::npos)
    {
        if (pos > start)
            tokens.push_back(fullPath.substr(start, pos - start));
        start = pos + 1;
    }
    if (start < fullPath.size())
        tokens.push_back(fullPath.substr(start));
    std::string pathSoFar;
    if (fullPath[0] == '/')
        pathSoFar = "/";
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        if (!pathSoFar.empty() && pathSoFar[pathSoFar.size() - 1] != '/')
            pathSoFar += "/";
        pathSoFar += tokens[i];
        if (!createSingleDir(pathSoFar))
            return false;
    }
    return true;
}

bool ResponseUtil::isMethodAllowed(const std::string &method, const LocationConfig &location_config)
{
    std::string trimmed_method = trim(method);
    for (size_t i = 0; i < location_config.methods.size(); ++i)
    {
        if (iequals(trimmed_method, location_config.methods[i]))
            return true;
    }
    return false;
}

bool ResponseUtil::validateUploadedFiles(const std::vector<UploadedFile> &files)
{
    return !files.empty();
}

bool ResponseUtil::getUploadDirectory(const LocationConfig &location_config, const ServerConfig &server_config,
                                      std::string &upload_dir)
{
    upload_dir = location_config.upload_directory;
    if (upload_dir.empty())
        upload_dir = server_config.root;
    if (!ensureDirectoryExists(upload_dir))
    {
        LogConfig::reportInternalError("Failed to ensure upload directory exists: " + upload_dir + ": " +
                                       strerror(errno));
        return false;
    }
    return true;
}

bool ResponseUtil::isFileExtensionAllowed(const std::string &filename,
                                          const std::vector<std::string> &allowed_extensions)
{
    return isAllowedExtension(filename, allowed_extensions);
}

bool ResponseUtil::saveUploadedFile(const std::string &upload_dir, const UploadedFile &file,
                                    std::string &sanitized_filename)
{
    std::string file_path = upload_dir + "/" + sanitized_filename;
    std::cout << "DEBUG) Uploading to :" << file_path << std::endl;
    std::ofstream ofs(file_path.c_str(), std::ios::binary);

    // Check if the directory is writable
    if (access(upload_dir.c_str(), W_OK) != 0)
    {
        LogConfig::reportInternalError("Upload directory is not writable: " + upload_dir);
        return false;
    }

    if (!ofs.is_open())
    {
        LogConfig::reportInternalError("Failed to open file for writing: " + file_path + ": " + strerror(errno));
        return false;
    }
    if (!file.data.empty())
        ofs.write(&file.data[0], file.data.size());
    ofs.close();
    return true;
}

bool ResponseUtil::listUploadedFiles(const std::string &upload_dir, std::vector<std::string> &files)
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(upload_dir.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;
            std::string filePath = upload_dir + "/" + ent->d_name;
            struct stat st;
            if (stat(filePath.c_str(), &st) == 0)
            {
                if (S_ISREG(st.st_mode))
                    files.push_back(ent->d_name);
            }
        }
        closedir(dir);
        return true;
    }
    else
    {
        LogConfig::reportInternalError("Failed to open directory: " + upload_dir + ": " + strerror(errno));
        return false;
    }
}

std::string ResponseUtil::generateSuccessResponse(const std::string &jsonContent)
{
    return jsonContent;
}

bool ResponseUtil::deleteUploadedFile(const std::string &upload_dir, const std::string &filename)
{
    std::string filePath = upload_dir + "/" + filename;
    if (access(filePath.c_str(), F_OK) == -1)
    {
        LogConfig::reportInternalError("File does not exist: " + filePath + ": " + strerror(errno));
        return false;
    }
    if (remove(filePath.c_str()) != 0)
    {
        LogConfig::reportInternalError("Failed to delete file: " + filePath + ": " + strerror(errno));
        return false;
    }
    return true;
}

bool ResponseUtil::deleteAllUploadedFiles(const std::string &upload_dir)
{
    DIR *dir;
    struct dirent *ent;
    bool allDeleted = true;
    if ((dir = opendir(upload_dir.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;
            std::string filePath = upload_dir + "/" + ent->d_name;
            struct stat st;
            if (stat(filePath.c_str(), &st) == 0)
            {
                if (S_ISREG(st.st_mode))
                {
                    if (!deleteUploadedFile(upload_dir, ent->d_name))
                        allDeleted = false;
                }
            }
        }
        closedir(dir);
        return allDeleted;
    }
    else
    {
        LogConfig::reportInternalError("Failed to open directory for deletion: " + upload_dir + ": " + strerror(errno));
        return false;
    }
}

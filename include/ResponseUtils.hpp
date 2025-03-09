#ifndef RESPONSEUTILS_HPP
#define RESPONSEUTILS_HPP

#include "Configuration.hpp"
#include "Response.hpp"
#include <string>
#include <vector>

class ResponseUtil
{
  public:
    static std::string buildRequestedPath(const std::string &original_path, const LocationConfig &location_config,
                                          const ServerConfig &server_config);
    static bool isMethodAllowed(const std::string &method, const LocationConfig &location_config);
    static bool ensureDirectoryExists(const std::string &fullPath);
    static bool createSingleDir(const std::string &path);
    static std::string generateFileListJSON(const std::vector<std::string> &files);
    static bool validateUploadedFiles(const std::vector<UploadedFile> &files);
    static bool getUploadDirectory(const LocationConfig &location_config, const ServerConfig &server_config,
                                   std::string &upload_dir);
    static bool isFileExtensionAllowed(const std::string &filename, const std::vector<std::string> &allowed_extensions);
    static bool saveUploadedFile(const std::string &upload_dir, const UploadedFile &file,
                                 std::string &sanitized_filename);
    static bool listUploadedFiles(const std::string &upload_dir, std::vector<std::string> &files);
    static std::string generateSuccessResponse(const std::string &jsonContent);
    static bool deleteUploadedFile(const std::string &upload_dir, const std::string &filename);
    static bool deleteAllUploadedFiles(const std::string &upload_dir);
};

#endif // RESPONSEUTILS_HPP

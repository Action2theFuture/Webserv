#ifndef RESPONSEUTILS_HPP
#define RESPONSEUTILS_HPP

#include "Response.hpp"

namespace ResponseUtils
{
std::string buildRequestedPath(const std::string &original_path, const LocationConfig &location_config,
                               const ServerConfig &server_config);

// 메서드 허용 검사
bool isMethodAllowed(const std::string &method, const LocationConfig &location_config);
// 업로드 디렉토리 생성
bool ensureDirectoryExists(const std::string &fullPath);
bool createSingleDir(const std::string &path);
std::string generateFileListJSON(const std::vector<std::string> &files);
bool validateUploadedFiles(const std::vector<UploadedFile> &files);
bool getUploadDirectory(const LocationConfig &location_config, const ServerConfig &server_config,
                        std::string &upload_dir);
bool isFileExtensionAllowed(const std::string &filename, const std::vector<std::string> &allowed_extensions);
bool saveUploadedFile(const std::string &upload_dir, const UploadedFile &file, std::string &sanitized_filename);
bool listUploadedFiles(const std::string &upload_dir, std::vector<std::string> &files);
std::string generateSuccessResponse(const std::string &jsonContent);
bool deleteUploadedFile(const std::string &upload_dir, const std::string &filename);
bool deleteAllUploadedFiles(const std::string &upload_dir);
} // namespace ResponseUtils

#endif
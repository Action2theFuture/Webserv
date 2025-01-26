#ifndef RESPONSEUTILS_HPP
#define RESPONSEUTILS_HPP

#include "Response.hpp"

namespace ResponseUtils
{
std::string buildRequestedPath(const std::string &path, const LocationConfig &location_config,
                               const ServerConfig &server_config);

// 메서드 허용 검사
bool isMethodAllowed(const std::string &method, const LocationConfig &location_config);
// 업로드 디렉토리 생성
bool ensureDirectoryExists(const std::string &fullPath);
bool createSingleDir(const std::string &path);
std::string generateFileListJSON(const std::vector<std::string> &files);

} // namespace ResponseUtils

#endif
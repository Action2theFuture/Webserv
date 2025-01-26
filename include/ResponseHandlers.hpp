#ifndef RESPONSEHANDLERS_HPP
#define RESPONSEHANDLERS_HPP

#include "Response.hpp"

namespace ResponseHandlers
{
// 리디렉션 처리
Response handleRedirection(const LocationConfig &location_config);
// CGI 처리
Response handleCGI(const Request &request, const std::string &real_path, const ServerConfig &server_config);
// 정적 파일 처리
Response handleStaticFile(const std::string &real_path, const ServerConfig &server_config);
// 파일 업로드 처리
Response handleUpload(const std::string &real_path, const Request &request, const LocationConfig &location_config,
                      const ServerConfig &server_config);
Response handleFileList(const Request &request, const LocationConfig &location_config,
                        const ServerConfig &server_config);

}; // namespace ResponseHandlers

#endif // RESPONSEHANDLER_HPP
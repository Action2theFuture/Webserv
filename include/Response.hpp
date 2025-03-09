#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "CGIHandler.hpp" // 필요 시
#include "Configuration.hpp"
#include "Define.hpp"
#include "Log.hpp"
#include "Request.hpp"
#include "ResponseUtils.hpp"
#include "ServerConfig.hpp"
#include "Utils.hpp"

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <map>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

class Response
{
  public:
    Response();
    ~Response();

    static Response createResponse(const Request &request, const LocationConfig &location_config,
                                   const ServerConfig &server_config);
    static Response buildResponse(const Request &request, const ServerConfig &server_config,
                                  const LocationConfig *location_config);
    static Response createErrorResponse(int status, const ServerConfig &server_config);

    std::string toString() const;
    std::string getStatus() const;

    void setStatus(const std::string &status_code);
    void setHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &content);

    void setCookie(const std::string &key, const std::string &value, const std::string &path = "/", int max_age = 0);

  private:
    std::string _status;
    std::map<std::string, std::string> _headers;
    std::string _body;

    static std::string readErrorPageFromFile(const std::string &file_path, int status);

    static bool validateMethod(const Request &request, const LocationConfig &location_config);
    static bool getRealPath(const std::string &path, const LocationConfig &location_config,
                            const ServerConfig &server_config, std::string &real_path);
    static bool isCGIRequest(const std::string &real_path, const LocationConfig &location_config);

    Response handleGetFileList(const LocationConfig &location_config, const ServerConfig &server_config);
    Response handleDeleteFile(const Request &request, const LocationConfig &location_config,
                              const ServerConfig &server_config);
};

#endif // RESPONSE_HPP

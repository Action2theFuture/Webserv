#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

class Request;
class Response;

class CGIHandler
{
  public:
    CGIHandler();
    ~CGIHandler();

    bool execute(const Request &request, const std::string &script_path, std::string &cgi_output,
                 std::string &content_type);

  private:
    void setEnvironmentVariables(const Request &request, const std::string &script_path);

    CGIHandler(const CGIHandler &);
    CGIHandler &operator=(const CGIHandler &);
};

#endif // CGIHANDLER_HPP

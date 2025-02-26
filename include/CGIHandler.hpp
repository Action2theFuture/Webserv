#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <cstdlib>
#include <cstring>
#include <errno.h> // errno
#include <fcntl.h> // open, fcntl
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>  // struct stat
#include <sys/types.h> // data types
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

    // CGI 스크립트를 실행하고 결과를 반환
    bool execute(const Request &request, const std::string &script_path, std::string &cgi_output,
                 std::string &content_type);

  private:
    // 환경 변수 설정 함수
    void setEnvironmentVariables(const Request &request, const std::string &script_path);

    // 복사 금지 (C++98에서는 private 선언만으로 충분)
    CGIHandler(const CGIHandler &);
    CGIHandler &operator=(const CGIHandler &);
};

#endif // CGIHANDLER_HPP

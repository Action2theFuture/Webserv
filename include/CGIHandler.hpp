#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <map>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <sstream>
#include <fcntl.h>      // open, fcntl
#include <sys/stat.h>   // struct stat
#include <sys/types.h>  // data types
#include <errno.h>      // errno

class Request;
class Response;

class CGIHandler {
public:
    CGIHandler();
    ~CGIHandler();

    // CGI 스크립트를 실행하고 결과를 반환
    bool execute(const Request &request, const std::string &script_path, std::string &cgi_output, std::string &content_type);

private:
    // 환경 변수 설정 함수
    void setEnvironmentVariables(const Request &request, const std::string &script_path);

    // 복사 금지 (C++98에서는 private 선언만으로 충분)
    CGIHandler(const CGIHandler &);
    CGIHandler &operator=(const CGIHandler &);
};

#endif // CGIHANDLER_HPP

#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <map>
#include <string>
#include <vector>

struct ServerConfig; // 전방 선언

struct LocationConfig
{
    std::string path;
    std::vector<std::string> methods;
    std::string redirect;
    bool directory_listing;
    std::string index;
    std::string cgi_extension;
    std::string cgi_path;
    std::map<int, std::string> error_pages; // location별 에러 페이지 (없으면 비어있음)
    // 추가적인 설정 항목

    // 생성자: 기본값 설정
    LocationConfig()
        : path("/"), redirect(""), directory_listing(false), index("index.html"), cgi_extension(""), cgi_path("")
    {
    }
};

#endif // LOCATIONCONFIG_HPP

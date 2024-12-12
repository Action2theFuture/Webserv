#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>
#include <vector>

struct LocationConfig {
    std::string path;
    std::vector<std::string> methods;
    std::string redirect;
    std::string root;
    bool directory_listing;
    std::string index;
    std::string cgi_extension;
    std::string cgi_path;
    // 추가적인 설정 항목

    // 생성자: 기본값 설정
    LocationConfig()
        : path("/"),
          redirect(""),
          root(""),
          directory_listing(false),
          index("index.html"),
          cgi_extension(""),
          cgi_path("") {}
};

#endif // LOCATIONCONFIG_HPP

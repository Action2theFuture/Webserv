#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <map>
#include <string>
#include <vector>

struct ServerConfig; // 전방 선언

struct UploadedFile
{
    std::string name;         // 폼 필드 이름
    std::string filename;     // 원본 파일 이름
    std::string content_type; // 파일의 Content-Type
    std::vector<char> data;   // 파일 데이터
};

struct LocationConfig
{
    std::string root;
    std::string path;
    std::vector<std::string> methods;
    std::string redirect;
    bool directory_listing;
    std::string index;
    std::string cgi_extension;
    std::string cgi_path;
    std::map<int, std::string> error_pages; // location별 에러 페이지 (없으면 비어있음)
    std::string default_file;               // default file for directory request

    // 업로드 관련 설정
    std::string upload_directory;
    std::vector<std::string> allowed_extensions;
    // 추가적인 설정 항목

    LocationConfig()
        : path("/"), redirect(""), directory_listing(false), index("index.html"), cgi_extension(""), cgi_path("")
    {
    }
};

#endif // LOCATIONCONFIG_HPP

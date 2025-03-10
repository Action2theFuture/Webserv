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
    size_t filesize;
};

struct LocationConfig
{
    std::string root;
    std::string path;
    std::vector<std::string> methods;
    std::string redirect;
    std::string index;
    std::vector<std::string> cgi_extension;
    std::vector<std::string> cgi_path;
    std::map<int, std::string> error_pages; // location별 에러 페이지 (없으면 비어있음)
    std::string default_file;               // default file for directory request
    bool directory_listing;
    size_t client_max_body_size;            // 최대 요청 본문 크기 (바이트)


    // 업로드 관련 설정
    std::string upload_directory;
    std::vector<std::string> allowed_extensions;
    // 추가적인 설정 항목

    LocationConfig()
        : path("/"), redirect(""), index("index.html"), 
        directory_listing(false), client_max_body_size(1)
    {
    }
};

#endif // LOCATIONCONFIG_HPP

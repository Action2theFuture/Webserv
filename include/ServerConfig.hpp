#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include "LocationConfig.hpp" // LocationConfig 포함
#include <map>
#include <netinet/in.h> // sockaddr_in
#include <string>
#include <vector>

// 서버 블록 설정 구조체
struct ServerConfig
{
    int port;                               // 서버가 청취할 포트
    std::string server_name;                // 서버 이름
    std::string root;                       // 루트 디렉토리 경로
    unsigned long client_max_body_size;     // 최대 요청 본문 크기 (바이트)
    std::vector<LocationConfig> locations;  // 위치 블록 리스트
    std::map<int, std::string> error_pages; // 에러 코드에 대한 에러 페이지 경로 매핑
    std::vector<int> server_sockets;        // 서버 소켓 리스트

    // 생성자: 기본값 설정
    ServerConfig() {};

    ~ServerConfig() {};
};

#endif // SERVERCONFIG_HPP

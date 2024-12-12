#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include "Poller.hpp"
#include "Response.hpp" // Response 헤더 포함
#include "ServerConfig.hpp" // 서버 구성 구조체 포함

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <fcntl.h>
#include <algorithm>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <vector>
#include <map>
#include <set> 

class Server {
public:
    // 생성자: configFile만 인자로 받음
    Server(const std::string &configFile);
    ~Server();

    void start();

private:
    std::vector<ServerConfig> server_configs; // 서버 구성 리스트
    Poller *poller; // Poller 인터페이스 포인터

    // 소켓 초기화
    void initSockets();

    // 클라이언트 요청 처리
    void handleClientRead(int client_fd, const ServerConfig &server_config);
    void handleClientWrite(int client_fd);

    // 응답 전송
    void sendResponse(int client_fd, const Response &response);

    // 안전한 클라이언트 소켓 닫기
    void safelyCloseClient(int client_fd);

    // C++98 호환을 위한 int to string 변환 함수 선언
    std::string intToString(int number) const;
};

#endif // SERVER_HPP

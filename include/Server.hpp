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

   // 이벤트 핸들링
    bool processPollerEvents(std::vector<Event> &events);
    void processEvents(const std::vector<Event> &events);

    // 새로운 연결 처리
    void handleNewConnection(int server_fd);

    // 클라이언트 요청 처리
    void handleClientRead(int client_fd, const ServerConfig &server_config);
    void processClientRequest(int client_fd, const ServerConfig &server_config, const std::string &request_str);
    void handleClientReadError(int client_fd);
    void handleClientWrite(int client_fd);

    // 응답 전송
    void sendResponse(int client_fd, const Response &response);
    void sendBadRequestResponse(int client_fd);

    // 위치 매칭
    const LocationConfig *matchLocationConfig(const Request &request, const ServerConfig &server_config) const;

    // 안전한 클라이언트 소켓 닫기
    void safelyCloseClient(int client_fd);

    // 유틸리티
    ServerConfig &findMatchingServerConfig(int fd);
    bool setNonBlocking(int fd);
    bool isServerSocket(int fd, ServerConfig **matched_server) const;

    // C++98 호환을 위한 int to string 변환 함수 선언
    std::string intToString(int number) const;

    Response generateResponse(const Request &request, const ServerConfig &server_config, const LocationConfig *location_config);

    // 소켓 생성 함수
    int createSocket(int port);

    // 소켓 옵션 설정 함수
    void setSocketOptions(int sockfd, int port);

    // 소켓 비차단 모드 설정 함수
    void setSocketNonBlocking(int sockfd, int port);

    // 소켓 바인드 함수
    void bindSocket(int sockfd, int port);

    // 소켓 리스닝 함수
    void startListening(int sockfd, int port);
};

#endif // SERVER_HPP

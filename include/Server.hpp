#ifndef SERVER_HPP
#define SERVER_HPP

#include "Poller.hpp"
#include "RequestHandler.hpp"
#include "Response.hpp"
#include "ServerConfig.hpp"
#include "SocketManager.hpp"
#include "Utils.hpp"
#include <iostream>
#include <set>
#include <string>
#include <vector>

class Server
{
  public:
    // 생성자 및 소멸자
    Server(const std::string &configFile);
    ~Server();

    // 서버 실행
    void start();

  private:
    std::vector<ServerConfig> server_configs; // 서버 구성 리스트
    Poller *poller;                           // Poller 인터페이스 포인터

    // 소켓 초기화
    void initSockets();

    // Poller 이벤트 처리
    bool processPollerEvents(std::vector<Event> &events);
    void processEvents(const std::vector<Event> &events);

    // 새 연결 처리
    void handleNewConnection(int server_fd);

    // 클라이언트 요청 처리
    void handleClientRead(int client_fd, const ServerConfig &server_config);
    void handleClientWrite(int client_fd);
    void safelyCloseClient(int client_fd);

    // 비차단 모드 설정
    bool setNonBlocking(int fd);

    // 파일 디스크립터가 서버 소켓인지 확인
    bool isServerSocket(int fd, ServerConfig **matched_server) const;

    // 파일 디스크립터에 해당하는 서버 구성 찾기
    ServerConfig &findMatchingServerConfig(int fd);
};

#endif // SERVER_HPP

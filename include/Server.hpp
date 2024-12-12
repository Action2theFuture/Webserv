#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>

#include "Poller.hpp" // Poller 추상화 클래스 헤더

class Poller; // Forward declaration

class Server {
public:
    Server(const std::string &configFile);
    ~Server();
    
    void start();

private:
    void initSockets();
    void parseConfig(const std::string &configFile);
    
    // 서버 설정 변수
    std::vector<int> ports;
    std::vector<struct sockaddr_in> addresses;

    // 소켓 관련 변수
    std::vector<int> server_sockets;

    // Poller 객체
    Poller* poller;
};

#endif

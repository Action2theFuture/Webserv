#ifndef SERVER_HPP
#define SERVER_HPP

#include "Configuration.hpp"
#include "Log.hpp"

#ifdef __linux__
#include "EpollPoller.hpp"
#elif defined(__APPLE__)
#include "KqueuePoller.hpp"
#endif

#include "Poller.hpp"
#include "Response.hpp"
#include "ServerConfig.hpp"
#include "SocketManager.hpp"
#include "Utils.hpp"

#include <csignal>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

class Server
{
  public:
    Server(const std::string &configFile);
    ~Server();
    void start();
    // graceful shutdown을 위한 stop() 메소드 추가
    void stop();

  private:
    // Rule of Three 준수를 위해 복사 생성자와 복사 대입 연산자를 private으로 선언 (정의하지 않음)
    Server(const Server &);
    Server &operator=(const Server &);

    // private 멤버 변수에 언더바 접두사 추가
    std::vector<ServerConfig> _server_configs;
    Poller *_poller;
    std::map<int, std::string> _partialRequests;
    std::map<int, std::string> _outgoingData;
    std::map<int, Request> _requestMap;

    bool _is_running;

    // [ServerCore.cpp]
    void initSockets();
    bool processPollerEvents(std::vector<Event> &events);

    // [ServerEvents.cpp]
    void processEvents(const std::vector<Event> &events);
    void handleNewConnection(int server_fd);
    void handleClientRead(int client_fd, const ServerConfig &server_config);
    bool readClientData(int client_fd, std::string &buffer);
    bool handleReceivedData(int client_fd, const ServerConfig &server_config, std::string &buffer);

    // [ServerWrite.cpp]
    void writePendingData(int client_fd);
    bool checkKeepAliveNeeded(int client_fd);
    void handleClientWrite(int client_fd);
    bool setNonBlocking(int fd);
    bool isServerSocket(int fd, ServerConfig **matched_server) const;

    // [ServerUtils.cpp]
    static std::set<int> &getClosedFds();
    ServerConfig &findMatchingServerConfig(int fd);
    void safelyCloseClient(int client_fd);
    bool processClientRequest(int client_fd, const ServerConfig &server_config, const std::string &request_str,
                              int &consumed);
    void sendResponse(int client_fd, const Response &response);
    void sendBadRequestResponse(int client_fd, const ServerConfig &server_config);
};

#endif // SERVER_HPP

#include "Server.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <fcntl.h>
#include <algorithm>

#ifdef __linux__
#include "EpollPoller.hpp"
#elif defined(__APPLE__)
#include "KqueuePoller.hpp"
#endif

Server::Server(const std::string &configFile) {
    parseConfig(configFile);
    initSockets();
    
    // 운영 체제에 따라 Poller 객체 생성
#ifdef __linux__
    poller = new EpollPoller();
#elif defined(__APPLE__)
    poller = new KqueuePoller();
#else
    #error "Unsupported OS"
#endif

    // 소켓을 Poller에 추가
    for (size_t i = 0; i < server_sockets.size(); ++i) {
        poller->add(server_sockets[i], POLLER_READ);
    }
}

Server::~Server() {
    for (size_t i = 0; i < server_sockets.size(); ++i) {
        close(server_sockets[i]);
    }
    delete poller;
}

void Server::parseConfig(const std::string &configFile) {
    // 간단한 설정 파일 파싱 로직 (포트 등)
    std::ifstream file(configFile.c_str());
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << configFile << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string line;
    while (getline(file, line)) {
        // 공백 및 주석 처리
        if (line.empty() || line[0] == '#')
            continue;

        if (line.find("port") != std::string::npos) {
            std::istringstream iss(line);
            std::string key;
            int port;
            iss >> key >> port;
            ports.push_back(port);
        }
        // 추가 설정 파싱 (server_name, root, location 등)
    }
    file.close();
}

void Server::initSockets() {
    for (size_t i = 0; i < ports.size(); ++i) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        // 소켓 옵션 설정
        int opt = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        // 비차단 모드 설정
        int flags = fcntl(sockfd, F_GETFL, 0);
        if (flags == -1) {
            perror("fcntl");
            exit(EXIT_FAILURE);
        }
        if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
            perror("fcntl");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(ports[i]);

        if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("bind");
            exit(EXIT_FAILURE);
        }

        if (listen(sockfd, SOMAXCONN) < 0) {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        server_sockets.push_back(sockfd);
        addresses.push_back(addr);
    }
}

void Server::start() {
    std::vector<Event> events;

    while (true) {
        int n = poller->poll(events, -1);
        if (n == -1) {
            perror("poll");
            continue;
        }

        for (size_t i = 0; i < events.size(); ++i) {
            if (events[i].events & POLLER_READ) {
                // 새로운 연결 수락
                if (std::find(server_sockets.begin(), server_sockets.end(), events[i].fd) != server_sockets.end()) {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(events[i].fd, (struct sockaddr *)&client_addr, &client_len);
                    if (client_fd == -1) {
                        perror("accept");
                        continue;
                    }

                    // 비차단 모드 설정
                    int flags = fcntl(client_fd, F_GETFL, 0);
                    if (flags == -1) {
                        perror("fcntl");
                        close(client_fd);
                        continue;
                    }
                    if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                        perror("fcntl");
                        close(client_fd);
                        continue;
                    }

                    // Poller에 추가 (읽기 가능 이벤트)
                    poller->add(client_fd, POLLER_READ);
                    
                    // 클라이언트 관리 (Client 객체 생성 등)
                    // 예: clients[client_fd] = new Client(client_fd);
                }
                else {
                    // 클라이언트 요청 처리
                    // 예: clients[events[i].fd]->handleRead();
                }
            }

            if (events[i].events & POLLER_WRITE) {
                // 클라이언트에게 응답 쓰기
                // 예: clients[events[i].fd]->handleWrite();
            }
        }
    }
}

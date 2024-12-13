#include "Server.hpp"
#include "Configuration.hpp"

#ifdef __linux__
#include "EpollPoller.hpp"
#elif defined(__APPLE__)
#include "KqueuePoller.hpp"
#endif

// Server 클래스 생성자
Server::Server(const std::string &configFile) : poller(NULL) {
    Configuration config;
    if (!config.parseConfigFile(configFile)) {
        logError("Failed to parse configuration file.");
        exit(EXIT_FAILURE);
    }

    server_configs = config.servers;

#ifdef __linux__
    poller = new EpollPoller();
#elif defined(__APPLE__)
    poller = new KqueuePoller();
#else
    #error "Unsupported OS"
#endif

    initSockets();
}

// Server 클래스 소멸자
Server::~Server() {
    for (size_t i = 0; i < server_configs.size(); ++i) {
        for (size_t j = 0; j < server_configs[i].server_sockets.size(); ++j) {
            close(server_configs[i].server_sockets[j]);
        }
    }
    delete poller;
}

// 소켓 초기화
void Server::initSockets() {
    for (size_t i = 0; i < server_configs.size(); ++i) {
        ServerConfig &server = server_configs[i];

        int sockfd = SocketManager::createSocket(server.port);
        SocketManager::setSocketOptions(sockfd, server.port);
        SocketManager::setSocketNonBlocking(sockfd, server.port);
        SocketManager::bindSocket(sockfd, server.port);
        SocketManager::startListening(sockfd, server.port);

        server.server_sockets.push_back(sockfd);
        poller->add(sockfd, POLLER_READ);
    }
}

// 서버 시작 및 이벤트 루프
void Server::start() {
    while (true) {
        std::vector<Event> events;
        if (!processPollerEvents(events)) {
            logError("Failed to poll events.");
            continue;
        }
        processEvents(events);
    }
}

// Poller 이벤트 처리
bool Server::processPollerEvents(std::vector<Event> &events) {
    int n = poller->poll(events, -1);
    if (n == -1) {
        logError("poller->poll() failed: " + std::string(strerror(errno)));
        return false;
    }
    return true;
}

// 이벤트 목록 처리
void Server::processEvents(const std::vector<Event> &events) {
    for (size_t i = 0; i < events.size(); ++i) {
        int fd = events[i].fd;

        if (fd < 0) {
            logError("Invalid file descriptor in events[" + intToString(i) + "]");
            continue;
        }

        if (events[i].events & POLLER_READ) {
            ServerConfig *matched_server = NULL;
            if (isServerSocket(fd, &matched_server)) {
                handleNewConnection(fd);
            } else {
                handleClientRead(fd, findMatchingServerConfig(fd));
            }
        }

        if (events[i].events & POLLER_WRITE) {
            handleClientWrite(fd);
        }
    }
}

// 새 연결 처리
void Server::handleNewConnection(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1) {
        logError("accept() failed: " + std::string(strerror(errno)));
        return;
    }

    if (!setNonBlocking(client_fd)) {
        logError("Failed to set non-blocking mode for client_fd " + intToString(client_fd));
        close(client_fd);
        return;
    }

    if (!poller->add(client_fd, POLLER_READ)) {
        logError("Failed to add client_fd " + intToString(client_fd) + " to poller");
        close(client_fd);
    }
}

// 클라이언트 요청 처리
void Server::handleClientRead(int client_fd, const ServerConfig &server_config) {
    char buffer[4096];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        if (!RequestHandler::processClientRequest(client_fd, server_config, std::string(buffer)))
            safelyCloseClient(client_fd);
    } else if (bytes_read == 0) {
        safelyCloseClient(client_fd);
    } else {
        if (errno == ECONNRESET || errno == EAGAIN || errno == EWOULDBLOCK) {
            safelyCloseClient(client_fd);
        } else {
            logError("recv() failed for client_fd " + intToString(client_fd) + ": " + strerror(errno));
            safelyCloseClient(client_fd);
        }
    }
}

// 클라이언트 쓰기 처리
void Server::handleClientWrite(int client_fd) {
    // 현재 단순화된 처리 (확장 가능)
    (void)client_fd;
}

// 비차단 모드 설정
bool Server::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        logError("Failed to set non-blocking mode for fd " + intToString(fd));
        return false;
    }
    return true;
}

// 파일 디스크립터가 서버 소켓인지 확인
bool Server::isServerSocket(int fd, ServerConfig **matched_server) const {
    for (size_t s = 0; s < server_configs.size(); ++s) {
        for (size_t sock = 0; sock < server_configs[s].server_sockets.size(); ++sock) {
            if (fd == server_configs[s].server_sockets[sock]) {
                if (matched_server) {
                    *matched_server = const_cast<ServerConfig *>(&server_configs[s]);
                }
                return true;
            }
        }
    }
    return false;
}

// 파일 디스크립터에 해당하는 서버 구성 찾기
ServerConfig &Server::findMatchingServerConfig(int fd) {
    for (size_t s = 0; s < server_configs.size(); ++s) {
        for (size_t sock = 0; sock < server_configs[s].server_sockets.size(); ++sock) {
            if (fd == server_configs[s].server_sockets[sock]) {
                return server_configs[s];
            }
        }
    }
    return server_configs[0];
}

// 안전한 클라이언트 소켓 닫기
void Server::safelyCloseClient(int client_fd) {
    static std::set<int> closed_fds;

    if (closed_fds.find(client_fd) != closed_fds.end()) {
        return; // 이미 닫힌 fd
    }

    poller->remove(client_fd);
    close(client_fd);
    closed_fds.insert(client_fd);
}


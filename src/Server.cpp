#include "Server.hpp"
#include "Response.hpp"
#include "Request.hpp"
#include "Utils.hpp" // log_error, trim, normalizePath, getMimeType 함수 포함
#include "CGIHandler.hpp"
#include "ServerConfig.hpp"

#ifdef __linux__
#include "EpollPoller.hpp"
#elif defined(__APPLE__)
#include "KqueuePoller.hpp"
#endif

// Server 클래스 생성자
Server::Server(const std::string &configFile)
    : poller(NULL) { // poller는 동적으로 생성
    Configuration config;
    if (!config.parseConfigFile(configFile)) {
        logError("Failed to parse configuration file.");
        exit(EXIT_FAILURE);
    }

    server_configs = config.servers;

    // 운영 체제에 따라 Poller 객체 생성
#ifdef __linux__
    poller = new EpollPoller();
#elif defined(__APPLE__)
    poller = new KqueuePoller();
#else
    #error "Unsupported OS"
#endif

    // 소켓 초기화 및 Poller에 추가
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

        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            logError("socket() failed for port " + intToString(server.port));
            exit(EXIT_FAILURE);
        }

        // 소켓 옵션 설정
        int opt = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            logError("setsockopt() failed for port " + intToString(server.port));
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // 비차단 모드 설정
        int flags = fcntl(sockfd, F_GETFL, 0);
        if (flags == -1) {
            logError("fcntl(F_GETFL) failed for port " + intToString(server.port));
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
            logError("fcntl(F_SETFL) failed for port " + intToString(server.port));
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(server.port);

        if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            logError("bind() failed for port " + intToString(server.port));
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        if (listen(sockfd, SOMAXCONN) < 0) {
            logError("listen() failed for port " + intToString(server.port));
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        server.server_sockets.push_back(sockfd);
        poller->add(sockfd, POLLER_READ);
    }
}

// 서버 시작 및 이벤트 루프
void Server::start() {
    std::vector<Event> events;

    while (true) {
        int n = poller->poll(events, -1);
        if (n == -1) {
            logError("poller->poll() failed");
            continue;
        }

        for (size_t i = 0; i < events.size(); ++i) {
            int fd = events[i].fd;

            // 소켓 유효성 확인
            if (fd < 0) {
                logError("Invalid file descriptor in events[" + intToString(i) + "]");
                continue;
            }

            if (events[i].events & POLLER_READ) {
                // 새로운 연결 수락
                bool is_server_socket = false;
                ServerConfig *matched_server = NULL;
                for (size_t s = 0; s < server_configs.size(); ++s) {
                    for (size_t sock = 0; sock < server_configs[s].server_sockets.size(); ++sock) {
                        if (events[i].fd == server_configs[s].server_sockets[sock]) {
                            is_server_socket = true;
                            matched_server = &server_configs[s];
                            break;
                        }
                    }
                    if (is_server_socket)
                        break;
                }

                if (is_server_socket && matched_server != NULL) {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(events[i].fd, (struct sockaddr *)&client_addr, &client_len);
                    if (client_fd == -1) {
                        logError("accept() failed" + std::string(strerror(errno)));
                        continue;
                    }

                    // 비차단 모드 설정
                    int flags = fcntl(client_fd, F_GETFL, 0);
                    if (flags == -1 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                        logError("Failed to set non-blocking mode for client_fd " + intToString(client_fd));
                        close(client_fd);
                        continue;
                    }

                    // Poller에 추가
                    if (!poller->add(client_fd, POLLER_READ)) {
                        logError("Failed to add client_fd " + intToString(client_fd) + " to poller");
                        close(client_fd);
                    }

                    // Poller에 추가 (읽기 가능 이벤트)
                    if (!poller->add(client_fd, POLLER_READ)) {
                        logError("Failed to add client_fd " + intToString(client_fd) + " to poller");
                        close(client_fd);
                    }
                }
                else {
                    // 클라이언트 요청 처리
                    // 현재 예시에서는 첫 번째 서버 설정을 사용
                    handleClientRead(fd, server_configs[0]);
                }
            }

            if (events[i].events & POLLER_WRITE) {
                // 클라이언트에게 응답 쓰기
                handleClientWrite(fd);
            }
        }

        // 이벤트 벡터 초기화
        events.clear();
    }
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

// 클라이언트로부터 읽기 처리
void Server::handleClientRead(int client_fd, const ServerConfig &server_config) {
    if (client_fd < 0) {
        std::cerr << "Invalid file descriptor for client_fd " << client_fd << std::endl;
        return;
    }
    
    char buffer[4096];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        std::string request_str(buffer);

        // 요청 파싱
        Request request;
        if (!request.parse(request_str)) {
            // 잘못된 요청: 400 Bad Request 응답
            Response res;
            res.setStatus("400 Bad Request");
            res.setBody("<h1>400 Bad Request</h1>");
            res.setHeader("Content-Length", "17");
            res.setHeader("Content-Type", "text/html");
            sendResponse(client_fd, res);
            safelyCloseClient(client_fd);
            return;
        }

        // 요청에 맞는 위치 블록 찾기
        const LocationConfig *matched_location = NULL;
        for (size_t loc = 0; loc < server_config.locations.size(); ++loc) {
            if (request.getPath().find(server_config.locations[loc].path) == 0) {
                matched_location = &server_config.locations[loc];
                break;
            }
        }

        // 위치 설정에 따라 응답 생성
        Response response;
        if (matched_location != NULL) {
            // 해당 위치 설정에 따른 처리
            response = Response::createResponse(request, *matched_location);
        }
        else {
            // 기본 위치 설정에 따른 처리 (필요 시 기본 위치 설정 추가)
            LocationConfig default_location;
            // 기본 설정을 명시적으로 설정
            default_location.path = "/";
            default_location.methods.push_back("GET");
            default_location.root = server_config.root; // 서버 루트 사용
            default_location.directory_listing = false;
            default_location.index = "./www/html/index.html";
            response = Response::createResponse(request, default_location);
        }

        // 응답 전송
        sendResponse(client_fd, response);
        
        // 응답 전송 후 Poller에서 제거하고 소켓 닫기 (단순화된 처리)
        safelyCloseClient(client_fd);
    }
    else if (bytes_read == 0)
        safelyCloseClient(client_fd);
    else {
        // recv 에러
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 데이터가 아직 도착하지 않은 경우, 처리를 무시
            return;
        }
        else if (errno == ECONNRESET) {
            // Connection reset by peer - 조용히 연결 종료
            safelyCloseClient(client_fd);
            return;
        }
        logError("recv() failed for client_fd " + intToString(client_fd) + ": " + strerror(errno));
        safelyCloseClient(client_fd);
    }
}

// 클라이언트에게 응답 쓰기 처리 (현재 단순화됨)
void Server::handleClientWrite(int client_fd) {
    (void)client_fd; // 미사용 파라미터 무시
    // 현재 응답은 handleClientRead에서 바로 전송되므로, 이 함수는 필요 없음
    // 향후 비동기 응답 전송을 구현할 때 사용
}

// 응답 전송 함수
void Server::sendResponse(int client_fd, const Response &response) {
    std::string response_str = response.toString();
    ssize_t total_sent = 0;
    ssize_t to_send = response_str.size();

    while (total_sent < to_send) {
        ssize_t sent = send(client_fd, response_str.c_str() + total_sent, to_send - total_sent, 0);
        if (sent == -1) {
            // C++98 방식으로 숫자 변환
            std::cerr << "send() failed for client_fd " << client_fd << ": " << strerror(errno) << std::endl;
            logError("send() failed for client_fd " + intToString(client_fd) + ": " + strerror(errno));
            break;
        }
        total_sent += sent;
    }
}

// C++98 호환을 위한 int to string 변환 함수 정의
std::string Server::intToString(int number) const {
    std::stringstream ss;
    ss << number;
    return ss.str();
}

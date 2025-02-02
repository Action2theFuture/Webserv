#include "Server.hpp"

// Server 클래스 생성자
Server::Server(const std::string &configFile) : poller(NULL)
{
    std::cerr << "[DEBUG] Server ctor configFile: " << configFile << std::endl;

    Configuration config;
    if (!config.parseConfigFile(configFile))
    {
        LogConfig::logError("Failed to parse configuration file.");
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
Server::~Server()
{
    for (size_t i = 0; i < server_configs.size(); ++i)
    {
        for (size_t j = 0; j < server_configs[i].server_sockets.size(); ++j)
        {
            close(server_configs[i].server_sockets[j]);
        }
    }
    delete poller;
}

// 소켓 초기화
void Server::initSockets()
{
    for (size_t i = 0; i < server_configs.size(); ++i)
    {
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
void Server::start()
{
    while (true)
    {
        std::vector<Event> events;
        if (!processPollerEvents(events))
        {
            LogConfig::logError("Failed to poll events.");
            continue;
        }
        processEvents(events);
    }
}

// Poller 이벤트 처리
bool Server::processPollerEvents(std::vector<Event> &events)
{
    int n = poller->poll(events, -1);
    if (n == -1)
    {
        LogConfig::logError("poller->poll() failed: " + std::string(strerror(errno)));
        return false;
    }
    return true;
}

// 이벤트 목록 처리
void Server::processEvents(const std::vector<Event> &events)
{
    for (size_t i = 0; i < events.size(); ++i)
    {
        int fd = events[i].fd;

        if (fd < 0)
        {
            LogConfig::logError("Invalid file descriptor in events[" + intToString(i) + "]");
            continue;
        }

        if (events[i].events & POLLER_READ)
        {
            ServerConfig *matched_server = NULL;
            if (isServerSocket(fd, &matched_server))
            {
                handleNewConnection(fd);
            }
            else
            {
                handleClientRead(fd, findMatchingServerConfig(fd));
            }
        }

        if (events[i].events & POLLER_WRITE)
        {
            handleClientWrite(fd);
        }
    }
}

// 새 연결 처리
void Server::handleNewConnection(int server_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1)
    {
        LogConfig::logError("accept() failed: " + std::string(strerror(errno)));
        return;
    }

    if (!setNonBlocking(client_fd))
    {
        LogConfig::logError("Failed to set non-blocking mode for client_fd " + intToString(client_fd));
        close(client_fd);
        return;
    }

    if (!poller->add(client_fd, POLLER_READ))
    {
        LogConfig::logError("Failed to add client_fd " + intToString(client_fd) + " to poller");
        close(client_fd);
    }
}

// 클라이언트 요청 처리
void Server::handleClientRead(int client_fd, const ServerConfig &server_config)
{
    // 클라이언트별 누적 버퍼
    std::string &buffer = partialRequests[client_fd];

    // 1) 논블로킹 recv 반복
    if (!readClientData(client_fd, buffer))
    {
        // 에러 또는 클라이언트 종료 -> close
        safelyCloseClient(client_fd);
        partialRequests.erase(client_fd);
        return;
    }

    // 2) 누적 데이터 처리
    if (!handleReceivedData(client_fd, server_config, buffer))
    {
        safelyCloseClient(client_fd);
        partialRequests.erase(client_fd);
        return;
    }
}
// (2) readClientData: 논블로킹 recv 반복, EAGAIN 나오면 종료
bool Server::readClientData(int client_fd, std::string &buffer)
{
    while (true)
    {
        char tmp[BUFFER_SIZE];
        ssize_t bytes_read = recv(client_fd, tmp, sizeof(tmp), 0);

        if (bytes_read > 0)
        {
            buffer.append(tmp, bytes_read);
        }
        else if (bytes_read == 0)
        {
            // 클라이언트 연결 종료
            return false;
        }
        else
        {
            // bytes_read < 0 -> EAGAIN or error
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 지금은 더 이상 읽을 데이터 없음
                break;
            }
            else if (errno == ECONNRESET)
            {
                // "Connection reset by peer"
                return false;
            }
            else
            {
                // 그 외 에러
                std::cerr << "recv() failed on fd " << client_fd << ": " << strerror(errno) << std::endl;
                return false;
            }
        }
    }
    return true;
}

// (3) handleReceivedData: 누적된 buffer를 RequestHandler로 넘겨 처리
bool Server::handleReceivedData(int client_fd, const ServerConfig &server_config, std::string &buffer)
{
    // 간단히 한 번에 RequestHandler에게 넘김
    // 실제론 HTTP 파이프라이닝 고려 시 여러 요청분 파싱 가능
    // 여러 요청을 파이프라인/부분 수신 상태로 처리 가능
    // 하나의 요청을 파싱 시도
    //  - return >= 1 : 해당 요청을 소비한 바이트 수
    //  - return == 0 : 요청이 불완전(더 받아야 함) or 중단
    //  - return < 0  : 치명적 에러
    while (true)
    {
        int consumed = 0;
        if (!processClientRequest(client_fd, server_config, buffer, consumed))
        {
            // false -> 치명적 에러, 소켓 종료
            return false;
        }
        else if (consumed == 0)
        {
            // 불완전(또는 더 이상 처리할 요청 없음) -> 중단
            // 소켓은 유지, 다음 poll 이벤트에서 recv로 추가 데이터 대기
            break;
        }
        else
        {
            // consumed > 0 : 한 요청을 제대로 처리함
            // 해당 구간만큼 버퍼에서 지움
            buffer.erase(0, consumed);

            // buffer에 남은 데이터가 있다면
            // 또 하나의 요청이 들어 있을 수 있으니 계속 루프
            if (buffer.empty())
                break;
        }
    }
    return true; // 치명적 에러 없이 종료
}

// 클라이언트 쓰기 처리
void Server::handleClientWrite(int client_fd)
{
    // 현재 단순화된 처리 (확장 가능)
    (void)client_fd;
}

// 비차단 모드 설정
bool Server::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        return false;
    return true;
}

// 파일 디스크립터가 서버 소켓인지 확인
bool Server::isServerSocket(int fd, ServerConfig **matched_server) const
{
    for (size_t s = 0; s < server_configs.size(); ++s)
    {
        for (size_t sock = 0; sock < server_configs[s].server_sockets.size(); ++sock)
        {
            if (fd == server_configs[s].server_sockets[sock])
            {
                if (matched_server)
                {
                    *matched_server = const_cast<ServerConfig *>(&server_configs[s]);
                }
                return true;
            }
        }
    }
    return false;
}

// 파일 디스크립터에 해당하는 서버 구성 찾기
ServerConfig &Server::findMatchingServerConfig(int fd)
{
    for (size_t s = 0; s < server_configs.size(); ++s)
    {
        for (size_t sock = 0; sock < server_configs[s].server_sockets.size(); ++sock)
        {
            if (fd == server_configs[s].server_sockets[sock])
            {
                return server_configs[s];
            }
        }
    }
    return server_configs[0];
}

// 안전한 클라이언트 소켓 닫기
void Server::safelyCloseClient(int client_fd)
{
    static std::set<int> closed_fds;

    if (closed_fds.find(client_fd) != closed_fds.end())
    {
        return; // 이미 닫힌 fd
    }

    poller->remove(client_fd);
    close(client_fd);
    closed_fds.insert(client_fd);
}

const LocationConfig *matchLocationConfig(const Request &request, const ServerConfig &server_config)
{
    std::cout << "===========================================" << std::endl;

    // root request
    if (request.getPath() == "/" || request.getPath().empty())
    {
        std::cout << "Debug: Root request detected" << std::endl;
        // location config for "/"
        for (size_t loc = 0; loc < server_config.locations.size(); ++loc)
        {
            if (server_config.locations[loc].path == "/")
            {
                std::cout << "Debug: Matched root location" << std::endl;
                return &server_config.locations[loc];
            }
        }
        std::cout << "Debug: No root location found" << std::endl;
        return NULL;
    }

    const LocationConfig *matched_location = NULL;
    size_t longest_match = 0;

    /// 
    std::string req_directory = request.getPath().substr(0, request.getPath().find_last_of('/'));

    std::string req_path_lower = toLower(server_config.root + req_directory);
    std::cout << "Debug: Final requested path: " << req_path_lower << std::endl;

    ///


    for (size_t loc = 0; loc < server_config.locations.size(); ++loc)
    {
        std::string loc_path_lower = toLower(server_config.root + server_config.locations[loc].path); //
        const std::string &req_path = req_path_lower;
        std::cout << "Matching request path: " << req_path << " with location path: " << loc_path_lower << std::endl;

        if (req_path.compare(0, loc_path_lower.length(), loc_path_lower) == 0)
        {
            // 추가 조건: 정확한 매칭 또는 다음 문자가 '/'
            if (req_path.length() == loc_path_lower.length() || req_path[loc_path_lower.length()] == '/')
            {
                // 가장 긴 매칭 경로를 선택하여 우선순위 처리
                if (loc_path_lower.length() > longest_match)
                {
                    matched_location = &server_config.locations[loc];
                    longest_match = loc_path_lower.length();
                    std::cout << "New matched location: " << loc_path_lower << std::endl;
                }
            }
        }
    }
    if (matched_location)
        std::cout << "Final matched location: " << matched_location->path << std::endl;
    else
        std::cout << "No matching location found for path: " << request.getPath() << std::endl;

    return matched_location;
}


/* const LocationConfig *matchLocationConfig(const Request &request, const ServerConfig &server_config)
{
    std::cout << "===========================================" << std::endl;

    // root request
    if (request.getPath() == "/" || request.getPath().empty())
    {
        std::cout << "Debug: Root request detected" << std::endl;
        // location config for "/"
        for (size_t loc = 0; loc < server_config.locations.size(); ++loc)
        {
            if (server_config.locations[loc].path == "/")
            {
                std::cout << "Debug: Matched root location" << std::endl;
                return &server_config.locations[loc];
            }
        }
        std::cout << "Debug: No root location found" << std::endl;
        return NULL;
    }

    const LocationConfig *matched_location = NULL;
    size_t longest_match = 0;

    /// 
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        std::cerr << "Debug: Failed to get current working directory" << std::endl;
        return NULL;
    }

    std::string req_directory = request.getPath().substr(0, request.getPath().find_last_of('/'));

    std::string req_path_lower = toLower(std::string(cwd) + req_directory);
    std::cout << "Debug: Final requested path: " << req_path_lower << std::endl;

    ///


    for (size_t loc = 0; loc < server_config.locations.size(); ++loc)
    {
        std::string loc_path_lower = toLower(std::string(cwd) + server_config.locations[loc].path); //
        const std::string &req_path = req_path_lower;
        std::cout << "Matching request path: " << req_path << " with location path: " << loc_path_lower << std::endl;

        if (req_path.compare(0, loc_path_lower.length(), loc_path_lower) == 0)
        {
            // 추가 조건: 정확한 매칭 또는 다음 문자가 '/'
            if (req_path.length() == loc_path_lower.length() || req_path[loc_path_lower.length()] == '/')
            {
                // 가장 긴 매칭 경로를 선택하여 우선순위 처리
                if (loc_path_lower.length() > longest_match)
                {
                    matched_location = &server_config.locations[loc];
                    longest_match = loc_path_lower.length();
                    std::cout << "New matched location: " << loc_path_lower << std::endl;
                }
            }
        }
    }
    if (matched_location)
        std::cout << "Final matched location: " << matched_location->path << std::endl;
    else
        std::cout << "No matching location found for path: " << request.getPath() << std::endl;

    return matched_location;
} */

bool Server::processClientRequest(int client_fd, const ServerConfig &server_config, const std::string &request_str,
                                  int &consumed)
{
    using namespace ResponseHandlers;

    consumed = 0;
    bool isPartial = false;
    Request request;
    if (!request.parse(request_str, consumed, isPartial))
    {
        sendBadRequestResponse(client_fd, server_config);
        return false;
    }

    if (isPartial)
    {
        // 아직 불완전 -> consumed=0
        consumed = 0;
        return true; // 소켓 유지
    }

    const LocationConfig *matched_location = matchLocationConfig(request, server_config);
    if (matched_location == NULL)
    {
        std::cerr << "No matching location found for path: " << request.getPath() << std::endl;
        Response res = Response::createErrorResponse(404, server_config);
        sendResponse(client_fd, res);
        return false;
    }
    // std::cout << "mached location path : " << matched_location->path << std::endl;
    Response response = Response::buildResponse(request, server_config, matched_location);
    sendResponse(client_fd, response);

    consumed = request_str.size();
    return true;
}

void Server::sendResponse(int client_fd, const Response &response)
{
    std::string response_str = response.toString();
    ssize_t total_sent = 0;
    ssize_t to_send = response_str.size();

    while (total_sent < to_send)
    {
        ssize_t sent = send(client_fd, response_str.c_str() + total_sent, to_send - total_sent, 0);
        if (sent == -1)
        {
            // C++98 방식으로 숫자 변환
            std::cerr << "send() failed for client_fd " << client_fd << ": " << strerror(errno) << std::endl;
            LogConfig::logError("send() failed for client_fd " + intToString(client_fd) + ": " + strerror(errno));
            break;
        }
        total_sent += sent;
    }
}

void Server::sendBadRequestResponse(int client_fd, const ServerConfig &server_config)
{
    Response res;
    res.setStatus(BAD_REQUEST_404);
    // 에러 페이지가 설정되어 있는지 확인
    std::string error_body = "<h1>400 Bad Request</h1>"; // 기본 에러 메시지
    if (server_config.error_pages.find(400) != server_config.error_pages.end())
    {
        error_body = server_config.error_pages.at(400);
    }
    res.setBody(error_body);
    res.setHeader("Content-Length", intToString(error_body.length()));
    res.setHeader("Content-Type", "text/html");
    sendResponse(client_fd, res);
}
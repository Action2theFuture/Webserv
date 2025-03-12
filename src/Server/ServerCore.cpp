#include "EpollPoller.hpp"
#include "KqueuePoller.hpp"
#include "Server.hpp"
#include <cstring>
#include <errno.h>
#include <stdlib.h>

extern volatile sig_atomic_t shutdown_flag;

static void show_ascii()
{
    std::ifstream file(ASCII_ART_PATH);
    if (!file)
    {
        std::cerr << "Error: Could not open file " << ASCII_ART_PATH << std::endl;
        return;
    }

    std::cout << "\033[32m";
    std::string line;
    while (std::getline(file, line))
    {
        std::cout << line << std::endl;
    }
    std::cout << "\033[0m";
}

Server::Server(const std::string &configFile) : _poller(NULL)
{
    Configuration config;
    if (!config.parseConfigFile(configFile))
    {
        LogConfig::reportInternalError("Failed to parse configuration file.");
        throw std::runtime_error("Failed to parse configuration file.");
    }
    show_ascii();
    _server_configs = config.servers;
#ifdef __linux__
    _poller = new EpollPoller();
#elif defined(__APPLE__)
    _poller = new KqueuePoller();
#else
#error "Unsupported OS"
#endif
    initSockets();
}

Server::~Server()
{
    for (size_t i = 0; i < _server_configs.size(); ++i)
    {
        for (size_t j = 0; j < _server_configs[i].server_sockets.size(); ++j)
        {
            close(_server_configs[i].server_sockets[j]);
        }
    }
    delete _poller;
    std::map<int, std::string>().swap(_partialRequests);
    std::map<int, std::string>().swap(_outgoingData);
    std::map<int, Request>().swap(_requestMap);
}

void Server::initSockets()
{
    for (size_t i = 0; i < _server_configs.size(); ++i)
    {
        ServerConfig &server = _server_configs[i];
        int sockfd = SocketManager::createSocket(server.port);
        SocketManager::setSocketNonBlocking(sockfd, server.port);
        SocketManager::bindSocket(sockfd, server.port);
        SocketManager::startListening(sockfd, server.port);
        server.server_sockets.push_back(sockfd);
        _poller->add(sockfd, POLLER_READ);
    }
}

void Server::start()
{
    _is_running = true;
    while (_is_running)
    {
        if (shutdown_flag)
        {
            _is_running = false;
            break;
        }

        std::vector<Event> events;
        if (!processPollerEvents(events))
        {
            LogConfig::reportInternalError("Failed to poll events.");
            continue;
        }
        processEvents(events);
    }
}

void Server::stop()
{
    _is_running = false;
}

bool Server::processPollerEvents(std::vector<Event> &events)
{
    int n = _poller->poll(events, 1000);
    if (n == -1)
    {
        LogConfig::reportInternalError("poller->poll() failed: " + std::string(strerror(errno)));
        return false;
    }
    return true;
}

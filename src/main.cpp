#include "Server.hpp"

Server *g_server = NULL;

void signalHandler(int signum)
{
    std::cerr << "\nReceived signal " << signum << ", shutting down server..." << std::endl;
    if (g_server)
    {
        // graceful shutdown 호출: 서버의 stop() 메소드를 호출해서 메인 루프 종료
        g_server->stop();
        // 이후 cleanup은 Server의 소멸자에서 수행됩니다.
        delete g_server;
        g_server = NULL;
    }
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    // 인자 개수 확인
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return EXIT_FAILURE;
    }

    // configFile을 argv[1]으로 설정
    std::string configFile = argv[1];

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try
    {
        std::cerr << "Starting server with config file: " << configFile << std::endl;
        g_server = new Server(configFile);
        g_server->start(); // 블로킹 호출
        // server.start()가 블로킹 호출이므로 아래 메시지는 보이지 않을 수 있다
        std::cerr << "Server stopped." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

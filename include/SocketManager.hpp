#ifndef SOCKETMANAGER_HPP
#define SOCKETMANAGER_HPP

#include <string>

class SocketManager
{
  public:
    static int createSocket(int port);
    static void setSocketOptions(int sockfd, int port);
    static void setSocketNonBlocking(int sockfd, int port);
    static void bindSocket(int sockfd, int port);
    static void startListening(int sockfd, int port);
};

#endif // SOCKETMANAGER_HPP

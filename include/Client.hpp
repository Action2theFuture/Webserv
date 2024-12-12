#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <sys/socket.h>
#include "Request.hpp"
#include "Response.hpp"

class Client {
public:
    Client(int fd);
    ~Client();
    
    int getFd() const;
    void handleRead();
    void handleWrite();
    bool isResponseReady() const;

private:
    int fd;
    std::string read_buffer;
    std::string write_buffer;
    Request request;
    Response response;
    bool response_ready;
};

#endif

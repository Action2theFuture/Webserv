#include "Client.hpp"
#include <unistd.h>
#include <iostream>

Client::Client(int fd) : fd(fd), response_ready(false) {}

Client::~Client() {
    close(fd);
}

int Client::getFd() const {
    return fd;
}

void Client::handleRead() {
    char buffer[4096];
    ssize_t bytes_read = recv(fd, buffer, sizeof(buffer), 0);
    if (bytes_read <= 0) {
        // 에러 처리 또는 연결 종료
        close(fd);
        return;
    }
    read_buffer.append(buffer, bytes_read);

    // 요청 파싱
    if (request.parse(read_buffer)) {
        // 요청 처리
        response = Response::createResponse(request);
        write_buffer = response.toString();
        response_ready = true;
    }
}

void Client::handleWrite() {
    if (write_buffer.empty())
        return;
    ssize_t bytes_sent = send(fd, write_buffer.c_str(), write_buffer.size(), 0);
    if (bytes_sent < 0) {
        perror("send");
        close(fd);
        return;
    }
    write_buffer.erase(0, bytes_sent);
}

bool Client::isResponseReady() const {
    return response_ready;
}

#ifndef POLLER_HPP
#define POLLER_HPP

#include <vector>
#include <cstdint>

// Poller 추상화 클래스에서 사용할 자체 이벤트 플래그 정의
const uint32_t POLLER_READ = 1 << 0;
const uint32_t POLLER_WRITE = 1 << 1;

struct Event {
    int fd;
    uint32_t events;
};

class Poller {
public:
    virtual ~Poller() {}
    virtual bool add(int fd, uint32_t events) = 0;
    virtual bool modify(int fd, uint32_t events) = 0;
    virtual bool remove(int fd) = 0;
    virtual int poll(std::vector<Event> &events, int timeout = -1) = 0;
};

#endif

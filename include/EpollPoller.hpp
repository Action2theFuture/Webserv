#ifndef EPOLLPOLLER_HPP
#define EPOLLPOLLER_HPP

#ifdef __linux__

#include "Poller.hpp"
#include <sys/epoll.h>


class EpollPoller : public Poller {
public:
    EpollPoller();
    ~EpollPoller();
    
    void add(int fd, uint32_t events);
    void modify(int fd, uint32_t events);
    void remove(int fd);
    int poll(std::vector<Event> &events, int timeout = -1);

private:
    int epoll_fd;
    std::vector<struct epoll_event> epoll_events;
};

#endif

#endif

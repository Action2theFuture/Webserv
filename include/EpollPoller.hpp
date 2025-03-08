#ifndef EPOLLPOLLER_HPP
#define EPOLLPOLLER_HPP

#ifdef __linux__

#include "Define.hpp"
#include "Poller.hpp"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>

class EpollPoller : public Poller
{
  public:
    EpollPoller();
    ~EpollPoller();

    bool add(int fd, uint32_t events);
    bool modify(int fd, uint32_t events);
    bool remove(int fd);
    int poll(std::vector<Event> &events, int timeout = -1);

  private:
    int _epoll_fd;
    // _changes will act like the temporary changes array similar to KqueuePoller's 'changes'
    std::vector<struct epoll_event> _changes;
};

#endif

#endif

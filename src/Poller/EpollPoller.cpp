#ifdef __linux__

#include "EpollPoller.hpp"
#include "Log.hpp"
#include "Utils.hpp" // for intToString
#include <sys/epoll.h>

EpollPoller::EpollPoller() : _changes()
{
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1)
    {
        std::cerr << "epoll_create1 failed: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    // Reserve space for change events similar to Kqueue's fixed array size.
    _changes.resize(MAX_EVENTS);
}

EpollPoller::~EpollPoller()
{
    close(_epoll_fd);
}

bool EpollPoller::add(int fd, uint32_t events)
{
    struct epoll_event ev;
    ev.events = 0;
    if (events & POLLER_READ)
        ev.events |= EPOLLIN;
    if (events & POLLER_WRITE)
        ev.events |= EPOLLOUT;
    ev.data.fd = fd;

    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        std::cerr << "epoll_ctl add failed for fd " << intToString(fd) << ": " 
                  << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool EpollPoller::modify(int fd, uint32_t events)
{
    struct epoll_event ev;
    ev.events = 0;
    if (events & POLLER_READ)
        ev.events |= EPOLLIN;
    if (events & POLLER_WRITE)
        ev.events |= EPOLLOUT;
    ev.data.fd = fd;

    if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1)
    {
        std::cerr << "epoll_ctl modify failed for fd " << intToString(fd) << ": " 
                  << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool EpollPoller::remove(int fd)
{
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
        std::cerr << "epoll_ctl remove failed for fd " << intToString(fd) << ": " 
                  << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

int EpollPoller::poll(std::vector<Event> &events_out, int timeout)
{
    // Create a temporary array for epoll_wait
    struct epoll_event events[MAX_EVENTS];
    int nevents = epoll_wait(_epoll_fd, events, MAX_EVENTS, timeout);
    if (nevents == -1)
    {
        std::cerr << "epoll_wait failed: " << strerror(errno) << std::endl;
        return -1;
    }
    events_out.clear();
    for (int i = 0; i < nevents; ++i)
    {
        if (events[i].events & EPOLLERR)
        {
            int err = 0;
            socklen_t len = sizeof(err);
            if (getsockopt(events[i].data.fd, SOL_SOCKET, SO_ERROR, &err, &len) == 0) {
                std::cerr << "epoll event error on fd " << intToString(events[i].data.fd)
                      << ": " << strerror(err) << std::endl;
            } else {
                std::cerr << "epoll event error on fd " << intToString(events[i].data.fd)
                      << ": unable to get error" << std::endl;
            }   
            // 연결이 리셋되었으므로 해당 fd를 epoll에서 제거합니다.
            remove(events[i].data.fd);
            continue;
        }
        Event ev;
        ev.fd = events[i].data.fd;
        ev.events = 0;
        if (events[i].events & EPOLLIN)
            ev.events |= POLLER_READ;
        if (events[i].events & EPOLLOUT)
            ev.events |= POLLER_WRITE;
        events_out.push_back(ev);
    }
    return nevents;
}

#endif

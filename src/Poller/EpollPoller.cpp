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
        LogConfig::reportInternalError("epoll_create1 failed: " + std::string(strerror(errno)));
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
        LogConfig::reportInternalError("epoll_ctl add failed for fd " + intToString(fd) + ": " +
                                       std::string(strerror(errno)));
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
        LogConfig::reportInternalError("epoll_ctl modify failed for fd " + intToString(fd) + ": " +
                                       std::string(strerror(errno)));
        return false;
    }
    return true;
}

bool EpollPoller::remove(int fd)
{
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
        LogConfig::reportInternalError("epoll_ctl remove failed for fd " + intToString(fd) + ": " +
                                       std::string(strerror(errno)));
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
        LogConfig::reportInternalError("epoll_wait failed: " + std::string(strerror(errno)));
        return -1;
    }
    events_out.clear();
    for (int i = 0; i < nevents; ++i)
    {
        if (events[i].events & EPOLLERR)
        {
            LogConfig::reportInternalError("epoll event error on fd " + intToString(events[i].data.fd) + ": " +
                                           std::string(strerror(errno)));
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

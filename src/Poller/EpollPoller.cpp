#ifdef __linux__

#include "EpollPoller.hpp"
#include "Log.hpp"
#include "Utils.hpp" // for intToString
#include <sys/epoll.h>

EpollPoller::EpollPoller()
{
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1)
    {
        std::string errMsg = "epoll_create1 failed: " + std::string(strerror(errno));
        throw std::runtime_error(errMsg);
    }
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
        std::string errMsg = "epoll_ctl add failed for fd " + intToString(fd) + ": " + std::string(strerror(errno));
        LogConfig::reportInternalError(errMsg);
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
        std::string errMsg = "epoll_ctl modify failed for fd " + intToString(fd) + ": " + std::string(strerror(errno));
        LogConfig::reportInternalError(errMsg);
        return false;
    }
    return true;
}

bool EpollPoller::remove(int fd)
{
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
        std::string errMsg = "epoll_ctl remove failed for fd " + intToString(fd) + ": " + std::string(strerror(errno));
        LogConfig::reportInternalError(errMsg);
        return false;
    }
    return true;
}

int EpollPoller::poll(std::vector<Event> &events_out, int timeout)
{
    int nevents = epoll_wait(_epoll_fd, _events, MAX_EVENTS, timeout);
    if (nevents == -1)
    {
        if (errno == EINTR)
            return 0;
        std::string errMsg = "epoll_wait failed: " + std::string(strerror(errno));
        LogConfig::reportInternalError(errMsg);
        return -1;
    }
    events_out.clear();
    for (int i = 0; i < nevents; ++i)
    {
        if (_events[i].events & EPOLLERR)
        {
            int err = 0;
            socklen_t len = sizeof(err);
            if (getsockopt(_events[i].data.fd, SOL_SOCKET, SO_ERROR, &err, &len) == 0)
            {
                std::string errMsg =
                    "epoll event error on fd " + intToString(_events[i].data.fd) + ": " + std::string(strerror(err));
                LogConfig::reportInternalError(errMsg);
            }
            else
            {
                std::string errMsg =
                    "epoll event error on fd " + intToString(_events[i].data.fd) + ": unable to get error";
                LogConfig::reportInternalError(errMsg);
            }
            remove(_events[i].data.fd);
            continue;
        }
        Event ev;
        ev.fd = _events[i].data.fd;
        ev.events = 0;
        if (_events[i].events & EPOLLIN)
            ev.events |= POLLER_READ;
        if (_events[i].events & EPOLLOUT)
            ev.events |= POLLER_WRITE;
        events_out.push_back(ev);
    }
    return nevents;
}

#endif

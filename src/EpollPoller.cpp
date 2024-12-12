#ifdef __linux__

#include "EpollPoller.hpp"
#include <cstring>
#include <iostream>
#include <cstdlib>

EpollPoller::EpollPoller() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }
}

EpollPoller::~EpollPoller() {
    close(epoll_fd);
}

void EpollPoller::add(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = 0;
    if (events & POLLER_READ)
        ev.events |= EPOLLIN;
    if (events & POLLER_WRITE)
        ev.events |= EPOLLOUT;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl: add");
        exit(EXIT_FAILURE);
    }
}

void EpollPoller::modify(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = 0;
    if (events & POLLER_READ)
        ev.events |= EPOLLIN;
    if (events & POLLER_WRITE)
        ev.events |= EPOLLOUT;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
        perror("epoll_ctl: mod");
        exit(EXIT_FAILURE);
    }
}

void EpollPoller::remove(int fd) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        perror("epoll_ctl: del");
        exit(EXIT_FAILURE);
    }
}

int EpollPoller::poll(std::vector<Event> &events_out, int timeout) {
    struct epoll_event events[MAX_EVENTS];
    int n = epoll_wait(epoll_fd, events, MAX_EVENTS, timeout);
    if (n == -1) {
        perror("epoll_wait");
        return -1;
    }

    events_out.clear();
    for (int i = 0; i < n; ++i) {
        Event ev;
        ev.fd = events[i].data.fd;
        ev.events = 0;
        if (events[i].events & EPOLLIN)
            ev.events |= POLLER_READ;
        if (events[i].events & EPOLLOUT)
            ev.events |= POLLER_WRITE;
        events_out.push_back(ev);
    }

    return n;
}

#endif

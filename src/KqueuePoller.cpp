#ifdef __APPLE__

#include "KqueuePoller.hpp"
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cstdlib>

KqueuePoller::KqueuePoller() : changelist_count(0) {
    kqueue_fd = kqueue();
    if (kqueue_fd == -1) {
        perror("kqueue");
        exit(EXIT_FAILURE);
    }
}

KqueuePoller::~KqueuePoller() {
    close(kqueue_fd);
}

void KqueuePoller::add(int fd, uint32_t events) {
    if (events & POLLER_READ) {
        EV_SET(&changes[changelist_count++], fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    }
    if (events & POLLER_WRITE) {
        EV_SET(&changes[changelist_count++], fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
    }
}

void KqueuePoller::modify(int fd, uint32_t events) {
    // 먼저 기존 필터를 삭제
    EV_SET(&changes[changelist_count++], fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    EV_SET(&changes[changelist_count++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
    
    // 새로운 필터 추가
    if (events & POLLER_READ) {
        EV_SET(&changes[changelist_count++], fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    }
    if (events & POLLER_WRITE) {
        EV_SET(&changes[changelist_count++], fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
    }
}

void KqueuePoller::remove(int fd) {
    EV_SET(&changes[changelist_count++], fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    EV_SET(&changes[changelist_count++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
}

int KqueuePoller::poll(std::vector<Event> &events_out, int timeout) {
    struct timespec ts;
    struct timespec *pts = NULL; // NULL이면 무한 대기

    if (timeout >= 0) {
        ts.tv_sec = timeout / 1000;                   // 초 단위
        ts.tv_nsec = (timeout % 1000) * 1000000;      // 나노초 단위
        pts = &ts;
    }
    // else pts remains NULL for infinite timeout

    int nevents = kevent(kqueue_fd, changes, changelist_count, events, MAX_EVENTS, pts);
    changelist_count = 0; // 변경 사항 초기화
    
    if (nevents == -1) {
        perror("kevent");
        return -1;
    }

    events_out.clear();
    for (int i = 0; i < nevents; ++i) {
        Event ev;
        ev.fd = events[i].ident;
        ev.events = 0;
        if (events[i].filter == EVFILT_READ)
            ev.events |= POLLER_READ;
        if (events[i].filter == EVFILT_WRITE)
            ev.events |= POLLER_WRITE;
        events_out.push_back(ev);
    }

    return nevents;
}

#endif

#ifdef __APPLE__

#include "KqueuePoller.hpp"

KqueuePoller::KqueuePoller() : changelist_count(0)
{
    kqueue_fd = kqueue();
    if (kqueue_fd == -1)
    {
        std::string errMsg = "kqueue failed: " + std::string(strerror(errno));
        throw std::runtime_error(errMsg);
    }
}

KqueuePoller::~KqueuePoller()
{
    close(kqueue_fd);
}

bool KqueuePoller::add(int fd, uint32_t events)
{
    if (events & POLLER_READ)
    {
        EV_SET(&changes[changelist_count++], fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    }
    if (events & POLLER_WRITE)
    {
        EV_SET(&changes[changelist_count++], fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
    }
    // kevent 호출로 변경 사항 적용
    if (kevent(kqueue_fd, changes, changelist_count, NULL, 0, NULL) == -1)
    {
        std::string errMsg = "kevent add failed: " + std::string(strerror(errno));
        LogConfig::reportInternalError(errMsg);
        changelist_count = 0; // 초기화
        return false;
    }

    changelist_count = 0; // 초기화
    return true;
}

bool KqueuePoller::modify(int fd, uint32_t events)
{
    // 먼저 기존 필터를 삭제
    EV_SET(&changes[changelist_count++], fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    EV_SET(&changes[changelist_count++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);

    if (kevent(kqueue_fd, changes, changelist_count, NULL, 0, NULL) == -1)
    {
        if (errno != ENOENT)
        {
            std::string errMsg = "kevent modify (delete) failed: " + std::string(strerror(errno));
            LogConfig::reportInternalError(errMsg);
            changelist_count = 0;
            return false;
        }
    }
    changelist_count = 0;
    // 새로운 이벤트 추가
    if (events & POLLER_READ)
    {
        EV_SET(&changes[changelist_count++], fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    }
    if (events & POLLER_WRITE)
    {
        EV_SET(&changes[changelist_count++], fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
    }
    // kevent 호출로 변경 사항 적용
    if (kevent(kqueue_fd, changes, changelist_count, NULL, 0, NULL) == -1)
    {
        std::string errMsg = "kevent modify failed: " + std::string(strerror(errno));
        LogConfig::reportInternalError(errMsg);
        changelist_count = 0; // 초기화
        return false;
    }

    changelist_count = 0; // 초기화
    return true;
}

bool KqueuePoller::remove(int fd)
{
    if (fd < 0)
        return false;
    EV_SET(&changes[changelist_count++], fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    EV_SET(&changes[changelist_count++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);

    // kevent 호출로 변경 사항 적용
    if (kevent(kqueue_fd, changes, changelist_count, NULL, 0, NULL) == -1)
    {
        if (errno != ENOENT)
        {
            std::string errMsg = "kevent remove failed: " + std::string(strerror(errno));
            LogConfig::reportInternalError(errMsg);
            changelist_count = 0; // 초기화
            return false;
        }
    }

    changelist_count = 0; // 초기화
    return true;
}

int KqueuePoller::poll(std::vector<Event> &events_out, int timeout)
{
    struct timespec ts;
    struct timespec *pts = NULL; // NULL이면 무한 대기

    if (timeout >= 0)
    {
        ts.tv_sec = timeout / 1000;              // 초 단위
        ts.tv_nsec = (timeout % 1000) * 1000000; // 나노초 단위
        pts = &ts;
    }
    // else pts remains NULL for infinite timeout

    int nevents = kevent(kqueue_fd, changes, changelist_count, events, MAX_EVENTS, pts);
    changelist_count = 0; // 변경 사항 초기화

    if (nevents == -1)
    {
        if (errno == EINTR)
        {
            // SIGINT 등으로 인해 kevent()가 중단된 경우, fatal error가 아니므로 0을 반환합니다.
            return 0;
        }
        std::string errMsg = "kevent poll failed: " + std::string(strerror(errno));
        LogConfig::reportInternalError(errMsg);
        return -1;
    }

    // 이벤트 변환 및 반환
    events_out.clear();
    for (int i = 0; i < nevents; ++i)
    {
        if (events[i].flags & EV_ERROR)
        {
            std::string errMsg =
                "Event error on fd " + intToString(events[i].ident) + ": " + std::string(strerror(events[i].data));
            LogConfig::reportInternalError(errMsg);
            continue;
        }

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

#ifndef KQUEUEPOLLER_HPP
#define KQUEUEPOLLER_HPP

#ifdef __APPLE__

#include "Poller.hpp"
#include <sys/event.h>
#include <sys/time.h>

class KqueuePoller : public Poller
{
  public:
    KqueuePoller();
    ~KqueuePoller();

    bool add(int fd, uint32_t events);
    bool modify(int fd, uint32_t events);
    bool remove(int fd);
    int poll(std::vector<Event> &events_out, int timeout = -1);

  private:
    int kqueue_fd;
    static const int MAX_EVENTS = 1000;
    struct kevent changes[MAX_EVENTS];
    struct kevent events[MAX_EVENTS];
    int changelist_count;
};

#endif

#endif

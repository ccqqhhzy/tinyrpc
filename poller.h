// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __POLLER_H__
#define __POLLER_H__

#include <stdint.h>
#include <functional>
#include <vector>
#include <mutex>
#include <queue>

namespace tinyrpc {

enum EPOLL_EVENT {
    EV_READ  = 0X0001,
    EV_WRITE = 0X0002,
    EV_TIMER = 0X0004,
    EV_EXCLUSIVE = 0X0008, // EPOLLEXCLUSIVE(since Linux 4.5)
};

using EventCallback = std::function<void (int, int, void*)>;

typedef struct EventItem {
    int fd;
    int events;
    EventCallback readCallback;
    EventCallback writeCallback;
    void* readArg;
    void* writeArg;

    EventItem() : fd(-1), events(0), readArg(nullptr), writeArg(nullptr) {}
} EventItem;

struct TimerEventItem {
    int64_t expiration; // ms
    int interval;       // ms
    bool repeat;
    EventCallback callback;
    void* arg;

    bool operator<(const TimerEventItem& other) const {
         // min-heap
        return expiration > other.expiration;
    }
};

class Poller {
public:
    enum {
        MAX_FD = 10240, // 10k
    };

    using EventItemList = std::vector<EventItem>;
    using FireEventList = std::vector<EventItem*>;
    using EpollResultList = std::vector<struct epoll_event>;

    Poller(uint32_t eventDataListSize);
    ~Poller();

    bool init();

    void setFdReadCallback(int fd, const EventCallback& cb, void* arg);
    void setFdWriteCallback(int fd, const EventCallback& cb, void* arg);
    bool addFd(int fd, int events);
    bool delFd(int fd);
    bool alterEvent(int fd, int events);
    bool addEvent(int fd, int events);
    bool delEvent(int fd, int events);
    bool addTimer(int intervalMs, bool repeat, const EventCallback& cb, void* arg);
    void runLoop();

    void stop() { isRunning_ = false; }
    void setTimeout(int timeout) { timeout_ = timeout; }
    
private:
    bool poll(int timeout, FireEventList& fireEventList);
    void handleFireEvent(const FireEventList& fireEventList);
    void checkTimers();
    int64_t getCurrentTimeMillis() const;

    int epollfd_;
    int timeout_;
    bool isRunning_;
    uint32_t activeEventNum_;

    uint32_t eventDataListSize_;
    EventItemList eventDataList_;
    
    EpollResultList epollResultList_;

    FireEventList fireEventList_;

    // timer: min-heap
    std::mutex timerMutex_;
    std::priority_queue<TimerEventItem> timerQueue_;
};


} // namespace tinyrpc
#endif // __POLLER_H__


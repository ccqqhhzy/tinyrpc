// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>

#include "poller.h"
#include "log.h"
#include "compiler.h"


using namespace tinyrpc;

Poller::Poller(uint32_t eventDataListSize) 
    : epollfd_(-1)
    , timeout_(10)
    , isRunning_(false)
    , activeEventNum_(0)
    , eventDataListSize_(eventDataListSize) {
    eventDataList_.resize(eventDataListSize_);
    epollResultList_.resize(eventDataListSize_);

    epollfd_ = ::epoll_create(eventDataListSize_);
    if (epollfd_ < 0) {
        LOG(Error, "epoll_create failed!");
        abort();
    }
}

Poller::~Poller() {
    close(epollfd_);
    epollfd_ = -1; 
}

void Poller::setFdReadCallback(int fd, const EventCallback& cb, void* arg) {
    assert(fd <= (int)eventDataListSize_);
    eventDataList_[fd].readCallback = cb;
    eventDataList_[fd].readArg = arg;
}

void Poller::setFdWriteCallback(int fd, const EventCallback& cb, void* arg) {
    assert(fd <= (int)eventDataListSize_);
    eventDataList_[fd].writeCallback = cb;
    eventDataList_[fd].writeArg = arg;
}

bool Poller::addFd(int fd, int events) {
    assert(fd <= (int)eventDataListSize_);

    struct epoll_event ee;
    memset(&ee, 0, sizeof(ee));
    ee.data.ptr = &(eventDataList_[fd]);

    if (events & EV_READ)
        ee.events |= EPOLLIN;
    if (events & EV_WRITE)
        ee.events |= EPOLLOUT;
    if (events & EV_EXCLUSIVE)
        ee.events |= EPOLLEXCLUSIVE;

    int ret = epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ee);
    if (ret < 0) {
        LOG(Error, "epoll_ctl failed, err = %s", strerror(errno));
        return false;
    }

    eventDataList_[fd].fd = fd;
    eventDataList_[fd].events |= events;
    ++activeEventNum_;

    return true;
}

bool Poller::delFd(int fd) {
    assert(fd <= (int)eventDataListSize_);

    struct epoll_event ee;
    ee.events = 0;
    ee.data.ptr = &(eventDataList_[fd]);

    int ret = epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, &ee);
    if (ret < 0) {
        LOG(Error, "epoll_ctl failed, err = %s", strerror(errno));
        return false;
    }

    eventDataList_[fd].events = 0;
    --activeEventNum_;

    return true;
}

bool Poller::alterEvent(int fd, int events) {
    assert(fd <= (int)eventDataListSize_);

    int newEvent = 0;
    if (events & EV_READ)
        newEvent |= EPOLLIN;
    if (events & EV_WRITE)
        newEvent |= EPOLLOUT;

    struct epoll_event ee;
    ee.data.ptr = &(eventDataList_[fd]);
    ee.events = newEvent;

    int ret = epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &ee);
    if (ret < 0) {
        LOG(Error, "alterEvent failed, err:%s", strerror(errno));
        return false;
    }

    eventDataList_[fd].events = newEvent;

    return true;
}

bool Poller::addEvent(int fd, int events) {
    assert(fd <= (int)eventDataListSize_);

    struct epoll_event ee;
    ee.data.ptr = &(eventDataList_[fd]);
    
    if (events & EV_READ)
        ee.events |= EPOLLIN;
    if (events & EV_WRITE)
        ee.events |= EPOLLOUT;

    int ret = epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &ee);
    if (ret < 0) {
        LOG(Error, "alterEvent failed, err:%s", strerror(errno));
        return false;
    }

    eventDataList_[fd].events |= events;

    return true;
}

bool Poller::delEvent(int fd, int events) {
    assert(fd <= (int)eventDataListSize_);

    struct epoll_event ee;
    ee.data.ptr = &(eventDataList_[fd]);
    
    if (events & EV_READ)
        ee.events &= ~EPOLLIN;
    if (events & EV_WRITE)
        ee.events &= ~EPOLLOUT;

    int ret = epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &ee);
    if (ret < 0) {
        LOG(Error, "alterEvent failed, err:%s", strerror(errno));
        return false;
    }

    eventDataList_[fd].events &= ~events;

    return true;
}

bool Poller::addTimer(int intervalMs, bool repeat,
                      const EventCallback& cb, void* arg)
{
    std::lock_guard<std::mutex> lock(timerMutex_);

    int64_t now = getCurrentTimeMillis();
    TimerEventItem item;
    item.expiration = now + intervalMs;
    item.interval = intervalMs;
    item.repeat = repeat;
    item.callback = cb;
    item.arg = arg;

    timerQueue_.push(item);

    return true;
}

int64_t Poller::getCurrentTimeMillis() const 
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void Poller::runLoop()
{
    int waitTime = 0;
    isRunning_ = true;
    while (isRunning_) {
        fireEventList_.clear();

        // default timeout
        waitTime = timeout_;
        // get the waitTime of the latest timer
        {
            std::lock_guard<std::mutex> lock(timerMutex_);
            if (!timerQueue_.empty()) {
                int64_t now = getCurrentTimeMillis();
                int64_t earliest = timerQueue_.top().expiration;
                waitTime = (earliest > now) ? (earliest - now) : 0;
                // note: if waitTime == 0, epoll_wait will return immediately 
                // even there is no event happen,
                // avoid idling
            }
        }

        poll(waitTime, fireEventList_);

        handleFireEvent(fireEventList_);

        checkTimers();
    }
}

bool Poller::poll(int timeout, FireEventList& fireEventList)
{
    const int retNum = ::epoll_wait(epollfd_, &*epollResultList_.begin(), 
                            epollResultList_.size(), timeout);

    if (retNum < 0) {
        if (errno != EINTR) {
            LOG(Error, "epoll_wait failed, err:%s", strerror(errno));
            return false;
        }
    }

    for (int i = 0; i < retNum; ++i) {
        EventItem *fireEvent = static_cast<EventItem*>(epollResultList_[i].data.ptr);

        if (epollResultList_[i].events & (EPOLLIN)) {
            fireEvent->events |= EV_READ;
        }
        if (epollResultList_[i].events & EPOLLOUT) {
            fireEvent->events |= EV_WRITE;
        }
        if (epollResultList_[i].events & (EPOLLERR | EPOLLHUP)) {
            fireEvent->events |= EV_READ | EV_WRITE;
        }

        fireEventList.push_back(fireEvent);
    }

    return true;
}

void Poller::handleFireEvent(const FireEventList& fireEventList)
{
    EventItem* ev = nullptr;
    for (size_t i = 0; i < fireEventList.size(); ++i) {
        ev = fireEventList[i];

        if (ev->events & EV_READ) {
            if (ev->readCallback) {
                ev->readCallback(ev->fd, ev->events, ev->readArg);
            }
        }

        if (ev->events & EV_WRITE) {
            if (ev->writeCallback) {
                ev->writeCallback(ev->fd, ev->events, ev->writeArg);
            }
        }
    }
}

void Poller::checkTimers()
{
    std::lock_guard<std::mutex> lock(timerMutex_);
    int64_t now = getCurrentTimeMillis();

    while (!timerQueue_.empty() && timerQueue_.top().expiration <= now) {
        TimerEventItem item = timerQueue_.top();
        timerQueue_.pop();

        if (item.callback) {
            // fd=-1 表示是 timer 事件
            item.callback(-1, EV_TIMER, item.arg);
        }

        if (item.repeat) {
            item.expiration = now + item.interval;
            timerQueue_.push(item);
        }
    }
}

    /*
     * listenfd 被多个子进程注册到 epoll，有新的连接时会引发惊群效应：
     *  所有子进程的 epoll_wait 都被唤醒。同时，所有子进程都触发 accept(listenfd)。
     * 解决方法：
     * 1.忽视惊群效应
     *   1.1 具体方法：
     *      accept时加互斥锁，获得锁的子进程accept新的连接，例如shm+semaphore。
     *   1.2 负载均衡
     *      由子进程控制，子进程可以根据自己的连接数来选择释放枪锁。
     *      例如，设置子进程最多10k连接，到了设定阈值就不接收新连接。
     *   1.3 为什么不用mutex，而是semaphore?
     *      因为子进程持有mutex锁后异常退出没有释放的话，会导致其它子进程没有机会获取mutex锁。
     *      semaphore有undo操作，持有semaphore(计数器-1)的子进程没有释放异常退出时信号量计数器恢复(+1)。
     *   1.4 弊端
     *      不必要的唤醒，上下文切换等，浪费cpu
     * 2.避免惊群效应
     *   2.1 具体方法：
     *      只让父进程(watcher)负责监听listenfd，父进程accept新连接的socket，
     *      然后，通过父子进程之间的 pipe fd 发送给子进程。
     *   2.2 负载均衡
     *      由父进程控制，子进程在共享内存更新连接数，父进程根据各个子进程连接数来选择把新连接交给谁。
     *   2.3 弊端
     *      瞬时新的链接蜂拥过来，单一进程accept可能会成为瓶颈
     * 3.避免惊群效应
     *   2.1 具体方法：
     *      子进程监听同一个ip端口(例如在workerRun调用listen)，socker在bind前设置SO_REUSEPORT
     *   2.2 负载均衡
     *      内核负责
     *   2.3 安全性
     *      是否其它进程可以监听同一个ip端口截取数据？同一个有效用户id的进程才能绑定。
     * 4.避免惊群效应
     *   4.1 具体方法：
     *      EPOLLEXCLUSIVE(since Linux 4.5)
     *      父进程listern，子进程对 listenfd 用 epoll_ctl + EPOLL_CTL_ADD 注册事件时，
     *      让 events |= EPOLLEXCLUSIVE
     *   4.2 负载均衡
     *      内核负责。内核会选择一个进程/线程去唤醒。
     * 5.避免惊群效应
     *   5.1 具体方法
     *      设置 listendfd 非阻塞，同时EPOLL_CTL_ADD 成 EPOLLIN|EPOLLET|EPOLLONESHOT，
     *      子进程/线程accept(listenfd)后，需要 EPOLL_CTL_MOD 重置 EPOLLIN|EPOLLET|EPOLLONESHOT。
     *   5.2 负载均衡
     *      没法控制。因为在[被唤醒->accept->重置]时间窗口，有多个链接到达，
     *      这些链接可能都会被当前唤醒的子进程/线程accept
     */

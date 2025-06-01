// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include "log.h"
#include "connection.h"
#include "compiler.h"
#include "server.h"
#include "util.h"
#include <cassert>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/prctl.h>


using namespace std;
using namespace tinyrpc;

// create pipe for signal event
// sig_pipefd[1] writen by sigHandler(SIGCHLD) etc
// sig_pipefd[0] read by parent/child to check whether should exit
static int sig_pipefd[2];

Server::Server(const std::vector<option>& vecOpt)
    : opt_(vecOpt)
    , workerList_(NULL)
    , workerIndex_(-1)
    , listenfd_(-1)
    , maxfd_(opt_.commonOption_.maxConnNum_)
    , poller_(NULL)
    , codec_(new Codec())
    , connPool_(opt_.commonOption_.maxConnNum_) {
    workerNum_ = opt_.commonOption_.workerNum_;
    assert(workerNum_ > 0 && workerNum_ <= PROCESS_MAXNUM);
    workerList_ = new Process[workerNum_];
    assert(workerList_);

    for (int i = 0; i < workerNum_; i++) {
        socketpair(AF_LOCAL, SOCK_STREAM, 0, workerList_[i].pipefd);
        
        workerList_[i].pid= fork();
        if (workerList_[i].pid < 0) {
            LOG(Error, "fork failed!");
        } else if (workerList_[i].pid > 0) { // parent
            // write/read sv[1] to IPC with child
            close(workerList_[i].pipefd[0]);
            continue;                           
        } else { // child
            workerIndex_ = i;
            // write/read sv[0] to IPC with parent
            close(workerList_[i].pipefd[1]); 
            break;
        }
    }
}

Server::~Server() {
    if (workerList_) {
        delete [] workerList_;
    }

    if (poller_) {
        delete poller_;
        poller_ = nullptr;
    }

    if (codec_) {
        delete codec_;
        codec_ = nullptr;
    }
}

auto Server::clearConnAndEraseFromConnMap(const std::shared_ptr<Connection>& conn) {
    if (!conn) {
        return connMap_.end();
    }

    int fd = conn->getFd();

    poller_->delFd(fd);
    close(fd);
    conn->reset();

    auto it = connMap_.find(fd);
    if (it != connMap_.end()) {
        // refcount of shared_ptr will -1 after erase()
        return connMap_.erase(it);
    }
    return connMap_.end();
}

bool Server::listenOnAddress(int family, const char* ip, uint16_t port, 
                             int& listenfd) {
    int fd = socket(family, SOCK_STREAM, 0);

    int optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    if (AF_INET == family) {
        struct sockaddr_in sa = {};
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        inet_pton(AF_INET, ip, &sa.sin_addr);

        if (::bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
            LOG(Error, "IPv4 bind failed. fd:%d,err:%s", fd, strerror(errno));
            return false;
        }
    } else if (family == AF_INET6) {
        struct sockaddr_in6 sa6 = {};
        sa6.sin6_family = AF_INET6;
        sa6.sin6_port = htons(port);
        inet_pton(AF_INET6, ip, &sa6.sin6_addr);

        if (::bind(fd, (struct sockaddr*)&sa6, sizeof(sa6)) < 0) {
            LOG(Error, "IPv6 bind failed. fd:%d,err:%s", fd, strerror(errno));
            return false;
        }
    } else {
        LOG(Error, "unknown family:%d", family);
        return false;
    }

    if (::listen(fd, 5) < 0) {
        LOG(Error, "listen failed. fd:%d,err:%s", fd, strerror(errno));
        return false;
    }

    Util::set_fl(fd, O_NONBLOCK);
    listenfd = fd;
    return true;
}

void Server::run() {
    if (isWorker()) {
        workerRun();
    } else {
        watcherRun();
    }
}

static void sigHandler(int signo) {
    int save_errno = errno;
    if (-1 ==send(sig_pipefd[1], (char*)&signo, 1, 0)) {
        LOG(Error, "send failed, err:%s", strerror(errno));
    }
    errno = save_errno;
}

void Server::watcherRun() {
    prctl(PR_SET_NAME, (unsigned long)"rpc-watcher", 0, 0, 0);
    LOG(Info, "watcher pid:%d", getpid());

    poller_ = new Poller(50);
    assert(poller_);

    Util::registerSignal(SIGCHLD, sigHandler);
    Util::registerSignal(SIGTERM, sigHandler);
    Util::registerSignal(SIGINT, sigHandler);

    // create pipe for signal event,
    // sig_pipefd[1] writen by sigHandler(SIGCHLD)
    // sig_pipefd[0] read by parent who check whether child is exit by waitpid 
    if (-1 == socketpair(AF_LOCAL, SOCK_STREAM, 0, sig_pipefd)) {
        LOG(Error, "socketpair err:%s", strerror(errno));
        return;
    }

    Util::set_fl(sig_pipefd[0], O_NONBLOCK);

    poller_->setFdReadCallback(sig_pipefd[0],
        std::bind(&Server::onSigPipeFdOfWatcher, this, std::placeholders::_1, 
            std::placeholders::_2, std::placeholders::_3), this); 
    poller_->addFd(sig_pipefd[0], EV_READ);

    poller_->setTimeout(50);
    poller_->runLoop();
}

void Server::workerRun() {
    // cat /proc/<pid>/comm to get worker name
    char workerName[32] = {0};
    snprintf(workerName, sizeof(workerName), "rpc-worker-%d", workerIndex_);
    prctl(PR_SET_NAME, (unsigned long)workerName, 0, 0, 0);
    LOG(Info, "worker pid:%d", getpid());

    poller_ = new Poller(opt_.commonOption_.maxConnNum_);
    assert(poller_);
    
    Util::registerSignal(SIGTERM, sigHandler);
    
    // create pipe for signal event,
    // sig_pipefd[1] writen by sigHandler(SIGTERM)
    // sig_pipefd[0] read by child to exit  
    if (-1 == socketpair(AF_LOCAL, SOCK_STREAM, 0, sig_pipefd)) {
        LOG(Error, "socketpair err:%s", strerror(errno));
        return;
    }

    Util::set_fl(sig_pipefd[0], O_NONBLOCK);
    Util::set_fl(workerList_[workerIndex_].pipefd[0], O_NONBLOCK);

    const char *ip = opt_.serviceAddrOption_.ip_.c_str();
    uint16_t port = opt_.serviceAddrOption_.port_;
    int family = opt_.serviceAddrOption_.isIPv6_ ? AF_INET6 : AF_INET;
    int listenfd = -1;
    if (!listenOnAddress(family, ip, port, listenfd)) {
        LOG(Error, "listenOnAddress failed, ip:%s, port:%d", ip, port);
        return;
    }
    listenfd_ = listenfd;
    LOG(Info, "listenfd:%d", listenfd_);

    poller_->setFdReadCallback(sig_pipefd[0],
        std::bind(&Server::onSigPipeFdOfWorker, this, std::placeholders::_1, 
            std::placeholders::_2, std::placeholders::_3), this);
    poller_->addFd(sig_pipefd[0], EV_READ);

    poller_->setFdReadCallback(listenfd_,
        std::bind(&Server::onAccept, this, std::placeholders::_1, 
            std::placeholders::_2, std::placeholders::_3), this);
    poller_->addFd(listenfd_, EV_READ/*|EV_EXCLUSIVE*/);

    poller_->setFdReadCallback(workerList_[workerIndex_].pipefd[0],
        std::bind(&Server::onPipeFdOfWorker, this, std::placeholders::_1, 
            std::placeholders::_2, std::placeholders::_3), this);
    poller_->addFd(workerList_[workerIndex_].pipefd[0], EV_READ);
    
    poller_->addTimer(5000, true, 
        std::bind(&Server::checkIdleConnections, this, std::placeholders::_1,
            std::placeholders::_2, std::placeholders::_3), this);

    poller_->setTimeout(10);
    poller_->runLoop();
}

void Server::onSigPipeFdOfWatcher(int fd, int events, void* arg) {
    // handle signal 
    char buf[PROCESS_MAXNUM] = {0};
    int fdErr = FD_ERR_NONE;
    ssize_t rlen = Connection::myRecv(fd, buf, sizeof(buf), fdErr);

    for (int i = 0; i < rlen; i++) {
        switch (buf[i]) {
            case SIGCHLD:
                // SIGCHLD 信号处理函数是阻塞的，如果同时触发多个 SIGCHLD 的话，
                // SIGCHLD信号不会排队，只调用一次信号处理函数
                pid_t ret_pid;
                while (-1 == (ret_pid = waitpid(-1, NULL, WNOHANG))) {
                    continue;
                }

                for (int n = 0; n < workerNum_; n++) {
                    if (ret_pid == workerList_[n].pid) {
                        workerList_[n].pid = -1;
                        close(workerList_[n].pipefd[1]);
                        LOG(Info, "watcher catch SIGCHLD, worker pid:%d exit!",
                             ret_pid);
                    }
                }
                break;
            case SIGTERM:
            case SIGINT:
                // parent ternination, so kill all children
                for (int n = 0; n < workerNum_; n++) {
                    if (-1 != workerList_[n].pid) {
                        kill(workerList_[n].pid, SIGTERM);
                        LOG(Info, "watcher catch SIGTERM or SIGINT, "
                            "so kill worker pid:%d", workerList_[n].pid);
                    }
                }
                            
                // 父进程退出runLoop
                poller_->stop();
                break;
            default:
                break;
        }
    }

    // 假如子进程都退出，则父进程也退出
    bool isAllChildrenStop = true;
    for (int n = 0; n < workerNum_; n++) {
        if (-1 != workerList_[n].pid) {
            isAllChildrenStop = false;
        }
    }
    if (isAllChildrenStop) {
        poller_->stop();
    }
}

void Server::onSigPipeFdOfWorker(int fd, int events, void* arg) {
    char buf[PROCESS_MAXNUM] = {0};
    int fdErr = FD_ERR_NONE;
    ssize_t rlen = Connection::myRecv(fd, buf, sizeof(buf), fdErr);

    for (int n = 0; n < rlen; n++) {
        switch (buf[n]) {
            case SIGINT:
                LOG(Info, "worker pid:%d catch SIGTERM or SIGINT", getpid());
                poller_->stop();
                break;
            default:
                break;
        }
    }
}

void Server::onPipeFdOfWorker(int fd, int events, void* arg) {
    if (!(events & EV_READ)) {
        LOG(Error, "not EV_READ, fd:%d,events:%0x", fd, events);
        return;
    }
}

void Server::onAccept(int fd, int events, void* arg) {
    LOG(Debug, "fd:%d,events:%d", fd, events);

    struct sockaddr_storage clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    int acceptfd = accept(fd, (struct sockaddr*)&clientAddr, &addrLen);
    if (acceptfd < 0) {
        if (EINTR == errno || EAGAIN == errno /*|| EWOULDBLOCk == errno*/) {
            return; 
        } else {
            LOG(Error, "accept failed! err:%s", strerror(errno));
            return;
        }
    }

    // limit acceptfd <= maxfd_, EventItemList in poller is array that index is fd
    if (unlikely(acceptfd > maxfd_)) {
        LOG(Warn, "too many conn, fd:%d, limited maxfd:%d", acceptfd, maxfd_);
        close(acceptfd);
        return;
    }

    Util::set_fl(acceptfd, O_NONBLOCK);

    auto conn = connPool_.get();
    if (!conn) {
        close(acceptfd);
        LOG(Error, "connPool_ get failed, acceptfd:%d", acceptfd);
        return;
    }
    conn->setFd(acceptfd);
    conn->setStatus(CONN_STATUS_OK);
    conn->updateLastActiveTime(time(nullptr));

    AddrInfo *addr = conn->getRemoteAddr();
    if (clientAddr.ss_family == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *)&clientAddr;
        inet_ntop(AF_INET, &sin->sin_addr, addr->ip, AddrInfo::IP_SIZE);
        addr->port = ntohs(sin->sin_port);
        conn->setFamily(AF_INET);
    } else if (clientAddr.ss_family == AF_INET6) {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&clientAddr;
        inet_ntop(AF_INET6, &sin6->sin6_addr, addr->ip, AddrInfo::IP_SIZE);
        addr->port = ntohs(sin6->sin6_port);
        conn->setFamily(AF_INET6);
    } else {
        LOG(Error, "unknown address family: %d", clientAddr.ss_family);
        strcpy(addr->ip, "UNKNOWN");
        addr->port = 0;
    }

    connMap_[acceptfd] = conn;

    poller_->setFdReadCallback(acceptfd, 
        [this, fd = acceptfd](int fd, int events, void* arg) {
            onRead(fd, events, connMap_[fd]);
        }, 
        nullptr);
    poller_->setFdWriteCallback(acceptfd, 
        [this, fd = acceptfd](int fd, int events, void* arg) {
            onWrite(fd, events, connMap_[fd]);
        }, 
        nullptr);
        
    poller_->addFd(acceptfd, EV_READ);
        
    LOG(Info, "acceptfd:%d,remote ip:%s,port:%u", 
        acceptfd, addr->ip, addr->port);
}

void Server::onRead(int fd, int events, const std::shared_ptr<Connection>& conn) {
    bool isUpdate = false;

    if (conn->tcpRecv()) {
        if (!codec_->processMessage(conn.get())) {
            LOG(Error, "processMessage pack fail or protocolType err,"
                "delFd fd:%d,client ip:%s,port:%u",
                fd, conn->getRemoteAddr()->ip, conn->getRemoteAddr()->port);
            clearConnAndEraseFromConnMap(conn);
            return;
        }
        isUpdate = true;
    }

    if (conn->hasPendingRsp()) {
        conn->tcpSend();
        if (conn->hasPendingRsp()) {
            if (!(events & EV_WRITE)) {
                poller_->addEvent(fd, EV_WRITE);
            } 
        } else {
            if (events & EV_WRITE) {
                poller_->delEvent(fd, EV_WRITE);
            }
        }
        isUpdate = true;
    }

    if (isUpdate) {
        conn->updateLastActiveTime(time(nullptr));
    }

    if (conn->getStatus() == CONN_STATUS_BROKEN) {
        LOG(Info, "sock broken! delFd fd:%d,client ip:%s,port:%u", 
            fd, conn->getRemoteAddr()->ip, conn->getRemoteAddr()->port);
        clearConnAndEraseFromConnMap(conn);
        return;
    }
}

void Server::onWrite(int fd, int events, const std::shared_ptr<Connection>& conn) {
    if (conn->hasPendingRsp()) {
        if (!conn->tcpSend()) {
            LOG(Error, "tcpSend err, delFd fd:%d,client ip:%s,port:%u",
                fd, conn->getRemoteAddr()->ip, conn->getRemoteAddr()->port);
            clearConnAndEraseFromConnMap(conn);
        }

        if (!conn->hasPendingRsp()) {
            poller_->delEvent(fd, EV_WRITE);
        }
        conn->updateLastActiveTime(time(nullptr));
    }

    if (conn->getStatus() == CONN_STATUS_BROKEN) {
        LOG(Info, "sock broken! delFd fd:%d,client ip:%s,port:%u", 
            fd, conn->getRemoteAddr()->ip, conn->getRemoteAddr()->port);
        clearConnAndEraseFromConnMap(conn);
        return;
    }
}

void Server::checkIdleConnections(int fd, int events, void* arg) {
    time_t now = time(NULL);
    const time_t idleTimeout = opt_.commonOption_.idleTimeout_;

    auto it = connMap_.begin();
    while (it != connMap_.end()) {
        int fd = it->first;
        std::shared_ptr<Connection>& conn = it->second;
        if (!conn || conn->getStatus() != CONN_STATUS_OK) {
            ++it;
            continue;
        }

        if (now - conn->getLastActiveTime() > idleTimeout) {
            LOG(Info, "Gracefully closing idle connection, "
                "fd:%d, remote ip:%s, port:%u",
                fd, conn->getRemoteAddr()->ip, conn->getRemoteAddr()->port);

            // attempt shutdown gracefully
            if (conn->hasPendingRsp()) {
                conn->tcpSend();
                if (conn->hasPendingRsp()) {
                    // re-check later
                    conn->updateLastActiveTime(now);
                    ++it;
                    continue;
                }
            }

            LOG(Info, "close idle connection, fd:%d, remote ip:%s, port:%u",
                fd, conn->getRemoteAddr()->ip, conn->getRemoteAddr()->port);

            // no pending data, close the connection
            shutdown(conn->getFd(), SHUT_WR);
            it = clearConnAndEraseFromConnMap(conn);
        } else {
            ++it;
        }
    }
}

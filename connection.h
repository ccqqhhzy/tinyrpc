// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include "buffer.h"
#include "poller.h"
#include <unistd.h>
#include <sys/types.h>


namespace tinyrpc {

enum FdErrorInfo {
    FD_ERR_NONE = 0,
    FD_ERR_BROKEN,
    FD_ERR_OTHER,
};

enum ConnStatus {
    CONN_STATUS_NONE = 0,
    CONN_STATUS_OK,
    CONN_STATUS_BROKEN,
};

typedef struct AddrInfo {
    enum {
        IP_SIZE = 128,
    };
    char ip[IP_SIZE];
    uint32_t port;
}AddrInfo;

class Connection {
public:
    explicit Connection(int fd = -1, uint32_t bufSize = Buffer::BUF_DEFAULT_SIZE);
    Connection(const Connection& c) = delete;
    Connection& operator = (const Connection& c) = delete;
    ~Connection();

    void setFd(int fd) { sockfd_ = fd; }
    int getFd() const { return sockfd_; }
    void setStatus(ConnStatus status) { status_ = status; }
    int getStatus() const { return status_; }
    bool isOk() const { return status_ == CONN_STATUS_OK; }
    bool isEquel(int fd) { return sockfd_ == fd; }
    void setFamily(int family) { family_ = family; }
    int getFamily() const { return family_; }
    void updateLastActiveTime(time_t t) { lastActiveTime_ = t; }
    time_t getLastActiveTime() const { return lastActiveTime_; }

    static ssize_t myRecv(int fd, char *buf, size_t len, int &fdErr);
    static ssize_t mySend(int fd, char *buf, size_t len, int &fdErr);

    bool tcpRecv();
    bool tcpSend();

    bool intoSndBuf(const char* inbuff, uint32_t len);
    bool outRcvBuf(char* outbuff, uint32_t len);
    Buffer* getSndBuf() { return sndbuf_; }
    Buffer* getRcvBuf() { return rcvbuf_; }
    bool hasPendingRsp() { return sndbuf_->size() > 0; }

    AddrInfo* getRemoteAddr() { return &remoteAddr_; }
    AddrInfo* getLocalAddr() { return &localAddr_; }

    static void setReusePort(int sockfd);

    void reset() {
        sockfd_ = -1;
        status_ = CONN_STATUS_NONE;
        rcvbuf_->reset();
        sndbuf_->reset();
        lastActiveTime_ = 0;
    }

private:
    int sockfd_;
    ConnStatus status_;
    int family_;

    Buffer* rcvbuf_;
    Buffer* sndbuf_;
    
    AddrInfo remoteAddr_;
    AddrInfo localAddr_;

    time_t lastActiveTime_;
};

} // namespace tinyrpc

#endif /*__CONNECTION_H__*/


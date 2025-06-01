// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include "connection.h"
#include "log.h"
#include <assert.h>
#include <sys/socket.h>

using namespace tinyrpc;

Connection::Connection(int fd, uint32_t bufSize) 
    : sockfd_(fd), 
      status_(CONN_STATUS_NONE),
      family_(AF_INET),
      rcvbuf_(new Buffer(bufSize)),
      sndbuf_(new Buffer(bufSize)) {
    
}

Connection::~Connection() {
    if (sockfd_ > 0 && status_ == CONN_STATUS_OK) {
        close(sockfd_);
    }

    if (rcvbuf_) {
        delete rcvbuf_;
        rcvbuf_ = NULL;
    }

    if (sndbuf_) {
        delete sndbuf_;
        sndbuf_ = NULL;
    }
}

ssize_t Connection::myRecv(int fd, char *buf, size_t len, int &fdErr) {
    ssize_t ret = 0;
    ssize_t rlen = 0;
    if (len <= 0) {
        return 0;
    }

    while (true) {
        rlen = ::recv(fd, buf + ret, len - ret, 0);
        if (rlen == 0) {
            LOG(Info,"rlen = 0, fd closed! fd:%d", fd);
            fdErr = FD_ERR_BROKEN;
            break;
        } else if (rlen < 0) {
            int err = errno;
            if (EINTR == err) {
                continue;
            } else if (err == EAGAIN || err == EWOULDBLOCK) {
                break;
            } else {
                LOG(Error, "recv failed! fd:%d,err:%s", fd, strerror(err));
                ret = -1;
                fdErr = FD_ERR_OTHER;
                break;
            }
        }

        ret += rlen;
        if (ret == (ssize_t)len) {
            break;
        }
    }

    return ret;
}

ssize_t Connection::mySend(int fd, char *buf, size_t len, int &fdErr) {
    ssize_t ret = 0;
    if (len <= 0) {
        return 0;
    }

    ssize_t slen = 0;
    while (true) {
        slen = ::send(fd, buf + ret, len - ret, 0);
        if(slen < 0) {
            if (errno == EINTR)
                continue;
            else if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                int err = errno;
                LOG(Error, "send failed! fd:%d,err:%s", fd, strerror(err));
                ret = -1;
                fdErr = FD_ERR_OTHER;
                break;
            }
        }

        ret += slen;
        if (ret == (ssize_t)len) {
            break;
        }
    }

    return ret;
}

bool Connection::tcpRecv()
{
    assert(rcvbuf_);
    uint32_t free_size = rcvbuf_->getFreeSize();
    char *start = rcvbuf_->getWritePtr();
    if (nullptr == start || free_size == 0) {
        return false;
    }
    
    int fdErr = FD_ERR_NONE;
    ssize_t len = myRecv(sockfd_, start, free_size, fdErr);
    
    if (fdErr == FD_ERR_BROKEN || fdErr == FD_ERR_OTHER) {
        status_ = CONN_STATUS_BROKEN;
        return false;
    }

    if (len < 0) {
        return false;
    }
    
    rcvbuf_->setWriteSize(len);    
    return true;
}

bool Connection::tcpSend()
{
    assert(sndbuf_);
    int data_size = sndbuf_->size();
    char *start = sndbuf_->getReadPtr();
    if (nullptr == start) {
        return false;
    }

    if (0 == data_size) {
        return true;
    }

    int fdErr = FD_ERR_NONE;
    ssize_t len = mySend(sockfd_, start, data_size, fdErr);
    
    if (fdErr == FD_ERR_BROKEN || fdErr == FD_ERR_OTHER) {
        status_ = CONN_STATUS_BROKEN;
        return false;
    }

    if (len < 0) {
        return false;
    }
    
    sndbuf_->setReadSize(len);
    return true;
}

bool Connection::intoSndBuf(const char* inbuff, uint32_t len)
{
    assert(sndbuf_);
    return sndbuf_->in(inbuff, len);
}

bool Connection::outRcvBuf(char* outbuff, uint32_t len)
{
    assert(rcvbuf_);
    return rcvbuf_->out(outbuff, len);
}

void Connection::setReusePort(int sockfd)
{
#ifdef SO_REUSEPORT
    int optval = 1;
    int ret = ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, 
                    &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0) {
        LOG(Error, "SO_REUSEPORT failed. fd:%d", sockfd);
    } else {
        LOG(Error, "SO_REUSEPORT ok. fd:%d", sockfd);
    }
#else
    LOG(Error, "SO_REUSEPORT is not supported. fd:%d", sockfd);
#endif
}
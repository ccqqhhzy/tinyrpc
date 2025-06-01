// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include "socket.h"
#include "log.h"
#include "util.h"
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <iostream>

using namespace tinyrpc;
using namespace std;

void Socket::setNonBlocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int Socket::connect(const std::string& host, const std::string& port, 
                    int timeoutMs) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // 支持 IPv4 或 IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_protocol = IPPROTO_TCP;

    // 解析地址
    int s = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
    if (s != 0) {
        LOG(Error, "getaddrinfo failed, err:%s", gai_strerror(s));
        return -1;
    }

    int sockfd = -1;
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        sockfd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1)
            continue;

        setNonBlocking(sockfd);

        int ret = ::connect(sockfd, rp->ai_addr, rp->ai_addrlen);
        if (ret == 0) {
            // 立即连接成功（罕见）
            freeaddrinfo(result);
            return sockfd;
        } else if (errno == EINPROGRESS) {
            // 非阻塞 connect 正在进行中，等待可写事件
            struct pollfd pfd;
            pfd.fd = sockfd;
            pfd.events = POLLOUT;

            ret = ::poll(&pfd, 1, timeoutMs);
            if (ret == 0) {
                // 超时
                close(sockfd);
                sockfd = -1;
                LOG(Error, "Connect timeout,host:%s,port:%s", 
                    host.c_str(), port.c_str());
                continue;
            } else if (ret < 0) {
                close(sockfd);
                sockfd = -1;
                LOG(Error, "Connect failed, poll failed:%s,host:%s,port:%s",
                    strerror(errno), host.c_str(), port.c_str());
                continue;
            }

            // 检查连接是否出错
            int so_error;
            socklen_t len = sizeof(so_error);
            if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0 
                    || so_error != 0) {
                close(sockfd);
                sockfd = -1;
                LOG(Error, "Connect getsockopt failed:%s,host:%s,port:%s",
                    strerror(so_error ? so_error : errno), host.c_str(), 
                    port.c_str());
                continue;
            }

            // 成功连接
            break;
        } else {
            // 其他错误
            close(sockfd);
            sockfd = -1;
            LOG(Error, "Connect failed:%s,host:%s,port:%s",
                strerror(errno), host.c_str(), port.c_str());
            continue;
        }
    }

    freeaddrinfo(result);

    if (sockfd == -1) {
        return -1; // 所有地址都无法连接
    }

    // 恢复为阻塞模式（可选）
    Util::clear_fl(sockfd, O_NONBLOCK);

    return sockfd;
}
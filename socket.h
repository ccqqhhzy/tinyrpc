// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <string>

namespace tinyrpc {


class Socket {
public:
    // 连接远程主机，返回 socket fd
    static int connect(const std::string& host, const std::string& port, 
                       int timeoutMs);

    // 设置 socket 为非阻塞模式
    static void setNonBlocking(int sockfd);

    // TODO: 添加更多 socket 相关操作 no delay、keep alive、reuse addr、reuse port、SO_LINGER 等
};

} // namespace tinyrpc
#endif // __SOCKET_H__
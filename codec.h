// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __CODEC_H__
#define __CODEC_H__

#include "protocol.h"
#include "connection.h"
#include "dispatcher_pb/protocol_pb.h"
#include "dispatcher_cc/protocol_cc.h"
#include <stdint.h>
#include <string>
#include <poll.h>
#include <unordered_map>

namespace tinyrpc {

class Codec {
public:
    explicit Codec();
    virtual ~Codec() {}

    bool processMessage(Connection* conn);

    // at client side: syn call wait for rsp
    template<typename T>
    bool processResponse(Connection* conn, std::shared_ptr<T>& rsp, 
                         uint32_t timeout = 3000);

    static bool sendMessage(Connection* conn, uint32_t protocolType,
                            uint32_t protocolUri, const std::string& message);

    std::shared_ptr<Protocol> getProtocol(uint32_t protocolType);

private:
    int unpack(Connection* conn, ProtocolHead& head, char** data, uint32_t& len);

    static bool pack(Connection* conn, uint32_t protocolType, 
        uint32_t protocolUri, const std::string& message);

    std::unordered_map<uint32_t, std::shared_ptr<Protocol>> protocolMap_;

};

template<typename T>
bool Codec::processResponse(Connection* conn, std::shared_ptr<T>& rsp, 
                            uint32_t timeout)
{
    struct timeval begin_time;
    gettimeofday(&begin_time, NULL);
    uint32_t elapsed = 0;

    while (true) {
        struct pollfd pfd;
	    pfd.fd = conn->getFd();
	    pfd.events = POLLIN;
	    pfd.revents = 0;

        if (elapsed >= timeout) {
            LOG(Warn, "poll timeout expired");
            return false;
        }
        int remaining_timeout = timeout - elapsed;

	    int pret = ::poll(&pfd, 1, remaining_timeout);
        if (0 == pret) {
            // timed out and no file descriptors were ready
            return false;
        } else if (pret < 0) {
            LOG(Error, "poll failed! fd:%d,err:%s", 
                conn->getFd(), strerror(errno));
            return false;
        }

        // pret > 0 ：数据可读
        if (!conn->tcpRecv()) {
            return false;
        }

        ProtocolHead head;
        char* package = nullptr;
        uint32_t packageSize = 0;

        int ret = unpack(conn, head, &package, packageSize);
        if (0 == ret) {
            // 计算剩余超时时间
            struct timeval current_time;
            gettimeofday(&current_time, NULL);
            elapsed = (current_time.tv_sec - begin_time.tv_sec) * 1000 
                + (current_time.tv_usec - begin_time.tv_usec) / 1000;
            // 包不完整，继续接收数据
            continue;
        } else if (ret < 0) {
            return false;
        }

        if (protocolMap_.find(head.protocolType) == protocolMap_.end()) {
            LOG(Error, "unknown protocol type:0x%x", head.protocolType);
            return false;
        }
        
        VoidPtr message;
        if (!protocolMap_[head.protocolType]->parseToMessage(package,
                packageSize, head.protocolUri, message)) {
            return false;
        }
        std::shared_ptr<T> concrete = std::static_pointer_cast<T>(message);
        assert(message);
        rsp = concrete;
        
        break;
    } // while

    return true;
}

} //namespace tinyrpc

#endif // __CODEC_H__ 
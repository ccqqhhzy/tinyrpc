// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __CLIENT_RPC_H__
#define __CLIENT_RPC_H__

#include "option.h"
#include "connection.h"
#include "codec.h"
#include "protocol.h"
#include "socket.h"
#include "log.h"
#include "util.h"
#include <string>
#include <functional>
#include <memory>

namespace tinyrpc {

template<typename CLIENT>
class RpcClient : public std::enable_shared_from_this<RpcClient<CLIENT>> {
public:
    friend class AsynCallPoller;

    explicit RpcClient(const ClientOptions& opt)
        : options_(opt)
        , conn_(nullptr)
        , codec_(new Codec())
        , isConn_(false)
        , isMakeAsync_(false) {
        connect();
    }

    virtual ~RpcClient() = default;

    bool isOk() {
        return isConn_ && conn_ && conn_->isOk();
    }

    bool reconn() {
        return isOk() ? true : connect();
    }

    virtual void makeAsync() = 0;

    // 异步回调注册
    template<typename REQ, typename RSP>
    bool registerCallback(std::function<void(const std::shared_ptr<RSP>&)> 
                          callback) {
        return static_cast<CLIENT*>(this)->template 
                            registerCallback<REQ, RSP>(callback);
    }

    // 同步调用
    template<typename REQ, typename RSP>
    bool synCall(const REQ& req, std::shared_ptr<RSP>& rsp, 
                 uint32_t timeout = 3000) {
        return static_cast<CLIENT*>(this)->template 
                            synCall<REQ, RSP>(req, rsp, timeout);
    }

    // 异步调用
    template<typename REQ>
    bool asynCall(const REQ& req) {
        return static_cast<CLIENT*>(this)->template asynCall<REQ>(req);
    }

    template<typename T>
    bool serialize(const T& req, std::string& out);

protected:
    virtual std::shared_ptr<Protocol> getProtocol() = 0;
    virtual ProtocolType protocolType() const = 0;

protected:
    bool connect();

    ClientOptions options_;
    std::shared_ptr<Connection> conn_;
    std::shared_ptr<Codec> codec_;
    bool isConn_;
    bool isMakeAsync_;
};

template<typename CLIENT>
bool RpcClient<CLIENT>::connect()
{
    std::string host = options_.serviceAddrOption.ip_;
    std::string port = std::to_string(options_.serviceAddrOption.port_);
    int timeoutMs = options_.connectTimeoutMs;

    int fd = Socket::connect(host, port, timeoutMs);
    if (fd < 0) {
        LOG(Error, "connect failed! host:%s, port:%s", host.c_str(), 
            port.c_str());
        return false;
    }

    // 获取本地地址
    struct sockaddr_in ss;
    socklen_t ss_len = sizeof(ss);
    if (getsockname(fd, (struct sockaddr *)&ss, &ss_len) < 0) {
        LOG(Error, "getsockname failed! err:%s", strerror(errno));
        close(fd);
        return false;
    }

    if (!conn_) {
        conn_ = std::make_shared<Connection>();
    } else {
        conn_->reset();
    }
    conn_->setFd(fd);
    conn_->setStatus(CONN_STATUS_OK);
    Util::set_fl(fd, O_NONBLOCK);

    AddrInfo *remoteAddr = conn_->getRemoteAddr();
    strncpy(remoteAddr->ip, host.c_str(), sizeof(remoteAddr->ip) - 1);
    remoteAddr->port = atoi(port.c_str());

    AddrInfo *localAddr = conn_->getLocalAddr();
    memcpy(&localAddr->ip, &ss.sin_addr, sizeof(ss.sin_addr));
    localAddr->port = ntohs(ss.sin_port);

    isConn_ = true;
    LOG(Info, "client connected from ip:%s,port:%d", 
        localAddr->ip, localAddr->port);
    return true;
}

} // namespace tinyrpc

#endif // __CLIENT_RPC_H__
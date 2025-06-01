// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __CLIENT_PB_H__
#define __CLIENT_PB_H__


#include "client_rpc.h"
#include <google/protobuf/message.h>
#include <chrono>

namespace tinyrpc {

class PbClient : public RpcClient<PbClient> {
public:
    using RpcClient::RpcClient;
    explicit PbClient(const ClientOptions& opt);
    ~PbClient() override = default;

    template<typename T>
    bool serialize(const T& req, std::string& out) {
        return req.SerializeToString(&out);
    }

    template<typename REQ, typename RSP>
    bool registerCallback(std::function<void(const std::shared_ptr<RSP>&)> 
                          callback);

    template<typename REQ, typename RSP>
    bool synCall(const REQ& req, std::shared_ptr<RSP>& rsp, 
                 uint32_t timeout = 3000);

    template<typename REQ>
    bool asynCall(const REQ& req);

    virtual void makeAsync() override;

protected:

    std::shared_ptr<Protocol> getProtocol() override;
    ProtocolType protocolType() const override;
};

template<typename REQ, typename RSP>
bool PbClient::registerCallback(std::function<void(
                                const std::shared_ptr<RSP>&)> callback) {
    if (options_.isAsync && !isMakeAsync_) {
        makeAsync();
    }

    auto protocol = getProtocol();
    if (!protocol) {
        return false;
    }

    auto dispatcher = std::static_pointer_cast<pb::Dispatcher>(
                                        protocol->getDispatcher());
    if (!dispatcher) {
        return false;
    }

    dispatcher->registerCallback<REQ, RSP>(callback);

    return true;
}

template<typename REQ, typename RSP>
bool PbClient::synCall(const REQ& req, std::shared_ptr<RSP>& rsp, 
                       uint32_t timeout) {
    if (!conn_ || !codec_) {
        return false;
    }

    auto protocol = getProtocol();
    if (!protocol) {
        return false;
    }

    auto dispatcher = std::static_pointer_cast<pb::Dispatcher>(
        protocol->getDispatcher());
    if (!dispatcher) {
        return false;
    }

    dispatcher->registerDescriptor<RSP>();

    auto startTime = std::chrono::steady_clock::now();

    std::string message;
    if (!serialize(req, message)) {
        LOG(Error, "Serialize failed, uri:0x%xu", REQ::URI);
        return false;
    }

    if (!Codec::sendMessage(conn_.get(), protocolType(), REQ::URI, message)) {
        LOG(Error, "Send message failed, uri:0x%xu", REQ::URI);
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - startTime).count();
    if (elapsedTime >= timeout) {
        LOG(Error, "Timeout exceeded before processResponse, uri:0x%xu", 
            REQ::URI);
        return false;
    }

    auto remainingTime = timeout - elapsedTime;
    if (!codec_->processResponse<RSP>(conn_.get(), rsp, remainingTime)) {
        LOG(Error, "Process response failed, uri:0x%xu", REQ::URI);
        return false;
    }

    return true;
}

template<typename REQ>
bool PbClient::asynCall(const REQ& req) {
    std::string message;
    if (!serialize(req, message)) {
        LOG(Error, "Serialize failed, uri:0x%xu", REQ::URI);
        return false;
    }
    if (!Codec::sendMessage(conn_.get(), protocolType(), REQ::URI, message)) {
        LOG(Error, "Send message failed, uri:0x%xu", REQ::URI);
        return false;
    }

    return true;
}

} // namespace tinyrpc
#endif // __CLIENT_PB_H__
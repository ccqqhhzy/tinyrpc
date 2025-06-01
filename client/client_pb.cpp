// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include "client_pb.h"
#include "asyncall_poller.h"

namespace tinyrpc {

PbClient::PbClient(const ClientOptions& opt)
    : RpcClient<PbClient>(opt) {
}

std::shared_ptr<Protocol> PbClient::getProtocol() {
    return codec_->getProtocol(PROTOCOL_TYPE_PB);
}

ProtocolType PbClient::protocolType() const {
    return PROTOCOL_TYPE_PB;
}

void PbClient::makeAsync() {
    AsynCallPoller::getInstance().addFd<PbClient>(conn_->getFd(), 
        shared_from_this());

    isMakeAsync_ = true;
}

} // namespace tinyrpc
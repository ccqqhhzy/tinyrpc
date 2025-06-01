// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

// cc_client.cpp

#include "client_cc.h"
#include "asyncall_poller.h"


namespace tinyrpc {

CcClient::CcClient(const ClientOptions& opt)
    : RpcClient<CcClient>(opt) {
}

std::shared_ptr<Protocol> CcClient::getProtocol() {
    return codec_->getProtocol(PROTOCOL_TYPE_CC);
}

ProtocolType CcClient::protocolType() const {
    return PROTOCOL_TYPE_CC;
}

void CcClient::makeAsync() {
    AsynCallPoller::getInstance().addFd<CcClient>(conn_->getFd(), 
        shared_from_this());

    isMakeAsync_ = true;
}

} // namespace tinyrpc
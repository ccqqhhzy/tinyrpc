// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include "protocol_cc.h"
#include "codec.h"

using namespace tinyrpc;
using namespace cc;

CcProtocol::CcProtocol()
    : dispatcher_(new Dispatcher()) {
}

bool CcProtocol::parseToMessage(const char* package, uint32_t packageSize, 
                                uint32_t protocolUri, VoidPtr& message) {
    MessagePtr mes = dispatcher_->createMessage(protocolUri);
    if (!mes) {
        return false;
    }

    uint32_t headLen = ProtocolHead::getLen();
    const char *data = package + headLen;
    int len = packageSize - headLen;
    Payload payload(const_cast<char*>(data), len);
    mes->unserialize(payload);
    message = mes;
    
    return true;
}

bool CcProtocol::serializeToString(VoidPtr& message, std::string& data) {
    MessagePtr mes = std::static_pointer_cast<Serializable>(message);
    Payload payload;
    mes->serialize(payload);
    data = payload.getData();
    return true;
}

bool CcProtocol::dispatch(const char* package, uint32_t packageSize, 
                          uint32_t protocolUri, Connection* conn) {
    VoidPtr voidMessage;
    if (!parseToMessage(package, packageSize, protocolUri, voidMessage)) {
        return false;
    }
    MessagePtr mes = std::static_pointer_cast<Serializable>(voidMessage);
    
    uint32_t rspUri = dispatcher_->getRspUri(protocolUri);
    if (0 == rspUri) {
        LOG(Error, "getRspUri ret 0! protocolUri:%d", protocolUri);
        return false;
    }

    if (protocolUri != rspUri) {
        // server-side request callback
        // create rsp
        MessagePtr rsp = dispatcher_->createMessage(rspUri);
        assert(rsp);

        // handle request
        if (!dispatcher_->onServerRequest(protocolUri, mes, rsp)) {
            LOG(Error, "onServerRequest fail, protocolUri:%d,rspUri:%d", 
                protocolUri, rspUri);
            return false;
        }

        cc::Payload payload;
        rsp->serialize(payload);

        if (!Codec::sendMessage(conn, PROTOCOL_TYPE_CC, rspUri, 
                                payload.getData())) {
            LOG(Error, "sendMessage fail, protocolUri:%d,rspUri:%d",
                protocolUri, rspUri);
            return false;
        }
    } else {
        // client-side async response callback
        if (!dispatcher_->onAsyncResponse(protocolUri, mes)) {
            LOG(Error, "onAsyncResponse fail, protocolUri:%d", protocolUri);
            return false;
        }
    }

    return true;
}

VoidPtr CcProtocol::getDispatcher() {
    return dispatcher_;
}
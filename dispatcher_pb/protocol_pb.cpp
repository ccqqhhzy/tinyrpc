// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include "protocol_pb.h"
#include "codec.h"

using namespace tinyrpc;
using namespace pb;

PbProtocol::PbProtocol()
    : dispatcher_(new Dispatcher()) {
}

bool PbProtocol::parseToMessage(const char* package, uint32_t packageSize, 
                                uint32_t protocolUri, VoidPtr& message) {
    MessagePtr mes = dispatcher_->createMessage(protocolUri);
    if (mes) {
        uint32_t headLen = ProtocolHead::getLen();
        const char *data = package + headLen;
        int len = packageSize - headLen;
        if (mes->ParseFromArray(data, len)) {
            message = mes;
            return true;
        } else {
            LOG(Error, "ParseFromArray failed! protocolUri:0x%xu", 
                protocolUri);
            return false;
        }
    }
    
    return false;
}

bool PbProtocol::serializeToString(VoidPtr& message, std::string& data) {
    MessagePtr mes = std::static_pointer_cast<google::protobuf::Message>(message);

    if (!mes->SerializeToString(&data)) {
        return false;
    }

    return true;
}

bool PbProtocol::dispatch(const char* package, uint32_t packageSize, 
                          uint32_t protocolUri, Connection* conn) {
    VoidPtr voidMessage;
    if (!parseToMessage(package, packageSize, protocolUri, voidMessage)) {
        LOG(Error, "parseToMessage fail, protocolUri:%d", protocolUri);
        return false;
    }
    MessagePtr mes = std::static_pointer_cast<google::protobuf::Message>(
        voidMessage);
    
    uint32_t rspUri = dispatcher_->getRspUri(protocolUri);
    if (0 == rspUri) {
        LOG(Error, "getRspUri ret 0! protocolUri:%d", protocolUri);
        return false;
    }

    if (protocolUri != rspUri) {
        // server-side request callback
        // create rsp
        MessagePtr rsp = dispatcher_->createMessage(rspUri);

        // handle request
        if (!dispatcher_->onServerRequest(protocolUri, mes, rsp)) {
            LOG(Error, "onServerRequest fail, protocolUri:%d,rspUri:%d",
                protocolUri, rspUri);
            return false;
        }

        std::string str;
        if (!rsp->SerializeToString(&str)) {
            LOG(Error, "SerializeToString fail, protocolUri:%d,rspUri:%d",
                protocolUri, rspUri);
            return false;
        }

        if (!Codec::sendMessage(conn, PROTOCOL_TYPE_PB, rspUri, str)) {
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

VoidPtr PbProtocol::getDispatcher() {
    return dispatcher_;
}
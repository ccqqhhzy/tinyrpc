// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include "codec.h"
#include "log.h"
#include <string.h>

using namespace tinyrpc;
using namespace tinyrpc::cc;
using namespace tinyrpc::pb;

Codec::Codec() {
    protocolMap_[PROTOCOL_TYPE_PB] = std::make_shared<PbProtocol>();
    protocolMap_[PROTOCOL_TYPE_CC] = std::make_shared<CcProtocol>();
}

bool Codec::processMessage(Connection* conn) {
    while (true) {
        ProtocolHead head;
        char* package = nullptr;
        uint32_t packageSize = 0;

        int ret = unpack(conn, head, &package, packageSize);
        if (0 == ret) {
            return true;
        } else if (ret < 0) {
            return false;
        }

        //head.dump();

        if (protocolMap_.find(head.protocolType) == protocolMap_.end()) {
            LOG(Error, "unknown protocol type:0x%x,fd:%d", 
                head.protocolType, conn->getFd());
            return false;
        }
        
        protocolMap_[head.protocolType]->dispatch(package, packageSize, 
            head.protocolUri, conn);
    }
}

int Codec::unpack(Connection* conn, ProtocolHead& head, char** data, 
                  uint32_t& len) {
    Buffer *rcvbuf = conn->getRcvBuf();
    assert(rcvbuf);

    uint32_t data_size = rcvbuf->size();
    if (0 == data_size) {
        return 0;
    }
        
    char* package = rcvbuf->getReadPtr();
    assert(package);
    uint32_t packageSize = 0;
    MsgLengthStatus status = ProtocolHead::getPackageSize(package, 
            data_size, packageSize);

    if (MSG_LEN_STATUS_OK == status) {
        // get package len
    } else if (MSG_LEN_STATUS_NOT_COMPLETE == status) {
        return 0;
    } else if (MSG_LEN_STATUS_ERR == status) {
        LOG(Error, "getPackageSize failed, status:%d, remote:%s:%d", 
            status, conn->getRemoteAddr()->ip, conn->getRemoteAddr()->port);
        return -1;
    } 
    
    head.unpack(package, packageSize);
    rcvbuf->setReadSize(packageSize);

    *data = package;
    len = packageSize;

    // TODO checksum，return -1 if check err

    return packageSize;
}

bool Codec::sendMessage(Connection* conn, uint32_t protocolType, 
                        uint32_t protocolUri, const std::string& message) {
    if (pack(conn, protocolType, protocolUri, message)) {
        return conn->tcpSend();
    }
    return false;
}

bool Codec::pack(Connection* conn, uint32_t protocolType,
                 uint32_t protocolUri, const std::string& message) {
    const char *body = message.c_str();
    uint32_t payloadLen = message.length();
    uint32_t headLen = ProtocolHead::getLen();
    
    ProtocolHead head;
    head.length = headLen + payloadLen;
    head.protocolType = protocolType;
    head.protocolUri = protocolUri;

    // TODO create checksum
    //head.checksum = xxx;

    char *start = conn->getSndBuf()->appendAt(headLen);
    if (!head.pack(start, headLen)) {
        LOG(Error, "pack failed! protocolUri:0x%xu", protocolUri);
        return false;
    }

    if (!conn->intoSndBuf(body, payloadLen)) {
        LOG(Error, "intoSndBuf failed! protocolUri:0x%xu", protocolUri);
        return false;
    }
    
    return true;
}

std::shared_ptr<Protocol> Codec::getProtocol(uint32_t protocolType) {
    if (protocolMap_.find(protocolType) == protocolMap_.end()) {
        return nullptr;
    }
    return protocolMap_[protocolType];
}
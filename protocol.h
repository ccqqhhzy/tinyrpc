// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "log.h"
#include "connection.h"
#include <stdint.h>
#include <arpa/inet.h>
#include <memory>

/*
#define OFFSETOF(type, member) \
    ((size_t)(((type*)0)->member))


#define MSG_MAX_SIZE 4096 
*/




namespace tinyrpc {

enum ProtocolType {
    PROTOCOL_TYPE_PB = 0, // google protobuf
    PROTOCOL_TYPE_CC,     // normal binary serialize/unserialize
};

enum MsgLengthStatus {
    MSG_LEN_STATUS_OK = 0,
    MSG_LEN_STATUS_ERR = 1,
    MSG_LEN_STATUS_NOT_COMPLETE = 2,
};

enum {
    PROTOCOL_TRACEID_SIZE = 32
};

struct ProtocolHead {
    uint32_t length;
    uint8_t protocolType;
    uint32_t protocolUri;
    uint32_t checksum;
    char traceId[PROTOCOL_TRACEID_SIZE]; // eg:uuid or snowflake etc

    ProtocolHead() 
        : length(0)
        , protocolType(PROTOCOL_TYPE_PB)
        , protocolUri(0)
        , checksum(0) {
        memset(traceId, 0, PROTOCOL_TRACEID_SIZE);
    }

    inline static uint32_t getLen() { return sizeof(ProtocolHead); }

    inline bool unpack(const char* buff, uint32_t len) {
        if(nullptr == buff || len < getLen()) {
            return false;
        }    

        const char* pos = buff;

        length = ntohl(*(const uint32_t*)pos);
        pos += sizeof(length);
    
        protocolType = *(const uint8_t*)pos;
        pos += sizeof(protocolType);
    
        protocolUri = ntohl(*(const uint32_t*)pos);
        pos += sizeof(protocolUri);
    
        checksum = ntohl(*(const uint32_t*)pos);
        pos += sizeof(checksum);
    
        memcpy(traceId, pos, PROTOCOL_TRACEID_SIZE);

        return true;
    }

    inline bool pack(char* buff, uint32_t len) {
        if (nullptr == buff || len < getLen()) {
            return false;
        }

        char *position = buff;
        *(uint32_t*)position = htonl(length);
        position += sizeof(length);

        *(uint8_t*)position = protocolType;
        position += sizeof(protocolType);

        *(uint32_t*)position = htonl(protocolUri);
        position += sizeof(protocolUri); 

        *(uint32_t*)position = htonl(checksum);
        position += sizeof(checksum);

        memcpy(position, traceId, PROTOCOL_TRACEID_SIZE);    

        return true;
    }

    inline void dump() const {
        LOG(Info, "[ProtocolHead,len:%u,protocolType:%u,protocolUri:0x%xu]", 
            length, protocolType, protocolUri);
    }

    static MsgLengthStatus getPackageSize(char* buff, uint32_t len, 
                                          uint32_t& packageSize) {
        if (len <= sizeof(ProtocolHead)) {
            return MSG_LEN_STATUS_NOT_COMPLETE;
        }
 
        uint32_t msg_len = ntohl(((ProtocolHead*)buff)->length);
        if (msg_len < sizeof(ProtocolHead)) {
            return MSG_LEN_STATUS_ERR;
        }

        if (msg_len > len) {
            return MSG_LEN_STATUS_NOT_COMPLETE;
        }

        packageSize = msg_len;
        
        return MSG_LEN_STATUS_OK;
    }
};

using VoidPtr = std::shared_ptr<void>;

class Protocol {
public:
    virtual ~Protocol() = default;
    
    virtual bool parseToMessage(const char* package, uint32_t packageSize, 
                                uint32_t protocolUri, VoidPtr& message) = 0;

    virtual bool serializeToString(VoidPtr& message, std::string& data) = 0;

    virtual bool dispatch(const char* package, uint32_t packageSize, 
                          uint32_t protocolUri, Connection* conn) = 0;
    
    virtual VoidPtr getDispatcher() = 0;
};

} // namespace tinyrpc
#endif // __PROTOCOL_H__
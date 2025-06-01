// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __PROTOCOL_PB_H__
#define __PROTOCOL_PB_H__


#include "dispatcher.h"
#include "protocol.h"
#include "log.h"
#include <cassert>
#include <functional>


namespace tinyrpc {
namespace pb {

class PbProtocol : public Protocol {
public:
    using MessagePtr = std::shared_ptr<google::protobuf::Message>;
    
    PbProtocol();
    virtual ~PbProtocol() = default;
    
    virtual bool parseToMessage(const char* data, uint32_t len, 
                                uint32_t protocolUri, VoidPtr& message) override;

    virtual bool serializeToString(VoidPtr& message, std::string& data) override;

    virtual bool dispatch(const char* data, uint32_t len, 
                          uint32_t protocolUri, Connection* conn) override;
    
    virtual VoidPtr getDispatcher() override;

private:

    std::shared_ptr<Dispatcher> dispatcher_;
};

} // namespace tiynrpc
} // pb
#endif // __PROTOCOL_PB_H__
// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __PROTOCOL_CC_H__
#define __PROTOCOL_CC_H__

#include "serialize.h"
#include "dispatcher.h"
#include "protocol.h"
#include "log.h"
#include "dispatcher_base/protocol_traits.h"
#include <cassert>
#include <functional>


namespace tinyrpc {
namespace cc {

class CcProtocol : public Protocol {
public:
    using MessagePtr = std::shared_ptr<Serializable>;

    CcProtocol();
    virtual ~CcProtocol() = default;
    
    virtual bool parseToMessage(const char* data, 
                                uint32_t len, 
                                uint32_t protocolUri, 
                                VoidPtr& message) override;

    virtual bool serializeToString(VoidPtr& message, 
                                   std::string& data) override;

    virtual bool dispatch(const char* data, uint32_t len, 
                          uint32_t protocolUri, Connection* conn) override;
    
    virtual VoidPtr getDispatcher() override;

private:

    std::shared_ptr<Dispatcher> dispatcher_;
};

} // namespace tiynrpc
} // cc
#endif // __PROTOCOL_CC_H__
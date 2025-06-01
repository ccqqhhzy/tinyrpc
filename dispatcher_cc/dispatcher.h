// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __DISPATCHER_CC_H__
#define __DISPATCHER_CC_H__

#include <memory>

#include "dispatcher_base/dispatcher_base.h"

namespace tinyrpc {
namespace cc {


using MessagePtr = std::shared_ptr<Serializable>;

class Dispatcher : public detail::GenericDispatcher<detail::CcProtocol, Dispatcher> {
public:
    
    MessagePtr createMessage(uint32_t protocolUri) override;

    template <typename T>
    DescriptorPtr getDescriptor() {
        return std::make_shared<T>();
    }
};

} // namespace cc
} // namespace tinyrpc
#endif // __DISPATCHER_CC_H__
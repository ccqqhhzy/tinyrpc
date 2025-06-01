// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __DISPATCHER_PB_H__
#define __DISPATCHER_PB_H__

#include <memory>

#include "dispatcher_base/dispatcher_base.h"
#include <google/protobuf/message.h>

namespace tinyrpc {
namespace pb {


using MessagePtr = std::shared_ptr<google::protobuf::Message>;

class Dispatcher : public detail::GenericDispatcher<detail::PbProtocol, Dispatcher> {
public:

    MessagePtr createMessage(uint32_t protocolUri) override;

    template <typename T>
    DescriptorPtr getDescriptor() {
        return T::descriptor();
    }
};

} // namespace pb
} // namespace tinyrpc
#endif //  __DISPATCHER_PB_H__
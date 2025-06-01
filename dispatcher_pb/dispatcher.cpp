// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

// file: dispatcher_pb/dispatcher.cpp
#include "dispatcher.h"
#include <google/protobuf/descriptor.h>

using namespace tinyrpc;
using namespace pb;

MessagePtr Dispatcher::createMessage(uint32_t protocolUri) {
    // create obj base on Descriptor
    auto it = descriptor_.find(protocolUri);
    if (it == descriptor_.end()) {
        LOG(Error, "no DescriptorPtr! protocolUri:0x%xu", protocolUri);
        return nullptr;
    }

    const google::protobuf::Descriptor* descriptor = it->second;
    if (descriptor) {
        const google::protobuf::Message* prototype =
          google::protobuf::MessageFactory::generated_factory()->
                GetPrototype(descriptor);
        if (prototype) {
            google::protobuf::Message* message = prototype->New();
            MessagePtr mes(message);
            return mes;
        }
    } else {
        LOG(Error, "getDescriptor failed! protocolUri:0x%xu", protocolUri);
    }
 
    return nullptr;
}
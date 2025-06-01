// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

// file: dispatcher_cc/dispatcher.cpp
#include "dispatcher.h"
#include "serialize.h"

using namespace tinyrpc;
using namespace cc;


MessagePtr Dispatcher::createMessage(uint32_t protocolUri) {
    // 基于 Descriptor 创建实例
    auto it = descriptor_.find(protocolUri);
    if (it != descriptor_.end()) {
        // 拷贝构造新的对象：避免复用已有对象去反序列化，数据会堆积到已有对象
        return it->second->clone();
    } else {
        LOG(Error, "no MessagePtr! protocolUri:%u", protocolUri);
    }

    return nullptr;
}
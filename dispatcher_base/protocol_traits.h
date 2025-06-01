// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __PROTOCOL_TRAITS_H__
#define __PROTOCOL_TRAITS_H__


#include "dispatcher_cc/serialize.h"
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <memory>

namespace tinyrpc {
namespace detail {

// 协议标签
struct PbProtocol {};
struct CcProtocol {};

// 协议元信息 traits
template<typename ProtocolTag>
struct ProtocolTraits;

// pb 协议 traits
template<>
struct ProtocolTraits<PbProtocol> {
    using MessageType = google::protobuf::Message;
    using DescriptorPtr = const google::protobuf::Descriptor*;
    static const char* name() { return "pb"; }
};

// cc 协议 traits
template<>
struct ProtocolTraits<CcProtocol> {
    using MessageType = cc::Serializable;
    using DescriptorPtr = std::shared_ptr<cc::Serializable>;
    static const char* name() { return "cc"; }
};

using VoidPtr = std::shared_ptr<void>;

} // namespace detail
} // namespace tinyrpc

#endif // __PROTOCOL_TRAITS_H__
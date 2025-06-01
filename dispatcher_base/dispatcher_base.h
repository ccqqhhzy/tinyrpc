// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __DISPATCHER_BASE_H__
#define __DISPATCHER_BASE_H__

#include <string>

#include "protocol_traits.h"
#include "callback.h"
#include "log.h"
#include <unordered_map>
#include <functional>
#include <memory>


namespace tinyrpc {
namespace detail {

template<typename PROTOCOL, typename DISPATCHER>
class GenericDispatcher {
public:
    using Traits = detail::ProtocolTraits<PROTOCOL>;
    using MessageType = typename Traits::MessageType;
    using DescriptorPtr = typename Traits::DescriptorPtr;
    using MessagePtr = std::shared_ptr<MessageType>;
    using CallbackPtr = std::shared_ptr<ICallback>;

    using DescriptorMap = std::unordered_map<uint32_t, DescriptorPtr>;
    using CallbackMap = std::unordered_map<uint32_t, CallbackPtr>;
    using Req2RspMap = std::unordered_map<uint32_t, uint32_t>;

    virtual ~GenericDispatcher() = default;

   template<typename REQ, typename RSP>
    void registerCallback(std::function<void(const std::shared_ptr<REQ>&,
                                const std::shared_ptr<RSP>&)> callback) {
        auto cb = std::make_shared<Callback<PROTOCOL, REQ, RSP>>();
        cb->setServerRequest(callback);
        callback_[REQ::URI] = cb;
        req2rsp_[REQ::URI] = RSP::URI;
        descriptor_[REQ::URI] = static_cast<DISPATCHER*>(this)->template 
            getDescriptor<REQ>();
        descriptor_[RSP::URI] = static_cast<DISPATCHER*>(this)->template
            getDescriptor<RSP>();

        assert(REQ::URI - RSP::URI != 0);
        LOG(Info, "%s::registerCallback! req uri:0x%xu, rsp uri:0x%xu",
            Traits::name(), REQ::URI, RSP::URI);
    }

    template<typename REQ, typename RSP>
    void registerCallback(std::function<void(
                                const std::shared_ptr<RSP>&)> callback) {
        auto cb = std::make_shared<Callback<PROTOCOL, REQ, RSP>>();
        cb->setAsyncCallback(callback);
        callback_[RSP::URI] = cb;
        req2rsp_[RSP::URI] = RSP::URI;
        descriptor_[RSP::URI] = static_cast<DISPATCHER*>(this)
            ->template getDescriptor<RSP>();
        LOG(Info, "%s::registerCallback! rsp uri:0x%xu", 
            Traits::name(), RSP::URI);
    }

    template<typename T>
    void registerDescriptor()
    {
        if (!checkProtocolUri(T::URI)) {
            descriptor_[T::URI] = static_cast<DISPATCHER*>(this)
                ->template getDescriptor<T>();
        }
    }

    bool onServerRequest(uint32_t reqUri, const MessagePtr& req, 
                         const VoidPtr& rsp) {
        auto it = callback_.find(reqUri);
        if (it != callback_.end()) {
            it->second->onServerRequest(req, rsp);
            return true;
        }
        LOG(Error, "no callback! reqUri:0x%xu", reqUri);
        return false;
    }

    bool onAsyncResponse(uint32_t rspUri, const MessagePtr& rsp) {
        auto it = callback_.find(rspUri);
        if (it != callback_.end()) {
            it->second->onAsyncResponse(rsp);
            return true;
        }
        LOG(Error, "no callback! rspUri:0x%xu", rspUri);
        return false;
    }

    bool checkProtocolUri(uint32_t protocolUri) const {
        return descriptor_.find(protocolUri) != descriptor_.end();
    }

    uint32_t getRspUri(uint32_t reqUri) const {
        auto it = req2rsp_.find(reqUri);
        return it != req2rsp_.end() ? it->second : 0;
    }

    virtual MessagePtr createMessage(uint32_t protocolUri) = 0;


    template <typename T>
    DescriptorPtr getDescriptor();

protected:

    CallbackMap callback_;
    Req2RspMap req2rsp_;
    DescriptorMap descriptor_; // 基于 Descriptor 创建 MessagePtr
};

} // namespace detail
} // namespace tinyrpc
#endif //  __DISPATCHER_BASE_H__
// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __CALLBACK_H__
#define __CALLBACK_H__


#include "protocol_traits.h"
#include  "callback_base.h"
#include <memory>
#include <functional>

namespace tinyrpc {
namespace detail {


template<typename Protocol, typename REQ, typename RSP>
class Callback : public ICallback {
public:
    using Traits = detail::ProtocolTraits<Protocol>;
    using MessageType = typename Traits::MessageType;
    using MessagePtr = std::shared_ptr<MessageType>;

    using ServerRequest = std::function<void (const std::shared_ptr<REQ>&,
        const std::shared_ptr<RSP>&)>;

    using AsyncCallback = std::function<void (const std::shared_ptr<RSP>&)>;

    explicit Callback() = default;

    void setServerRequest(const std::function<void (const std::shared_ptr<REQ>&,
                                            const std::shared_ptr<RSP>&)> cb) {
        callback_ = cb;
    }

    void setAsyncCallback(const std::function<void (
                                const std::shared_ptr<RSP>&)>& cb) {
        asyncCallback_ = cb;
    }

    void onServerRequest(const VoidPtr& req, const VoidPtr& rsp) const {
        auto concreteReq = down_pointer_cast<REQ>(req);
        auto concreteRsp = down_pointer_cast<RSP>(rsp);
        assert(concreteReq && concreteRsp);
        callback_(concreteReq, concreteRsp);
    }

    void onAsyncResponse(const VoidPtr& rsp) const {
        auto concreteRsp = down_pointer_cast<RSP>(rsp);
        assert(concreteRsp);
        asyncCallback_(concreteRsp);
    }

private:
    ServerRequest callback_;
    AsyncCallback asyncCallback_;
};

} // namespace detail
} // namespace tinyrpc

#endif // __CALLBACK_H__
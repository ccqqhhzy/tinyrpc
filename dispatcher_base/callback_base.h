// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __CALLBACK_BASE_H__
#define __CALLBACK_BASE_H__


#include "protocol_traits.h"
#include <functional>
#include <memory>


namespace tinyrpc {
namespace detail {


template<typename To, typename From>
inline To implicit_cast(From const &f) {
    return f;
}

template<typename To, typename From>
inline std::shared_ptr<To> down_pointer_cast(const std::shared_ptr<From>& f) {
    if (false) {
        implicit_cast<From*, To*>(0);
    }

#ifndef NDEBUG
    //assert(f == NULL || dynamic_cast<To*>(f.get()) != NULL);
#endif
    return std::static_pointer_cast<To>(f);
}


class ICallback {
public:
    virtual ~ICallback() = default;
    // server-side request callback
    virtual void onServerRequest(const VoidPtr& req, 
                                 const VoidPtr& rsp) const = 0;
    // client-side async response callback
    virtual void onAsyncResponse(const VoidPtr& rsp) const = 0;
};

} // namespace detail
} // namespace tinyrpc

#endif // __CALLBACK_BASE_H__
// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __STOPPABLE_H__
#define __STOPPABLE_H__


#include <chrono>
#include <future>

namespace tinyrpc
{

class Stoppable 
{
public:
    Stoppable() : futureObj_(exitSignal_.get_future()) {}
    
    Stoppable(Stoppable && obj) 
        : exitSignal_(std::move(obj.exitSignal_))
        , futureObj_(std::move(obj.futureObj_)) {}

    Stoppable & operator = (Stoppable && obj) {
        exitSignal_ = std::move(obj.exitSignal_);
        futureObj_ = std::move(obj.futureObj_);
        return *this;
    }

    virtual void run() = 0;

    // Thread function to be executed by thread
    void operator() () {
        run();
    }

    bool stopRequested() {
        // checks if value in future object is available
        if (futureObj_.wait_for(std::chrono::milliseconds(0))
                 == std::future_status::timeout) {
            return false;
        }
        return true;
    }

    void stop() {
        exitSignal_.set_value();
    }

private:
    std::promise<void> exitSignal_;
    std::future<void> futureObj_;
};

} // namespace
#endif // __STOPPABLE_H__
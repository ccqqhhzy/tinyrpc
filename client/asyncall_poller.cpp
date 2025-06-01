// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include "asyncall_poller.h"

using namespace tinyrpc;

AsynCallPoller::~AsynCallPoller() {
    poller_->stop();
    this->stop();

    if (thread_.joinable()) {
        thread_.join();
    }
}

void AsynCallPoller::init() {
    assert(poller_);
    poller_->setTimeout(100);
}

void AsynCallPoller::run() {
    while (!stopRequested()) {
        poller_->runLoop(); // once poll
        break;
    }
}

void AsynCallPoller::start() {
    thread_ = std::thread( [&]() { this->run(); } );
}

// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __ASYNCALL_POLLER_H__
#define __ASYNCALL_POLLER_H__

#include "connection.h"
#include "stoppable.h"
#include "poller.h"
#include "client_rpc.h"
#include <memory>
#include <string.h>
#include <functional>


namespace tinyrpc {

class AsynCallPoller : public Stoppable {
public:
    static AsynCallPoller & getInstance() {
        static AsynCallPoller instance;
        return instance;
    }

    ~AsynCallPoller();

    void init();

    virtual void run() override;

    void start();

    template<typename CLIENT>
    bool addFd(int fd, const std::shared_ptr<RpcClient<CLIENT>>& client);

private:
    AsynCallPoller() : poller_(new Poller(Poller::MAX_FD)) {}

    template<typename CLIENT>
    void onReadEvent(int fd, int events, 
                     const std::shared_ptr<RpcClient<CLIENT>>& client);

    std::shared_ptr<Poller> poller_;

    std::thread thread_;
};

template<typename CLIENT>
bool AsynCallPoller::addFd(int fd, 
                           const std::shared_ptr<RpcClient<CLIENT>>& client) {
    poller_->setFdReadCallback(fd, 
        [client, this] (int fd, int events, void* arg) {
            this->onReadEvent(fd, events, client);
        }, 
        client.get());
    return poller_->addFd(fd, EV_READ);
}

template<typename CLIENT>
void AsynCallPoller::onReadEvent(int fd, int events, 
        const std::shared_ptr<RpcClient<CLIENT>>& client) {
    if (!client->conn_) {
        LOG(Error, "conn is null,fd:%d", fd);
        return;
    }

    if (client->conn_->tcpRecv()) {
        client->codec_->processMessage(client->conn_.get());
    }

    if (client->conn_->getStatus() == CONN_STATUS_BROKEN) {
        poller_->delFd(fd);
        close(fd);
        client->conn_->reset();
        LOG(Info, "sock broken! delFd fd:%d,client ip:%s,port:%u",
            fd, client->conn_->getRemoteAddr()->ip, 
            client->conn_->getRemoteAddr()->port);
    }
}

} // namespace tinyrpc

#endif // __ASYNCALL_POLLER_H__
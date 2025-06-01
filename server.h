// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __SERVER_H__
#define __SERVER_H__

#include "codec.h"
#include "poller.h"
#include "option.h"
#include "log.h"
#include "objectpool.h"
#include <vector>
#include <unordered_map>

namespace tinyrpc {

typedef struct Process {
    pid_t pid;
    // socketpair for IPC between parent and child
    int pipefd[2];
    Process() : pid(-1) {}
} Process;

class Server {
public:
    enum {
        PROCESS_MAXNUM = 32,
    };

    explicit Server(const std::vector<option>& vecOpt);
    ~Server();

    void run(void);

    template<typename REQ, typename RSP>
    bool pbRegisterCallback(std::function<void(const std::shared_ptr<REQ>&,
                            const std::shared_ptr<RSP>&)> callback) {
        std::shared_ptr<Protocol> protocol = codec_->getProtocol(PROTOCOL_TYPE_PB);
        if (!protocol) {
            LOG(Error, "pbRegisterCallback protocol type error");
            return false;
        }

        auto dispatcher = std::static_pointer_cast<pb::Dispatcher>(
                protocol->getDispatcher());
        dispatcher->registerCallback<REQ,RSP>(callback);
        LOG(Info, "pbRegisterCallback reqUri:0x%xu", REQ::URI);

        return true;
    }

    template<typename REQ, typename RSP>
    bool ccRegisterCallback(std::function<void(const std::shared_ptr<REQ>&,
                            const std::shared_ptr<RSP>&)> callback) {
        std::shared_ptr<Protocol> protocol = codec_->getProtocol(PROTOCOL_TYPE_CC);
        if (!protocol) {
            LOG(Error, "ccRegisterCallback protocol type error");
            return false;
        }

        auto dispatcher = std::static_pointer_cast<cc::Dispatcher>(
                protocol->getDispatcher());
        dispatcher->registerCallback<REQ,RSP>(callback);
        LOG(Info, "ccRegisterCallback reqUri:0x%xu", REQ::URI);

        return true;
    }

private:
    bool listenOnAddress(int family, const char* ip, uint16_t port, 
                         int& listenfd);
    bool isWorker() const { return workerIndex_ >= 0; }
    //void clearConn(std::unique_ptr<Connection>& conn);
    auto clearConnAndEraseFromConnMap(const std::shared_ptr<Connection>& conn);
    
    void onSigPipeFdOfWatcher(int fd, int events, void* arg);
    void onSigPipeFdOfWorker(int fd, int events, void* arg);
    void onPipeFdOfWorker(int fd, int events, void* arg);
    void onAccept(int fd, int events, void* arg);
    void onRead(int fd, int events, const std::shared_ptr<Connection>& conn);
    void onWrite(int fd, int events, const std::shared_ptr<Connection>& conn);
    void checkIdleConnections(int fd, int events, void* arg);

    // run on child process
    void workerRun(void);
    // run on parent process
    void watcherRun(void);

    ServerOptions opt_;

    // worker process(child process)
    Process* workerList_;
    int workerNum_;
    // index of parent:-1, index of children:[0,n]
    int workerIndex_; 

    // worker process accept conn from nonblock listenfd_ 
    int listenfd_;
    // max fd accepted from listenfd_
    int maxfd_;

    Poller* poller_;   
    Codec* codec_;

    ObjectPool<Connection> connPool_;
    std::unordered_map<int, std::shared_ptr<Connection>> connMap_;
};


} // namespace tinyrpc
#endif //__SERVER_H__

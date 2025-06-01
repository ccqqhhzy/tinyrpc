// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include "client/client_cc.h"
#include "client/asyncall_poller.h"

#include "proto_cc/book.h"
#include <iostream>

using namespace std;
using namespace tinyrpc;
using namespace cc;


void sendBookReq(const ClientOptions& opt) {
    CcClient client(opt);
    if (!client.isOk()) {
        cout << "connect failed!" << endl;
        return;
    }

    BookReq req;
    std::shared_ptr<BookRsp> rsp;

    req.name = "jesse";
    req.age = 26;
    req.book.push_back("aaa");
    req.book.push_back("bbb");
    map<string,string> vs;
    vs["hello"] = "world";
    req.extend.push_back(vs);

    if (!client.synCall<BookReq, BookRsp>(req, rsp, 3000)) {
        cout << "sysCall failed!" << endl;
        return;
    }
    cout << "rsp:" << rsp->dump() << endl;

    req.name = "white";
    req.age = 50;
    req.book.push_back("ccc");
    req.book.push_back("ddd");
    cout << "req:" << req.dump() << endl;

    if (!client.synCall<BookReq, BookRsp>(req, rsp)) {
        cout << "sysCall failed!" << endl;
        return;
    }

    cout << "rsp:" << rsp->dump() << endl;

    req.name = "joe";
    req.age = 50;
    req.book.push_back("eee");
    req.book.push_back("fff");
    cout << "req:" << req.dump() << endl;

    if (!client.synCall<BookReq, BookRsp>(req, rsp)) {
        cout << "sysCall failed!" << endl;
        return;
    }

    cout << "rsp:" << rsp->dump() << endl;
}

static int iSuccCnt = 0;
static int iFailCnt = 0;
static int iTime = 0;

void AddSuccCnt() {
    int now = time(NULL);
    if (now >iTime) {
        cout << "time:" << iTime << "Succ Cnt:" << iSuccCnt 
            << " Fail Cnt:" << iFailCnt << endl;
        iTime = now;
        iSuccCnt = 0;
        iFailCnt = 0;
    } else {
        iSuccCnt++;
    }
}
void AddFailCnt() {
    int now = time(NULL);
    if (now >iTime) {
        cout << "time:" << iTime << "Succ Cnt:" << iSuccCnt 
            << " Fail Cnt:" << iFailCnt << endl;
        iTime = now;
        iSuccCnt = 0;
        iFailCnt = 0;
    } else {
        iFailCnt++;
    }
}

void* th_func(void* arg) {
    ClientOptions* options = static_cast<ClientOptions*>(arg);
    CcClient client(*options);
    if (!client.isOk()) {
        cout << "connect failed!" << endl;
        pthread_exit((void*)0);
    }

    BookReq req;
    std::shared_ptr<BookRsp> rsp;

    req.name = "jesse";
    req.age = 26;
    req.book.push_back("aaa");
    req.book.push_back("bbb");
    map<string,string> vs;
    vs["hello"] = "world";
    req.extend.push_back(vs);

    //for (int i = 0; i < 20000; ++i) {
    for (;;) {
        if (client.synCall<BookReq, BookRsp>(req, rsp)) {
            AddSuccCnt();
        } else {
            AddFailCnt();
        }
        //sleep(2);
    }

    return (void*)0;
}

void sendMultiReq(int threadNum, const ClientOptions& opt) {
    pthread_t *thid = (pthread_t*)malloc(threadNum * sizeof(pthread_t));

    for (int i = 0; i < threadNum; i++) {
        int ret = pthread_create(&thid[i], NULL, th_func, (void*)(&opt));
        if (0 != ret) {
            cout << "pthread_create failed" << endl;
            return;
        }
    }

    int retval = -1;
    for (int i = 0; i < threadNum; i++) {
        int ret = pthread_join(thid[i], (void**)(&retval));
        if(0 != ret) {
            if(EINVAL == ret) {
                cout << "ret:EINVAL" << endl;
            } else if(ESRCH == ret) {
                cout << "ret:ESRCH" << endl;
            } else {
                cout << "ret:EDEADLK" << endl;
            }
        }
    }

    if (NULL != thid) {
        free(thid);
        thid = NULL;
    }
}

///////////////////////////////////////////////////////////

typedef std::shared_ptr<BookRsp> BookRspPtr;

void onBookRsp(const BookRspPtr& rsp) {
    if (!rsp) {
        std::cout << "onHelloReq: message is NULL" << std::endl;
        return;
    }

    std::cout << "asyncall rsp info:" << rsp->dump() << std::endl;
}

void testAsynCall(ClientOptions& opt) {
    AsynCallPoller::getInstance().init();
    AsynCallPoller::getInstance().start();

    // 异步调用
    opt.isAsync = true;

    std::shared_ptr<RpcClient<CcClient>> client(new CcClient(opt));
    if (!client->isOk()) {
        cout << "connect failed!" << endl;
        return;
    }

    client->registerCallback<BookReq,BookRsp>(std::bind(onBookRsp, std::placeholders::_1));

    BookReq req;
    std::shared_ptr<BookRsp> rsp;

    req.name = "jesse";
    req.age = 26;
    req.book.push_back("aaa");
    req.book.push_back("bbb");
    map<string,string> vs;
    vs["hello"] = "world";
    req.extend.push_back(vs);

    if (!client->asynCall<BookReq>(req)) {
        cout << "asynCall failed!" << endl;
        return;
    }

    req.name = "white";
    req.age = 50;
    
    for (int i = 0; i < 1; i++) {
        if (!client->asynCall<BookReq>(req)) {
            cout << "fail..." << endl;
            break;
        }
    }

    cout << "asynCall ..." << endl;
    
    sleep(5);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cout << "usage:" << argv[0] << " [thread num] [ip] [port]" << endl;
        return -1;
    }

    int threadNum = atoi(argv[1]);
    string ip = argv[2];
    int port = atoi(argv[3]);
    
    ServiceAddrOption serviceAddrOption;
    serviceAddrOption.ip_ = ip;
    serviceAddrOption.port_ = port;

    ClientOptions opt;
    opt.setServiceAddrOption(serviceAddrOption);

    sendBookReq(opt);

    testAsynCall(opt);

    sendMultiReq(threadNum, opt);

    return 0;
}

/*

valgrind --tool=memcheck --leak-check=full --track-origins=yes ./exe_client_pb_test 1 ::1 8900

*/
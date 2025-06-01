// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include "client/client_pb.h"
#include "client/asyncall_poller.h"
#include "proto_pb/echo.pb.h"
#include "proto_pb/hello.pb.h"
#include <iostream>
#include <chrono>

using namespace std;
using namespace tinyrpc;
using namespace cc;
using namespace google::protobuf;
using namespace echo_proto;
using namespace hello_proto;


void sendEchoReq(const ClientOptions& opt) {
    PbClient client(opt);
    if (!client.isOk()) {
        cout << "connect failed!" << endl;
        return;
    }

    EchoReq req;
    std::shared_ptr<EchoRsp> rsp;

    req.set_sid("abc");
    req.set_loginid(1);
    req.set_info("hello");

    // 同步调用超时场景注意：
    // 1.控制重试次数；
    // 2.请求需要唯一的reqId，幂等性检查避免重复操作
    // 3.reqId校验，重试时避免旧响应冲突，覆盖新的响应
    int retryCount = 0;
    const int maxRetries = 3;
    while (retryCount++ < maxRetries) {
        bool succ = client.synCall<EchoReq, EchoRsp>(req, rsp, 3000);
        if (succ) {
            cout << "sendEchoReq info:" << rsp->info() << endl;
            break;
        }
        cout << "sendEchoReq sysCall failed!" << endl;
        // 退避
        std::this_thread::sleep_for(std::chrono::milliseconds(100 * retryCount));
    }
    cout << "sendEchoReq info:" << rsp->info() << endl;

    req.set_sid("efg");
    req.set_loginid(2);
    req.set_info("hahahahahhahahhahahahhahah");

    if (!client.synCall<EchoReq, EchoRsp>(req, rsp)) {
        cout << "sendEchoReq sysCall failed!" << endl;
        return;
    }

    cout << "sendEchoReq info:" << rsp->info() << endl;
}

void sendHelloReq(const ClientOptions& opt) {
    PbClient client(opt);
    if (!client.isOk()) {
        cout << "sendHelloReq connect failed!" << endl;
        return;
    }

    HelloReq req;
    std::shared_ptr<HelloRsp> rsp;

    req.set_sid("abc");
    req.set_loginid(1);
    req.set_info("hello");

    if (!client.synCall<HelloReq, HelloRsp>(req, rsp)) {
        cout << "sendHelloReq sysCall failed!" << endl;
        return;
    }
    cout << "sendHelloReq info:" << rsp->info() << endl;

    req.set_sid("efg");
    req.set_loginid(2);
    req.set_info("hahahahahhahahhahahahhahah");

    if (!client.synCall<HelloReq, HelloRsp>(req, rsp)) {
        cout << "sendHelloReq sysCall failed!" << endl;
        return;
    }

    cout << "sendHelloReq info:" << rsp->info() << endl;
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

void* th_func(void *arg) {
    ClientOptions* options = static_cast<ClientOptions*>(arg);
    PbClient client(*options);
    if (!client.isOk()) {
        cout << "connect failed!" << endl;
        pthread_exit((void*)0);
    }

    EchoReq req;
    std::shared_ptr<EchoRsp> rsp;
    req.set_sid("1111111111");
    req.set_info("hello");

    int i = 1;
    for (;;) {
        req.set_loginid(i++);
        if (client.synCall<EchoReq, EchoRsp>(req, rsp)) {
            //std::cout << "client ok" << std::endl;
            AddSuccCnt();
        } else {
            AddFailCnt();
            break;
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

using EchoReqPtr = std::shared_ptr<EchoReq>;
using EchoRspPtr = std::shared_ptr<EchoRsp>;
void onEchoRsp(const EchoRspPtr &rsp) {
    if (NULL == rsp) {
        //std::cout << "onEchoRsp: message is NULL" << std::endl;
        AddFailCnt();
	    return;
    }
    AddSuccCnt();
    cout << "asyncall rsp info:" << rsp->info() << endl;
}


void testAsynCall(ClientOptions& opt) {
    AsynCallPoller::getInstance().init();
    AsynCallPoller::getInstance().start();

    // 异步调用
    opt.isAsync = true;

    std::shared_ptr<RpcClient<PbClient>> client(new PbClient(opt));
    if (!client->isOk()) {
        cout << "connect failed!" << endl;
        return;
    }

    client->registerCallback<EchoReq,EchoRsp>(std::bind(onEchoRsp, std::placeholders::_1));

    EchoReq req;
    req.set_sid("1111111111");
    req.set_loginid(1);
    req.set_info("hello");
    client->asynCall<EchoReq>(req);

    req.set_loginid(2);
    req.set_info("abc123");
    
    for (int i = 0; i < 1; i++) {
        if (!client->asynCall<EchoReq>(req)) {
            cout << "fail..." << endl;
            break;
        }
    }

    cout << "asynCall ..." << endl;
    
    // avoid exit
    sleep(5);
}


void clearConnection_test(const ClientOptions& opt) {
    PbClient client1(opt);
    if (!client1.isOk()) {
        cout << "connect1 failed!" << endl;
        return;
    }

    PbClient client2(opt);
    if (!client2.isOk()) {
        cout << "connect2 failed!" << endl;
        return;
    }

    PbClient client3(opt);
    if (!client3.isOk()) {
        cout << "connect3 failed!" << endl;
        return;
    }

    sleep(10);

    HelloReq req;
    std::shared_ptr<HelloRsp> rsp;
    req.set_sid("abc");

    if (!client1.synCall<HelloReq, HelloRsp>(req, rsp)) {
        cout << "client1 sysCall failed!" << endl;
    }
    if (!client2.synCall<HelloReq, HelloRsp>(req, rsp)) {
        cout << "client2 sysCall failed!" << endl;
    }
    if (!client3.synCall<HelloReq, HelloRsp>(req, rsp)) {
        cout << "client3 sysCall failed!" << endl;
    }
}

int main(int argc, char *argv[]) {
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
    opt.connectTimeoutMs = 3000;
    opt.setServiceAddrOption(serviceAddrOption);

    sendEchoReq(opt);
    sendHelloReq(opt);

    testAsynCall(opt);

    std::cout << "##### clearConnection_test #####" << std::endl;
    clearConnection_test(opt);

    sendMultiReq(threadNum, opt);
   
    return 0;
}

/*

valgrind --tool=memcheck --leak-check=full --track-origins=yes ./exe_client_pb_test 1 ::1 8900


server:
$ ./exe_server_test 1 ::1 8900
workerNum:1

client:
$ ./exe_client_pb_test 4 ::1 8900


*/
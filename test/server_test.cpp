// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include "connection.h"
#include "server.h"
#include "log.h"
#include "option.h"
#include "proto_pb/echo.pb.h"
#include "proto_pb/hello.pb.h"
#include "proto_cc/book.h"
#include <iostream>
#include <cstdlib>
#include <cstdio>


using namespace google::protobuf;
using namespace echo_proto;
using namespace hello_proto;

using namespace std;
using namespace tinyrpc;
using namespace cc;



using EchoReqPtr = std::shared_ptr<EchoReq>;
using EchoRspPtr = std::shared_ptr<EchoRsp>;

class EchoService {
public:
    static EchoService& getInstance() {
    	static EchoService instance;
    	return instance;
    }

    void onEchoReq(const EchoReqPtr& req, const EchoRspPtr& rsp) {
        if (!req || !rsp) {
            std::cout << "onEchoReq: req is NULL" << std::endl;
		    return;
	    }

        rsp->set_retcode(1);
        if (req->loginid() == 1) {
            rsp->set_info("world");
        } else {
            rsp->set_info("happy");
        }
        
        rsp->set_sid(req->sid());
        rsp->set_loginid(req->loginid());
    }

private:
	EchoService() {}
};

///////////////////////////////////////////////////////////

using HelloReqPtr = std::shared_ptr<HelloReq>;
using HelloRspPtr = std::shared_ptr<HelloRsp>;

class HelloService {
public:
    static HelloService& getInstance() {
    	static HelloService instance;
    	return instance;
    }

    void onHelloReq(const HelloReqPtr& req, const HelloRspPtr& rsp) {
        if (!req || !rsp) {
            std::cout << "onHelloReq: req is NULL" << std::endl;
		    return;
	    }

        rsp->set_retcode(1);
        if (req->loginid() == 1) {
            rsp->set_info("you good");
        } else {
            rsp->set_info("you good good");
        }
        
        rsp->set_sid(req->sid());
        rsp->set_loginid(req->loginid());
    }

private:
	HelloService() {}
};

///////////////////////////////////////////////////////////

class BookService {
public:
    using BookReqPtr = std::shared_ptr<BookReq>;
    using BookRspPtr = std::shared_ptr<BookRsp>;

    static BookService& getInstance() {
        static BookService instance;
        return instance;
    }

    void onBookReq(const BookReqPtr& req, const BookRspPtr& rsp) {
        if (!req || !rsp) {
            std::cout << "onHelloReq: message is NULL" << std::endl;
            return;
        }

        //std::cout << "req:" << message->dump() << std::endl;

        rsp->result = 0;
        rsp->extend[req->name] = std::to_string(req->age);
    }
};

///////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
    if (argc != 4) {
        cout << "usage:" << argv[0] << "[workerNum] [ip] [port]" << endl;
        return -1;
    }

    int workerNum = atoi(argv[1]);
    string ip = argv[2];
    int port = atoi(argv[3]);

    bool isIPv6 = ip.size() > 2 && ip.find(":") != string::npos ? true : false;
    cout << "workerNum:" << workerNum << endl;

    string ident = argv[0];
    Logger::getInstance().init(ident);

    ServiceAddrOption serviceAddrOption;
    serviceAddrOption.ip_ = ip;
    serviceAddrOption.port_ = port;
    serviceAddrOption.isIPv6_ = isIPv6;

    CommonOption commonOption;
    commonOption.workerNum_ = workerNum;
    commonOption.idleTimeout_ = 7;
    commonOption.maxConnNum_ = 10000;

    vector<option> vecOpt;
    vecOpt.push_back(ServerOptions::createServiceAddrOption(serviceAddrOption));
    vecOpt.push_back(ServerOptions::createCommonOption(commonOption));

    Server srv(vecOpt);

    srv.pbRegisterCallback<EchoReq,EchoRsp>(std::bind(&EchoService::onEchoReq, 
        &EchoService::getInstance(), std::placeholders::_1, std::placeholders::_2));

    srv.pbRegisterCallback<HelloReq,HelloRsp>(std::bind(&HelloService::onHelloReq, 
        &HelloService::getInstance(), std::placeholders::_1, std::placeholders::_2));

    srv.ccRegisterCallback<BookReq,BookRsp>(std::bind(&BookService::onBookReq, 
        &BookService::getInstance(), std::placeholders::_1, std::placeholders::_2));

    srv.run();
    
    return 0;
}

/*

valgrind --tool=memcheck --leak-check=full ./exe_server_test 1 ::1 8900

 ./exe_server_test 1 ::1 8900

 */

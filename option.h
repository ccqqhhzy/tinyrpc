// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __OPTION_H__
#define __OPTION_H__

#include <stdint.h>
#include <functional>
#include <vector>


namespace tinyrpc {

struct ServiceAddrOption {
    std::string ip_;
    uint32_t port_;
    bool isIPv6_;
};
    
struct CommonOption {
    uint32_t workerNum_;
    time_t idleTimeout_; // seconds
    uint32_t maxConnNum_;
};
    
class Server;
class ServerOptions;
typedef std::function<bool (ServerOptions*)> option;

class ServerOptions {
public:
    ServerOptions(const std::vector<option>& vecOpt) {
        for (auto & func : vecOpt) {
            func(this);
        }
    }
    ~ServerOptions() = default;

    
    bool setServiceAddrOption(const ServiceAddrOption& opt) {
        serviceAddrOption_ = opt;
        return true;
    }
    bool setCommonOption(const CommonOption& opt) {
        commonOption_ = opt;
        return true;
    }

    static option createServiceAddrOption(const ServiceAddrOption& a) {
        option opt = [a] (ServerOptions* opts) -> bool {
            return opts->setServiceAddrOption(a);
        };
        return opt;
    }
    
    static option createCommonOption(const CommonOption& a) {
        option opt = [a] (ServerOptions* opts) -> bool {
            return opts->setCommonOption(a);
        };
        return opt;
    }

    friend class Server;

private:
    ServiceAddrOption serviceAddrOption_;
    CommonOption commonOption_;
};
    
struct ClientOptions {
    ServiceAddrOption serviceAddrOption;
    uint32_t connectTimeoutMs;
    bool isAsync;

    ClientOptions() 
        : connectTimeoutMs(3000)
        , isAsync(false) {

    }

    ClientOptions(const ClientOptions& opt) 
        : serviceAddrOption(opt.serviceAddrOption)
        , connectTimeoutMs(opt.connectTimeoutMs)
        , isAsync(opt.isAsync) {

    }

    ClientOptions& operator = (const ClientOptions& opt) {
        if (this != &opt) {
            serviceAddrOption = opt.serviceAddrOption;
            connectTimeoutMs = opt.connectTimeoutMs;
            isAsync = opt.isAsync;
        }
        return *this;
    }

    void setServiceAddrOption(const ServiceAddrOption& opt) {
        serviceAddrOption = opt;
    }
};

} // namespace tinyrpc
#endif // __OPTION_H__
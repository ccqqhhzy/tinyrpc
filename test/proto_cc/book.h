// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __BOOK_H__
#define __BOOK_H__

#include "../../dispatcher_cc/serialize.h"
#include <memory>
#include <iostream>
#include <string.h> // strlen
#include <sstream> // stringstream

namespace tinyrpc {

struct BookReq : public cc::Serializable {
    enum Uri {
        URI = (200 << 8 | 101)
    };

    std::string name;
    uint32_t age;
    std::vector<std::string> book;
    std::list<std::map<std::string, std::string> > extend;

    virtual void serialize(cc::Payload& p) const override {
        p << name << age << book << extend;
    }

    virtual void unserialize(const cc::Payload& p) override {
        p >> name >> age >> book >> extend;
    }

    virtual std::shared_ptr<Serializable> clone() const override {
        return std::make_shared<BookReq>(*this);
    }

    template<typename T>
    std::string dumpVector(const std::vector<T>& v) const {
        std::stringstream ss;
        for (auto & i : v) {
                ss << "(" << i << ")";
        }
        return ss.str();
    }    

    std::string dumpExend() const {
        std::stringstream ss;
        for (auto & i : extend) {
            for (auto & t : i) {
                ss << "(" << t.first << "," << t.second << ")";
            }
        }
        return ss.str();
    }

    std::string dump() const {
        std::stringstream ss;
        ss << "[name:" << name
            << ",age:" << age
            << ",book:" << dumpVector(book)
            << ",extend:" << dumpExend()
            << "]";
        return ss.str();
    }
};

struct BookRsp : public cc::Serializable {
    enum Uri {
        URI = (200 << 8 | 102)
    };

    uint32_t result;
    std::map<std::string, std::string> extend;

    virtual void serialize(cc::Payload& p) const override {
        p << result << extend;
    }

    virtual void unserialize(const cc::Payload& p) override {
        p >> result >> extend;
    }

    virtual std::shared_ptr<Serializable> clone() const override {
        return std::make_shared<BookRsp>(*this);
    }

    std::string dumpExend() const {
        std::stringstream ss;
        for (auto & i : extend) {
            ss << "(" << i.first << "," << i.second << ")";
        }
        return ss.str();
    }

    std::string dump() const {
        std::stringstream ss;
        ss << "[result:" << result
            << ",extend:" << dumpExend()
            << "]";
        return ss.str();
    }
};

} // namespace tinyrpc

#endif // __BOOK_H__
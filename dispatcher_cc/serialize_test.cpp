// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include "endian_swap.h"
#include "serialize.h"
#include <iostream>
#include <string.h>
#include <sstream>
#include <memory>

using namespace tinyrpc;
using namespace cc;
using namespace std;


struct MyReq : public cc::Serializable {
    string name;
    uint32_t age;
    vector<string> book;
    list<map<string,string> > extend;

    virtual void serialize(cc::Payload& p) const override {
        p << name << age << book << extend;
    }

    virtual void unserialize(const cc::Payload& p) override {
        p >> name >> age >> book >> extend;
    }

    virtual shared_ptr<Serializable> clone() const override {
        return make_shared<MyReq>(*this);
    }

    template<typename T>
    string dumpVector(const vector<T>& v) const {
        stringstream ss;
        for (auto & i : v) {
                ss << "(" << i << ")";
        }
        return ss.str();
    }    

    string dumpExend() const {
        stringstream ss;
        for (auto & i : extend) {
            for (auto & t : i) {
                ss << "(" << t.first << "," << t.second << ")";
            }
        }
        return ss.str();
    }

    string dump() const {
        stringstream ss;
        ss << "[name:" << name
            << ",age:" << age
            << ",book:" << dumpVector(book)
            << ",extend:" << dumpExend()
            << "]";
        return ss.str();
    }
};

void testXHTON16(uint16_t h) {
    cout << "testXHTON16," << hex << "h:" << h
        << ",isBigEndian?" << isBigEndian()
        << ",htons:" << htons(h)
        << ",ntohs:" << ntohs(htons(h))
        << ",my_htons:" << my_htons(h)
        << ",my_ntohs:" << my_ntohs(my_htons(h))
        << ",XHTON16:" << XHTON16(h)
        << ",XNTOH16:" << XNTOH16(XHTON16(h))
        << endl;    
}

void testXHTON32(uint32_t h) {
    cout << "testXHTON32," << hex << "h:" << h
        << ",isBigEndian?" << isBigEndian()
        << ",htonl:" << htonl(h)
        << ",ntohl:" << ntohl(htonl(h))
        << ",my_htonl:" << my_htonl(h)
        << ",my_ntohl:" << my_ntohl(my_htonl(h))
        << ",XHTON32:" << XHTON32(h)
        << ",XNTOH32:" << XNTOH32(XHTON32(h))
        << endl;    
}

void testXHTON64(uint64_t h) {
    uint32_t h32 = (uint32_t)h;
    cout << "testXHTON64," << hex << "h:" << h << ",h32:" << h32
        << ",isBigEndian?" << isBigEndian()
        << ",htonl:" << htonl(h)
        << ",ntohl:" << ntohl(htonl(h))
        << ",XHTON64:" << XHTON64(h)
        << ",XNTOH64:" << XNTOH64(XHTON64(h))
        << endl;    
}

void testPayload() {
    Payload p;
    uint16_t u16 = 0x1111;
    uint32_t u32 = 0x2222;
    uint64_t u64 = 0x3333;
    uint64_t u64Raw = 0x12345678;
    string str = "hello world";
    pair<string, uint32_t> pairV("jesse", 0x12);
    
    map<string, uint32_t> mapV;
    mapV["jesse"] = 0x12;
    mapV["white"] = 0x34;
    map<string, uint32_t> mapV1;
    mapV1["kobe"] = 0x24;
    mapV1["curry"] = 0x30;
    list<map<string,uint32_t> > listmapV;
    listmapV.push_back(mapV);
    listmapV.push_back(mapV1);

    vector<string> vecV;
    vecV.push_back("hello");
    vecV.push_back("world");

    // 序列化
    p << u16 << u32 << u64 << str << pairV << u64Raw << mapV << listmapV << vecV;

    uint16_t u16a = 0;
    uint32_t u32a = 0;
    uint64_t u64a = 0;
    string stra;
    uint64_t u64Rawa = 0;
    pair<string, uint32_t> pairT;
    map<string, uint32_t> mapT;
    list<map<string,uint32_t> > listmapT;
    vector<string> vecT;

    // 反序列化
    p >> u16a >> u32a >> u64a >> stra >> pairT >> u64Rawa >> 
        mapT >> listmapT >> vecT;

    cout << hex << "u16a:" << u16a << ",u32a:" << u32a << ",u64a:" << u64a
        << ",stra:" << stra << ",u64Rawa:" << u64Rawa << endl;
    cout << "str:" << str << ",isEqual:" << (str == stra) 
        << ",str len:" << str.size() << ",strlen:" << strlen(str.c_str()) 
        << ",sizeof:" << sizeof(str) << ", stra len:" << stra.size() << endl;
    cout << "pairT:" << pairT.first << "," << pairT.second << endl;
    
    cout << "mapT:";  
    printContainer(cout,mapT) << endl;

    cout << "listmapT:";  
    for (auto & i : listmapT) {
        printContainer(cout,i) << endl;
    }

    cout << "vecT:";  
    printContainer(cout,vecT) << endl;

}

void testReq() {
    MyReq req;
    req.name = "kd";
    req.age = 30;
    req.book.push_back("hello");
    req.book.push_back("world");
    map<string,string> tmp;
    tmp["tool"] = "golang";
    tmp["car"] = "rr";
    req.extend.push_back(tmp);
    cout << "req:" << req.dump() << endl;

    Payload p;
    req.serialize(p);

    MyReq reqT;
    reqT.unserialize(p);
    cout << "reqT:" << reqT.dump() << endl;

    MyReq reqT2;
    reqT2.unserialize(p);
    cout << "reqT2:" << reqT2.dump() << endl;
}

int main() {
    uint16_t h16 = 0x1234;
    testXHTON16(h16);

    uint32_t h32 = 0x12345678;
    testXHTON32(h32);

    uint64_t h64 = 0x1234567812345678;
    testXHTON64(h64);

    testPayload();

    testReq();

    return 0;
}

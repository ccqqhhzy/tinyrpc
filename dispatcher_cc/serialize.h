// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

/*
 * 序列化、反序列化
 */

#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "buffer.h"
#include "endian_swap.h"
#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <memory>

namespace tinyrpc {
namespace cc {

class Payload {
public:
    explicit Payload(uint32_t buffSize = Buffer::BUF_MIN_SIZE) : buff_(buffSize) {}
    explicit Payload(char *data, uint32_t size) : buff_(data, size) {}
    virtual ~Payload() {}

    std::string getData() { return std::string(buff_.getReadPtr(), buff_.size()); }

    // 序列化
    uint16_t xhton16(uint16_t h) const { return XHTON16(h); }
    uint32_t xhton32(uint32_t h) const { return XHTON32(h); }
    uint64_t xhton64(uint64_t h) const { return XHTON64(h); }

    Payload& append(const void* v, size_t s) {
        buff_.in(static_cast<const char*>(v), s); 
        return *this;        
    }

    Payload& append_uint16(uint16_t h) {
        h = xhton16(h); 
        return append(&h, sizeof(h));
    }

    Payload& append_uint32(uint32_t h) {
        h = xhton32(h); 
        return append(&h, sizeof(h));
    }

    Payload& append_uint64(uint64_t h) {
        h = xhton64(h); 
        return append(&h, sizeof(h));
    }

    Payload& append_str(const char* v, size_t s) {
        //if (s > 0xffff) throw xxx;
        return append_uint32(static_cast<uint32_t>(s)).append(v, s);
    }

    // 反序列化逻辑
    uint16_t xntoh16(uint16_t h) const { return XNTOH16(h); }
    uint32_t xntoh32(uint32_t h) const { return XNTOH32(h); }
    uint64_t xntoh64(uint64_t h) const { return XNTOH64(h); }

    uint16_t takeout_uint16() const {
        uint16_t data = 0;
        char outbuff[sizeof(uint16_t)] = {'\0'};
        if (-1 == buff_.peek(outbuff, sizeof(data))) {
            return 0;
        }
        data = *((uint16_t*)outbuff);
        data = xntoh16(data);
        return data;
    }

    uint32_t takeout_uint32() const {
        uint32_t data = 0;
        char outbuff[sizeof(uint32_t)] = {'\0'};
        if (-1 == buff_.peek(outbuff, sizeof(data))) {
            return 0;
        }
        data = *((uint32_t*)outbuff);
        data = xntoh32(data);
        return data;
    }

    uint32_t takeout_uint64() const {
        uint64_t data = 0;
        char outbuff[sizeof(uint64_t)] = {'\0'};
        if (-1 == buff_.peek(outbuff, sizeof(data))) {
            return 0;
        }
        data = *((uint64_t*)outbuff);
        data = xntoh64(data);
        return data;
    }

    std::string takeout_string() const {
        uint32_t len = takeout_uint32();
        char outbuff[len + 1] = {'\0'};
        if (-1 == buff_.peek(outbuff, len)) {
            return "";
        }
        return std::string(outbuff);
    }

    // 数值
    Payload & operator << (uint16_t v) { append_uint16(v); return *this; }
    Payload & operator << (uint32_t v) { append_uint32(v); return *this; }
    Payload & operator << (uint64_t v) { append_uint64(v); return *this; }
    const Payload & operator >> (uint16_t& v) const { 
        v = takeout_uint16(); return *this; 
    }
    const Payload & operator >> (uint32_t& v) const { 
        v = takeout_uint32(); return *this; 
    }
    const Payload & operator >> (uint64_t& v) const { 
        v = takeout_uint64(); return *this; 
    }

    // 字符串
    Payload & operator << (const std::string& v) { 
        append_str(v.c_str(), v.size()); 
        return *this; 
    }
    const Payload & operator >> (std::string& v) const { 
        v = takeout_string(); 
        return *this; 
    }

    // pair
    template <class T1, class T2>
    Payload& operator << (const std::pair<T1, T2>& p) { 
        *this << p.first << p.second; 
        return *this; 
    }
    template <class T1, class T2>
    const Payload& operator >> (std::pair<T1, T2>& p) const {
	    *this >> p.first >> p.second;
	    return *this;
    }
    template <class T1, class T2>
    const Payload& operator >> (std::pair<const T1, T2>& p) const {
	    const T1& m = p.first;
	    T1 & m2 = const_cast<T1 &>(m);
	    *this >> m2 >> p.second;
	    return *this;
    }

    // vector
    template <class T>
    Payload& operator << (const std::vector<T>& v) { 
        serializeContainer(v); 
        return *this; 
    }
    template <class T>
    const Payload& operator >> (std::vector<T>& v) const { 
        unserializeContainer(std::back_inserter(v)); 
        return *this; 
    }

    // list
    template <class T>
    Payload& operator << (const std::list<T>& v) { 
        serializeContainer(v); 
        return *this; 
    }
    template <class T>
    const Payload& operator >> (std::list<T>& v) const { 
        unserializeContainer(std::back_inserter(v)); 
        return *this; 
    }

    // set
    template <class T>
    Payload& operator << (const std::set<T>& v) { 
        serializeContainer(v); 
        return *this; 
    }
    template <class T>
    const Payload& operator >> (std::set<T>& v) const { 
        unserializeContainer(std::inserter(v, v.begin())); 
        return *this; 
    }

    // map
    template <class T1, class T2>
    Payload& operator << (const std::map<T1, T2>& v) { 
        serializeContainer(v); 
        return *this; 
    }
    template <class T1, class T2>
    const Payload& operator >> (std::map<T1, T2>& v) const { 
        unserializeContainer(std::inserter(v, v.begin())); 
        return *this; 
    }

    // list<map<T1,T2> >
    template <class T1, class T2>
    Payload& operator << (const std::list<std::map<T1, T2> >& v) { 
        serializeCompositeContainer(v); 
        return *this; 
    }
    template <class T1, class T2>
    const Payload& operator >> (std::list<std::map<T1, T2> >& v) const { 
        unserializeCompositeContainer(std::inserter(v, v.begin())); 
        return *this; 
    }

private:
    // 容器，序列化
    template <typename ContainerClass>
    void serializeContainer(const ContainerClass & c);

    // 容器，反序列化
    template <typename OutputIterator>
    void unserializeContainer(OutputIterator i) const;

    // 复合容器，序列化，如： list< map<key_type, value_ype> > 
    // 或 list< vector<value_ype> > 等
    template <typename ContainerClass>
    void serializeCompositeContainer(const ContainerClass& v);

    // 复合容器，反序列化， 如:  list< map<key_type, value_ype> >
    template <typename OutputIterator>
    void unserializeCompositeContainer(OutputIterator i) const;

    // 复合容器，反序列化， 如: list< vector<value_ype> >
    template < typename OutputIterator >
    void unserializeCompositeArray(OutputIterator i) const;

    // 复合容器，序列化，如: map<key_type, map<key_type, value_ype> >
    template < typename ContainerClass >
    void serializeCompositeMap(const ContainerClass & v);

    // 复合容器，反序列化，如: map<key_type, map<key_type, value_ype> >
    template < typename OutputIterator >
    void unserializeCompositeMap(OutputIterator i) const;

private:
    mutable Buffer buff_;
};

// 容器，序列化
template <typename ContainerClass>
void Payload::serializeContainer(const ContainerClass & c) {
    append_uint32(static_cast<uint32_t>(c.size()));
    for (typename ContainerClass::const_iterator i = c.begin(); 
        i != c.end(); ++i) {
        *this << *i;
    }
}

// 容器，反序列化
template <typename OutputIterator>
void Payload::unserializeContainer(OutputIterator i) const {
    for (uint32_t count = takeout_uint32(); count > 0; --count) {
        typename OutputIterator::container_type::value_type tmp;
        *this >> tmp;
        *i = tmp;
        ++i;
    }
}

// 复合容器，序列化，如： list< map<key_type, value_ype> > 
// 或 list< vector<value_ype> > 等
template <typename ContainerClass>
void Payload::serializeCompositeContainer(const ContainerClass& v) {
    append_uint32(static_cast<uint32_t>(v.size()));
    for (typename ContainerClass::const_iterator i = v.begin(); 
        i != v.end(); ++i) {
        serializeContainer(*i);
    }
}

// 复合容器，反序列化， 如:  list< map<key_type, value_ype> >
template <typename OutputIterator>
void Payload::unserializeCompositeContainer(OutputIterator i) const {
   for (uint32_t count = takeout_uint32(); count > 0; --count) {
        typename OutputIterator::container_type::value_type tmp;
        unserializeContainer(std::inserter(tmp, tmp.begin())); // 使用 inserter
        *i = tmp;
        ++i;
   }
}

// 复合容器，反序列化， 如: list< vector<value_ype> >
template < typename OutputIterator >
void Payload::unserializeCompositeArray(OutputIterator i) const {
    for (uint32_t count = takeout_uint32(); count > 0; --count) {
        typename OutputIterator::container_type::value_type tmp;
        unserializeContainer(std::back_inserter(tmp)); // 使用 back_inserter
        *i = tmp;
        ++i;
    }
}

// 复合容器，序列化，如: map<key_type, map<key_type, value_ype> >
template < typename ContainerClass >
void Payload::serializeCompositeMap(const ContainerClass & v) {
    append_uint32(static_cast<uint32_t>(v.size()));
    for (typename ContainerClass::const_iterator i = v.begin(); 
        i != v.end(); ++i) {
        *this << i->first;
        serializeContainer(i->second);
    }
}

// 复合容器，反序列化，如: map<key_type, map<key_type, value_ype> >
template < typename OutputIterator >
void Payload::unserializeCompositeMap(OutputIterator i) const {
    for (uint32_t count = takeout_uint32(); count > 0; --count) {
        typename OutputIterator::container_type::value_type tmp;
        const typename OutputIterator::container_type::key_type& m = tmp.first;
        typename OutputIterator::container_type::key_type & m2 = 
            const_cast<typename OutputIterator::container_type::key_type &>(m);
        *this >> m2;
        unserializeContainer(std::inserter(tmp.second, tmp.second.begin()));
        *i = tmp;
        ++i;
    }
}

///////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////
// ostream 打印相关

template<class T1, class T2>
inline std::ostream & operator << (std::ostream & os, const std::pair<T1, T2>& v) {
    os << v.first << "," << v.second;
    return os;
}

template <typename ContainerClass>
inline std::ostream & printContainer (std::ostream & os, const ContainerClass & c) {
	for (typename ContainerClass::const_iterator i = c.begin(); i != c.end(); ++i)
		os << *i << " ";
	return os;
}

///////////////////////////////////////////////////////////

// 接口类
class Serializable {
public:
    Serializable() = default;
    virtual ~Serializable() {}
    virtual void serialize(Payload& p) const = 0;
    virtual void unserialize(const Payload& p) = 0;
    // 反序列化时并不知道确切的类型，只能通过保存的类型信息来反序列化
    virtual std::shared_ptr<Serializable> clone() const = 0;
};


} // namespace cc
} // namespace tinyrpc

#endif // SERIALIZE_H
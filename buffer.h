// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stdint.h>

namespace tinyrpc
{

/*
                size
|___________|///////////|___________|
0          index                 capacity

1.write n  (move data to left, then append new data)
            size +=n
|///////////++++++++++++|___________|
0(index=0)                       capacity

2.read n
              size -= n
|___________|///////////|___________|
0        index += n              capacity

3.write n
    size += n
|////////////+++++++++++|___________|
0(index=0)                   capacity

*/

class Buffer {
public:
    explicit Buffer(uint32_t capacity);
    ~Buffer();
    Buffer(const Buffer& buf) = delete;
    Buffer& operator = (const Buffer& buf) = delete;
    // 浅拷贝
    Buffer(char* data, uint32_t size)
        : data_(data)
        , capacity_(size)
        , size_(size)
        , index_(0)
        , isOwner_(false) {}

    uint32_t capacity() const { return capacity_; }
    uint32_t size() const { return size_; }
    uint32_t getFreeSize() const;
    bool isFull() const;

    char* getWritePtr();
    void setWriteSize(uint32_t len);

    char* getReadPtr();
    void setReadSize(uint32_t len);

    bool resize(uint32_t size);
    char* appendAt(uint32_t len);
    bool in(const char* inbuff, uint32_t len);
    bool out(char* outbuff, uint32_t len);
    int peek(char* outbuff, uint32_t len);

    void reset()
    {
        size_ = 0;
        index_ = 0;
    }

    enum {
        BUF_MIN_SIZE = 1024 * 4,
        BUF_DEFAULT_SIZE = 1024 * 16,
        BUF_MAX_SIZE = 1024 * 1024 * 16,
    };

private:
    void shrink();
    uint32_t alignToPowerOfTwo(uint32_t size);

    char* data_;
    uint32_t capacity_;
    uint32_t size_;
    uint32_t index_;
    bool isOwner_; // cc 反序列化peek数据时，设为false，表示没有所有权不能析构
};

} //namespace tinyrpc

#endif /*__BUFFER_H__*/

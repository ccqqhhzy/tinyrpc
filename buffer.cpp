// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include <string.h>
#include <assert.h>
#include "buffer.h"
#include "log.h"

using namespace std;
using namespace tinyrpc;

Buffer::Buffer(uint32_t capacity) 
    : data_(nullptr),
      capacity_(0),
      size_(0),
      index_(0),
      isOwner_(true) {
    capacity_ = (BUF_MIN_SIZE <= capacity && capacity <= BUF_MAX_SIZE) 
        ? capacity : BUF_DEFAULT_SIZE;
    data_ = new char[capacity_]();
    assert(data_ != nullptr);
}

Buffer::~Buffer() {
    if (nullptr != data_ && isOwner_) {
        delete [] data_;
        data_ = nullptr;
    }
}

inline uint32_t Buffer::getFreeSize() const {
    // 写的时候会左移让index=0，不需要减去index
    return (capacity_ - size_ /*- index_*/);
}

inline bool Buffer::isFull() const {
    return (capacity_ <= size_);
}

inline char* Buffer::getWritePtr() {
    if (nullptr == data_) {
        return nullptr;
    }

    if (0 != index_) {
        memmove(data_, data_ + index_, size_);
    }
    index_ = 0;
    return &data_[size_];
}

inline void Buffer::setWriteSize(uint32_t len) {
    if (nullptr == data_ || len <= 0) {
        return;
    }

    size_ += len;
}

inline char* Buffer::getReadPtr() {
    if (nullptr == data_) {
        return nullptr;
    }

    // 让读在index_ & 0x11 != 0时，有机会移动index_，均摊一下move的开销
    if (0 != (index_ & 3)) {
        memmove(data_, data_ + index_, size_);
        index_ = 0;
    }
    return &data_[index_];
}

inline void Buffer::setReadSize(uint32_t len) {
    if (nullptr == data_ || len == 0) {
        return;
    }

    size_ -= len;
    index_ += len;

    if (0 == size_) {
        index_ = 0;
    }

    // 缩容：当前 buffer 使用量远小于容量
    if (capacity_ > BUF_DEFAULT_SIZE && size_ * 4 < capacity_) {
        shrink();
    }
}

inline void Buffer::shrink() {
    // 如果当前 size <= BUF_DEFAULT_SIZE，则缩到默认大小
    uint32_t new_cap = (size_ <= BUF_DEFAULT_SIZE) 
        ? BUF_DEFAULT_SIZE 
        : alignToPowerOfTwo(size_);

    if (new_cap >= capacity_) {
        return; // 不需要缩容
    }

    char* new_data = new char[new_cap]();
    assert(new_data != nullptr);

    if (data_ != nullptr && size_ > 0) {
        memcpy(new_data, data_ + index_, size_);
        delete[] data_;
    }

    data_ = new_data;
    capacity_ = new_cap;
    index_ = 0;
}

inline uint32_t Buffer::alignToPowerOfTwo(uint32_t size) {
    // 将容量对齐为最近的 2 的幂次（malloc 等内存分配器更喜欢这种对齐）
    if (size < BUF_MIN_SIZE) {
        return BUF_MIN_SIZE;
    }

    // already power of two
    if ((size & (size - 1)) == 0) {
        return size;
    }

    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;

    return (size + 1);
}

bool Buffer::resize(uint32_t size) {
    if (size < BUF_MIN_SIZE || size > BUF_MAX_SIZE) {
        return false;
    }
    if (capacity_ >= size) {
        return true;
    }

    char* new_data = new char[size]();
    assert(nullptr != new_data);
    memset(new_data, 0, size);

    if (data_ != nullptr && size_ > 0) {
        memcpy(new_data, data_ + index_, size_);
        delete [] data_;
    }
    data_ = new_data;
    capacity_ = size;
    index_ = 0;
    
    return true;
}

char* Buffer::appendAt(uint32_t len) {
    while (getFreeSize() < len) {
        if (!resize(2*capacity_)) {
            return nullptr;
        }
    }

    char *start = getWritePtr();
    assert(nullptr != start); 

    setWriteSize(len);   

    return start;
}

bool Buffer::in(const char* inbuff, uint32_t len) {
    char* start = appendAt(len);
    if (nullptr == start) {
        return false; 
    }

    memcpy(start, inbuff, len);
   
    return true;
}

bool Buffer::out(char* outbuff, uint32_t len) {
    if (size_ == 0) {
        return true;
    }

    const char* start = getReadPtr();
    if (nullptr == start) {
        return false;
    }

    uint32_t outlen = (len >= size_) ? size_ : len;

    memcpy(outbuff, start, outlen);
    setReadSize(outlen);

    return true;
}

int Buffer::peek(char* outbuff, uint32_t len) {
    if (index_ + len > size_) {
        return -1;
    }

    const char* start = &data_[index_];
    if (nullptr != start) {
        memcpy(outbuff, start, len);
        index_ += len;
    } else {
        return -1;
    }
    
    return 0;
}
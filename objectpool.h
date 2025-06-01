// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __OBJECTPOOL_H__
#define __OBJECTPOOL_H__


#include <queue>
#include <memory>
#include <mutex>

namespace tinyrpc {

template<typename T>
class ObjectPool {
public:
    explicit ObjectPool(size_t size);

    std::shared_ptr<T> get();
    
    size_t size();

private:
    std::mutex mtx_;
    std::queue<std::unique_ptr<T>> pool_;
};

template<typename T>
ObjectPool<T>::ObjectPool(size_t size) {
    for (size_t i = 0; i < size; ++i) {
        pool_.emplace(new T);
    }
}

template<typename T>
std::shared_ptr<T> ObjectPool<T>::get() {
    std::lock_guard<std::mutex> lock(mtx_);
    
    if (pool_.empty()) {
        pool_.push(std::move(std::make_unique<T>()));
    }

    T* rawPtr = pool_.front().release();
    pool_.pop();
    return std::shared_ptr<T>(rawPtr, 
        [this](T* p) {
            std::unique_ptr<T> obj(p);
            std::lock_guard<std::mutex> lock(mtx_);
            pool_.push(std::move(obj));
        });
}

template<typename T>
size_t ObjectPool<T>::size() {
    std::lock_guard<std::mutex> lock(mtx_);
    return pool_.size();
}

} // namespace tinyrpc

#endif // __OBJECTPOOL_H__
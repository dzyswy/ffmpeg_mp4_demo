#pragma once


#include <iostream>
#include <mutex>
#include <condition_variable>
#include <list>
#include <vector>
#include <memory>
#include <glog/logging.h>

namespace duck {
namespace thread {


template<typename T>
class SafeQueuePtr
{
public:
    SafeQueuePtr(int deep, const std::string& queue_name = std::string()) 
        : deep_(deep), queue_name_(queue_name), quit_(false) {

    }
    
    void set_quit(bool value) {
        std::unique_lock<std::mutex> lock(mutex_);

        quit_ = value;
        empty_cond_.notify_all(); 
        full_cond_.notify_all(); 
    }

    int push(T* data) {

        std::unique_lock<std::mutex> lock(mutex_);

        while(list_.size() >= deep_) {
            LOG(INFO) << name() << " queue is full, wait an available position...";
            full_cond_.wait(lock, [&] {return quit_ || (list_.size() < deep_);});
            if (quit_) {
                LOG(INFO) << name() << " receive quit flag, quit push!";
                return -1;
            }
        }
        list_.push_back(data);
        empty_cond_.notify_one(); 
        return 0;
    }

    T* pop() {

        std::unique_lock<std::mutex> lock(mutex_);

        while(list_.empty()) {
            LOG(INFO) << name() << " queue is empty, wait an available data...";
            empty_cond_.wait(lock, [&] {return quit_ || (!list_.empty());});
            if (quit_) {
                LOG(INFO) << name() << " receive quit flag, quit pop!";
                return nullptr;
            }
        }
        T* data = list_.front();
        list_.pop_front();
        full_cond_.notify_one(); 

        return data;
    }


    bool empty() {
        std::unique_lock<std::mutex> lock(mutex_);
        return list_.empty();
    }

    bool full() {
        std::unique_lock<std::mutex> lock(mutex_);
        bool is_full = (list_.size() >= deep_) ? true : false;
        return is_full;
    }

    size_t count() {
        std::unique_lock<std::mutex> lock(mutex_);
        return list_.size();
    }

    int deep() {
        std::unique_lock<std::mutex> lock(mutex_);
        return deep_;
    }

    std::string name() {
        return queue_name_;
    }


protected:
    int deep_;
    std::string queue_name_;
    std::list<T* > list_;
    std::mutex mutex_;
    std::condition_variable full_cond_;
    std::condition_variable empty_cond_;
    bool quit_;
};


}//namespace thread
}//namespace duck










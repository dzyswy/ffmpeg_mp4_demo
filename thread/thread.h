#pragma once

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <glog/logging.h>

namespace duck {
namespace thread {




class Thread
{
public:
    Thread(const std::string& thread_name) : thread_name_(thread_name), running_(false) {}

    virtual void process() = 0;

    virtual void start() {
        thread_ = std::thread(Thread::thread_handle, this); 
        //thread_.detach();
    }

    void join() {
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    std::string name() {
        return thread_name_;
    }

    void set_running(bool value) {
        running_ = value;
    }

    bool is_running() {
        return running_;
    }

protected:
    static void thread_handle(Thread* thread) {
        LOG(INFO) << thread->name() << " thread is running!";
        thread->set_running(true);
        thread->process();
        LOG(INFO) << thread->name() << " thread is quit!";
        thread->set_running(false);
    }

protected:
    std::string thread_name_;
    std::thread thread_;
    bool running_;
};




}//namespace thread
}//namespace duck



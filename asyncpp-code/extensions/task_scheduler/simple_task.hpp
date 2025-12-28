#pragma once
#ifndef INC_SIMPLE_TASK_HPP_
#define INC_SIMPLE_TASK_HPP_
#include <string>
#include <map>
#include <list>
#include <deque>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <utility>
#include <stdexcept>
#include "async-promise/promise.hpp"
class Service {
    using Defer     = promise::Defer;
    using Promise   = promise::Promise;
    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
    using Timers    = std::multimap<TimePoint, Defer>;
    using Tasks     = std::deque<Defer>;
    Timers timers_;
    Tasks  tasks_;
    mutable std::recursive_mutex mutex_;
    std::condition_variable_any cond_;
    std::atomic<bool> isAutoStop_;
    std::atomic<bool> isStop_;
public:
    Service()
        : isAutoStop_(true)
        , isStop_(false)
    {
    }
    Promise delay(uint64_t time_ms) {
        return promise::newPromise([&](Defer &defer) {
            TimePoint now = std::chrono::steady_clock::now();
            TimePoint time = now + std::chrono::milliseconds(time_ms);
            timers_.emplace(time, defer);
        });
    }
    Promise yield() {
        return promise::newPromise([&](Defer &defer) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            tasks_.push_back(defer);
            cond_.notify_one();
        });
    }
    void runInIoThread(const std::function<void()> &func) {
        promise::newPromise([=](Defer &defer) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            tasks_.push_back(defer);
            cond_.notify_one();
        }).then([func]() {
            func();
        });
    }
    void setAutoStop(bool isAutoExit) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        isAutoStop_ = isAutoExit;
        cond_.notify_one();
    }
    void run() {
        std::unique_lock<std::recursive_mutex> lock(mutex_);
        while(!isStop_ && (!isAutoStop_ || tasks_.size() > 0 || timers_.size() > 0)) {
            if (tasks_.size() == 0 && timers_.size() == 0) {
                cond_.wait(lock);
                continue;
            }
            while (!isStop_ && timers_.size() > 0) {
                TimePoint now = std::chrono::steady_clock::now();
                TimePoint time = timers_.begin()->first;
                if (time <= now) {
                    Defer &defer = timers_.begin()->second;
                    tasks_.push_back(defer);
                    timers_.erase(timers_.begin());
                }
                else if (tasks_.size() == 0) {
                    cond_.wait_for(lock, time - now);
                }
                else {
                    break;
                }
            }
            if(!isStop_ && tasks_.size() > 0) {
                size_t size = tasks_.size();
                for(size_t i = 0; i < size; ++i){
                    Defer defer = tasks_.front();
                    tasks_.pop_front();
                    {
                        std::unique_lock<std::recursive_mutex> unlock(mutex_, std::adopt_lock);
                        defer.resolve();
                    }
                    break;
                }
            }
        }
        while (timers_.size() > 0 || tasks_.size()) {
            while (timers_.size() > 0) {
                Defer defer = timers_.begin()->second;
                timers_.erase(timers_.begin());
                defer.reject(std::runtime_error("service stopped"));
            }
            while (tasks_.size() > 0) {
                Defer defer = tasks_.front();
                tasks_.pop_front();
                defer.reject(std::runtime_error("service stopped"));
            }
        }
    }
    void stop() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        isStop_ = true;
        cond_.notify_one();
    }
};
#endif
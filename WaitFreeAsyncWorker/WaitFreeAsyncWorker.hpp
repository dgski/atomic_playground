#pragma once

#include <functional>
#include <thread>
#include <iostream>

#include "../WaitFreeQueue/WaitFreeQueue.hpp"

class WaitFreeAsyncMulti
{
    WaitFreeQueue<std::function<void()>> _taskQueue;
    WaitFreeQueue<std::function<void()>> _callbackQueue;
    std::atomic<bool> _running = true;
    std::jthread _thread;

    void worker() {
        while (_running.load(std::memory_order::relaxed)) {
            auto nextTask = _taskQueue.getNextValue();
            if (!nextTask) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                continue;
            }
            (*nextTask)();
            continue;
        }
    }
public:
    WaitFreeAsyncMulti() : _thread(&WaitFreeAsyncMulti::worker, this) {}
    ~WaitFreeAsyncMulti() { _running.store(false); }

    template<typename ReturnType>
    class Future {
        std::optional<ReturnType> _value;
        std::atomic<ReturnType*> _ptr;
    public:
        Future() {}
        ReturnType* get() {
            return _ptr.exchange(nullptr, std::memory_order::acquire);
        }
        template<typename... Args>
        void resolve(Args&&... args) {
            _value.emplace(std::forward<Args>(args)...);
            _ptr.store(&_value.value(), std::memory_order::release);
        }
    };

    template<typename ReturnType, typename Func>
    bool schedule(Future<ReturnType>& outputPromise, Func func) {
        auto scheduleSuccess = _taskQueue.tryPublishLatestValue([&outputPromise, func]() {
            outputPromise.resolve(func());
        });
        if (!scheduleSuccess) {
            return false;
        }

        return true;
    }

    template<typename Func, typename CallbackFunc>
    bool schedule(Func func, CallbackFunc callback) {
        auto scheduleSuccess = _taskQueue.tryPublishLatestValue([this, func, callback]() {
            _callbackQueue.tryPublishLatestValue([result = std::move(func()), callback]() {
                callback(result);
            });
        });
        if (!scheduleSuccess) {
            return false;
        }

        return true;
    }

    void processCallbacks() {
        while (true) {
            auto nextCallback = _callbackQueue.getNextValue();
            if (!nextCallback) {
                return;
            }
            (*nextCallback)();
        }
    }
};
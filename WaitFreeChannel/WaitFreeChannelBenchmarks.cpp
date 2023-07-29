#include <iostream>
#include <thread>

#include "WaitFreeChannel.hpp"

template<typename Func>
inline auto benchmarkAndReturn(Func func) {
    auto t1 = std::chrono::high_resolution_clock::now();
    auto result = func();
    auto t2 = std::chrono::high_resolution_clock::now();
    return std::make_pair(result, std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count());
}


// Locking re-creation of the 'WaitFreeChannel' channel interface
template<typename Type>
class LockingChannel {
    std::mutex _mutex;
    Type* _ptr = nullptr;
    size_t _nextInsertionIndex = 0;
    std::optional<Type> _values[2];
public:
    LockingChannel() {}

    __attribute__((always_inline)) Type* getLatestValue() {
        Type* result = nullptr;
        std::lock_guard lock(_mutex);
        std::swap(result, _ptr);
        return result;
    }

    template<typename... Args>
    __attribute__((always_inline)) bool tryPublishLatestValue(Args&&... args) {
        std::lock_guard lock(_mutex);
        if (_ptr) {
            return false;
        }
        _values[_nextInsertionIndex].emplace(std::forward<Args>(args)...);
        _ptr = &_values[_nextInsertionIndex].value();
        _nextInsertionIndex = (_nextInsertionIndex + 1) % 2;
        return true;
    }
};

struct Message {
    std::string _data = "this is a data";
    Message(std::string data) : _data(data) {}
    auto& data() { return _data; }
};

template<typename ChannelType>
size_t runChannelReadLatencyBenchmark() {
    std::atomic<bool> running = true;
    ChannelType channel;

    size_t totalNanoSecondsTaken = 0;
    size_t reads = 0;

    // Thread one: critical that should minimize polling time
    auto t1 = std::thread([&]() {
        while (running.load(std::memory_order::relaxed)) {
            auto [nextMessage, timeTaken] = benchmarkAndReturn([&]() { return channel.getLatestValue(); });
            totalNanoSecondsTaken += timeTaken;
            ++reads;
            if (!nextMessage) {
                continue;
            }
        }
    });

    // Thread two: background processing thread (has all the time in the world)
    auto t2 = std::thread([&]() {
        while (running.load(std::memory_order::relaxed)) {
            if (!channel.tryPublishLatestValue("here is a mesage")) {
                continue;
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(10));

    running = false;

    t1.join();
    t2.join();

    return (totalNanoSecondsTaken / reads);
}

//template size_t runChannelReadLatencyBenchmark<LockingChannel<Message>>();
template size_t runChannelReadLatencyBenchmark<WaitFreeChannel<Message>>();

int main() {
    std::cout << "locking averagePollingTimeNs=" << runChannelReadLatencyBenchmark<LockingChannel<Message>>() << std::endl;
    std::cout << "waitFree averagePollingTimeNs=" << runChannelReadLatencyBenchmark<WaitFreeChannel<Message>>() << std::endl;
    return EXIT_SUCCESS;
}
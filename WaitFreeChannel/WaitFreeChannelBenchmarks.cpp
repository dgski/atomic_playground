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
    bool _read = true;
    std::optional<Type> _value;
public:
    LockingChannel() {}

    __attribute__((always_inline)) Type* getLatestValue() {
        std::lock_guard lock(_mutex);
        if (_read ) [[likely]] {
            return nullptr;
        }
        _read = true;
        return &_value.value();
    }

    template<typename... Args>
    __attribute__((always_inline)) bool tryPublishLatestValue(Args&&... args) {
        std::lock_guard lock(_mutex);
        if (!_read && _value.has_value()) {
            return false;
        }
        _read = false;
        _value.emplace(std::forward<Args>(args)...);
        return true;
    }
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
            while (!channel.tryPublishLatestValue()) {}
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(10));

    running = false;

    t1.join();
    t2.join();

    return (totalNanoSecondsTaken / reads);
}

struct Message {};

template size_t runChannelReadLatencyBenchmark<LockingChannel<Message>>();
template size_t runChannelReadLatencyBenchmark<WaitFreeChannel<Message>>();

int main() {
    std::cout << "locking averagePollingTime=" << runChannelReadLatencyBenchmark<LockingChannel<Message>>() << std::endl;
    std::cout << "waitFree averagePollingTime=" << runChannelReadLatencyBenchmark<WaitFreeChannel<Message>>() << std::endl;
    return EXIT_SUCCESS;
}
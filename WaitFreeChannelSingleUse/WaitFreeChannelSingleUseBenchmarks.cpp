#include <iostream>
#include <thread>
#include <vector>

#include "WaitFreeChannelSingleUse.hpp"
#include "../Shared/Utils.hpp"

// Locking re-creation of the 'WaitFreeChannelSingleUse' channel interface
template<typename Type>
class LockingChannelSingleUse {
    std::mutex _mutex;
    Type* _ptr = nullptr;
    std::optional<Type> _value;
public:
    LockingChannelSingleUse() {}

    __attribute__((always_inline)) Type* get() {
        Type* result = nullptr;
        std::lock_guard lock(_mutex);
        std::swap(result, _ptr);
        return result;
    }

    template<typename... Args>
    __attribute__((always_inline)) bool publish(Args&&... args) {
        std::lock_guard lock(_mutex);
        _value.emplace(std::forward<Args>(args)...);
        _ptr = &_value.value();
        return true;
    }
};

struct BenchmarkResult {
    size_t _readTimeTaken;
    size_t _writeTimeTaken;
};

// define stream-out operator for above struct
std::ostream& operator<<(std::ostream& os, const BenchmarkResult& result) {
    os << "readTimeTaken=" << result._readTimeTaken << "ns, writeTimeTaken=" << result._writeTimeTaken << "ns";
    return os;
}

template<typename ChannelType>
BenchmarkResult runChannelReadLatencyBenchmark() {
    std::atomic<bool> running = true;

    size_t totalNanoSecondsTaken = 0;
    size_t reads = 0;

    size_t totalNanoSecondsTakenWrites = 0;
    size_t writes = 0;

    // Create channels
    ChannelType* channels = new ChannelType[1000000];

    // Thread one: critical that should minimize polling time
    auto t1 = std::thread([&]() {
        for (size_t i = 0; i<1000000; ++i) {
            auto& channel = channels[i];
            auto [nextMessage, timeTaken] = benchmarkAndReturn([&]() { return channel.get(); });
            totalNanoSecondsTaken += timeTaken;
            ++reads;
            if (!nextMessage) {
                continue;
            }
        }
    });

    // Thread two: background processing thread
    auto t2 = std::thread([&]() {
        for (size_t i = 0; i<1000000; ++i) {
            auto& channel = channels[i];
            auto [result, timeTaken] = benchmarkAndReturn([&]() { return channel.publish(12341234); });
            totalNanoSecondsTakenWrites += timeTaken;
            ++writes;
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(10));

    running = false;

    t1.join();
    t2.join();

    return { (totalNanoSecondsTaken / reads), (totalNanoSecondsTakenWrites / writes) };
}

int main() {
    std::cout << "locking averagePollingTimeNs=" << runChannelReadLatencyBenchmark<LockingChannelSingleUse<int>>() << std::endl;
    std::cout << "waitFree averagePollingTimeNs=" << runChannelReadLatencyBenchmark<WaitFreeChannelSingleUse<int>>() << std::endl;

    // Open a channel a many times
    size_t totalTimeToOpen = 0;
    for (size_t i = 0; i<1000000; ++i) {
        auto [_, timeTaken] = benchmarkAndReturn([]() { WaitFreeChannelSingleUse<std::string>(); return true; });
        totalTimeToOpen += timeTaken;
    }
    std::cout << "averageTimeToOpen=" << (totalTimeToOpen / 1000000) << std::endl;

    WaitFreeChannelSingleUse<std::string> channel;

    auto t = std::thread([&]() {
        std::this_thread::sleep_for(std::chrono::seconds(4));
        channel.publish("This is some information");
    });

    // Wait for result
    size_t polls = 0;
    size_t tightLoopTotalTimeTaken = 0;
    while (true) {
        auto [result, timeTaken] = benchmarkAndReturn([&]() { return channel.get(); });
        if (result) {
            std::cout << "got result=" << *result << std::endl;
            channel.reset();
            break;
        }
        ++polls;
        tightLoopTotalTimeTaken += timeTaken;
    }
    t.join();

    std::cout << "tightLoopTotalTimeTaken=" << tightLoopTotalTimeTaken / polls << std::endl;

    return EXIT_SUCCESS;
}
#include <thread>
#include <mutex>

#include "WaitFreeQueue.hpp"

template<typename T>
class LockingQueue {
    std::mutex _mutex;
    std::optional<T> _values[std::numeric_limits<uint8_t>::max()+1];
    alignas(64) uint8_t _readIndex = 0;
    alignas(64) uint8_t _writeIndex = 0;
public:
    LockingQueue() {}

    __attribute__((always_inline)) T* getNextValue() {
        std::lock_guard lock(_mutex);
        if (_readIndex == _writeIndex) {
            return nullptr;
        }
        return &_values[_readIndex++].value();
    }

    template<typename... Args>
    __attribute__((always_inline)) bool tryPublishLatestValue(Args&&... args) {
        std::lock_guard lock(_mutex);
        if (uint8_t(_writeIndex + 1) == _readIndex) {
            return false;
        }
        _values[_writeIndex++].emplace(std::forward<Args>(args)...);
        return true;
    }
};

struct Message {
    char _contents[256];
};

template<typename QueueType>
size_t runQueueThroughputBenchmark() {
    QueueType queue;
    std::atomic<bool> running = true;
    size_t reads = 0;

    // Thread one: critical that should minimize polling time
    auto t1 = std::thread([&]() {
        while (running.load(std::memory_order::relaxed)) {
            auto nextMessage = queue.getNextValue();
            if (!nextMessage) {
                continue;
            }
            ++reads;
        }
    });

    // Thread two: background processing thread (has all the time in the world)
    auto t2 = std::thread([&]() {
        while (running.load(std::memory_order::relaxed)) {
            if (!queue.tryPublishLatestValue()) {
                continue;
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(10));

    running = false;

    t1.join();
    t2.join();

    return reads / 10;
};

template size_t runQueueThroughputBenchmark<LockingQueue<Message>>();
template size_t runQueueThroughputBenchmark<WaitFreeQueue<Message>>();

int main() {
    std::cout << "  lockingTotalThroughputPerSecond=" << runQueueThroughputBenchmark<LockingQueue<Message>>() << std::endl;
    std::cout << " waitFreeTotalThroughputPerSecond=" << runQueueThroughputBenchmark<WaitFreeQueue<Message>>() << std::endl;
    return EXIT_SUCCESS;
}
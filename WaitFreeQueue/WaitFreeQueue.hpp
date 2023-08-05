#include <atomic>
#include <iostream>
#include <optional>
#include <limits>

// Wait-free concurrent queue
// Based on tutorial: https://rigtorp.se/ringbuffer/
// Lock-free writing, wait and lock-free reading
// Supports move-only types
template<typename T, size_t N = 256>
class WaitFreeQueue {
    std::optional<T> _values[N];
    alignas(64) std::atomic<size_t> _readIndex = 0;
    alignas(64) size_t _readIndexCached = 0;
    alignas(64) std::atomic<size_t> _writeIndex = 0;
    alignas(64) size_t _writeIndexCached = 0;

    static size_t getNextIndex(size_t currentIndex) {
        if ((currentIndex + 1) == N) {
            return 0;
        }
        return currentIndex + 1;
    }

    __attribute__((always_inline)) size_t getReadIndex(size_t nextWriteIndex) {
        if (nextWriteIndex == _readIndexCached) {
            _readIndexCached = _readIndex.load(std::memory_order::acquire);
        }
        return _readIndexCached;
    }

    __attribute__((always_inline)) size_t getWriteIndex(size_t nextReadIndex) {
        if (nextReadIndex == _writeIndexCached) {
            _writeIndexCached = _writeIndex.load(std::memory_order::acquire);
        }
        return _writeIndexCached;
    }
public:
    WaitFreeQueue() {}

    __attribute__((always_inline)) T* getNextValue() {
        const auto readIndex = _readIndex.load(std::memory_order::relaxed);
        if (readIndex == getWriteIndex(readIndex)) {
            return nullptr;
        }

        auto result = &_values[readIndex].value();
        _readIndex.store(getNextIndex(readIndex), std::memory_order::release);
        return result;
    }

    template<typename... Args>
    __attribute__((always_inline)) bool tryPublishLatestValue(Args&&... args) {
        const auto writeIndex = _writeIndex.load(std::memory_order::relaxed);
        const auto nextWriteIndex = getNextIndex(writeIndex);
        if (nextWriteIndex == getReadIndex(nextWriteIndex)) {
            return false;
        }
        _values[writeIndex].emplace(std::forward<Args>(args)...);
        _writeIndex.store(nextWriteIndex, std::memory_order::release);
        return true;
    }
};
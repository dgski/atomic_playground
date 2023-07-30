#include <atomic>
#include <iostream>
#include <optional>
#include <limits>

// Wait-free concurrent queue
// Based on tutorial: https://rigtorp.se/ringbuffer/
// Lock-free writing, wait and lock-free reading
// Supports move-only types
template<typename T>
class WaitFreeQueue {
    std::optional<T> _values[std::numeric_limits<uint8_t>::max()+1];
    alignas(64) std::atomic<uint8_t> _readIndex = 0;
    alignas(64) uint8_t _readIndexCached = 0;
    alignas(64) std::atomic<uint8_t> _writeIndex = 0;
    alignas(64) uint8_t _writeIndexCached = 0;

    __attribute__((always_inline)) uint8_t getReadIndex(uint8_t nextWriteIndex) {
        if (nextWriteIndex == _readIndexCached) {
            _readIndexCached = _readIndex.load(std::memory_order::acquire);
        }
        return _readIndexCached;
    }

    __attribute__((always_inline)) uint8_t getWriteIndex(uint8_t nextReadIndex) {
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
        _readIndex.store(readIndex + uint8_t(1), std::memory_order::release);
        return result;
    }

    template<typename... Args>
    __attribute__((always_inline)) bool tryPublishLatestValue(Args&&... args) {
        const auto writeIndex = _writeIndex.load(std::memory_order::relaxed);
        const auto nextWriteIndex = uint8_t(writeIndex + 1);
        if (nextWriteIndex == getReadIndex(nextWriteIndex)) {
            return false;
        }
        _values[writeIndex].emplace(std::forward<Args>(args)...);
        _writeIndex.store(nextWriteIndex, std::memory_order::release);
        return true;
    }
};
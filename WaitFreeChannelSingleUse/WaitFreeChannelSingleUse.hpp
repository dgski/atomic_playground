#include <atomic>
#include <optional>

// Inter-thread communication 'channel'
// lock and Wait free reading, lock-free writing
// Supports move-only types and optimized for low polling latency on read side
// Only one value can be published at a time
//
// Potentially a lot faster than the regular variation as
// is can use memory_order_relaxed for loads and stores
template<typename Type>
class WaitFreeChannelSingleUse {
    alignas(64) std::optional<Type> _value;
    alignas(64) std::atomic<Type*> _ptr = nullptr;
public:
    WaitFreeChannelSingleUse() = default;
    WaitFreeChannelSingleUse(const WaitFreeChannelSingleUse&) = delete;
    WaitFreeChannelSingleUse(const WaitFreeChannelSingleUse&&) = delete;
    void operator=(const WaitFreeChannelSingleUse&) = delete;
    void operator=(const WaitFreeChannelSingleUse&&) = delete;

    // Get the latest published value, returns nullptr if no new value available
    __attribute__((always_inline)) Type* get() {
        return _ptr.load(std::memory_order::relaxed);
    }

    // Try to publish a new value, returns true if successful, false if not
    template<typename... Args>
    __attribute__((always_inline)) bool publish(Args&&... args) {
        _value.emplace(std::forward<Args>(args)...);
        _ptr.store(&_value.value(), std::memory_order::relaxed);
        return true;
    }

    void reset() {
        _value.reset();
        _ptr = nullptr;
    }
};
#include <atomic>
#include <optional>

// Inter-thread communication 'channel'
// lock and Wait free reading, lock-free writing
// Supports move-only types and optimized for low polling latency on read side
// Only one value can be published at a time
template<typename Type>
class WaitFreeChannel {
    std::optional<Type> _value;
    std::atomic<Type*> _ptr = nullptr;
public:
    WaitFreeChannel() = default;
    WaitFreeChannel(Type&& value) : _value(value), _ptr(&_value.value()) {}
    WaitFreeChannel(const WaitFreeChannel&) = delete;
    WaitFreeChannel(const WaitFreeChannel&&) = delete;
    void operator=(const WaitFreeChannel&) = delete;
    void operator=(const WaitFreeChannel&&) = delete;

    // Get the latest published value, returns nullptr if no new value available
    __attribute__((always_inline)) Type* getLatestValue() {
        return _ptr.exchange(nullptr, std::memory_order::acquire);
    }

    // Try to publish a new value, returns true if successful, false if not
    template<typename... Args>
    __attribute__((always_inline)) bool tryPublishLatestValue(Args&&... args) {
        if (_ptr.load(std::memory_order::seq_cst) != nullptr) {
            // Other thread still hasn't processed the other value you posted; don't publish
            return false;
        }
        _value.emplace(std::forward<Args>(args)...);
        _ptr.store(&_value.value(), std::memory_order::release);
        return true;
    }

    // Blocks until new value is published
    template<typename... Args>
    __attribute__((always_inline)) void publishLatestValue(Args&&... args) {
        while (!tryPublishLatestValue(std::forward<Args>(args)...)) {}
    }
};
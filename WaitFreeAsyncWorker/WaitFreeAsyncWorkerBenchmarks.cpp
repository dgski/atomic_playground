#include "WaitFreeAsyncWorker.hpp"
#include "../Shared/Utils.hpp"

int main() {
    WaitFreeAsyncMulti worker;

    size_t totalSchedulingTime = 0;
    size_t totalPollingTime = 0;
    constexpr auto ITERATIONS = 1000;
    for (size_t i=0; i<ITERATIONS; ++i) {
        auto [success, timeTaken] = benchmarkAndReturn([&]() { return worker.schedule([]() {
                std::vector<size_t> result;
                for (size_t i; i<10000000; ++i) {
                    result.push_back(i);
                }
                return result;
            },
            [](auto&) {
            });
        });
        totalSchedulingTime += timeTaken;
        auto [_, pollingTimeTaken] = benchmarkAndReturn(
            [&]() { worker.processCallbacks(); return true; });;
        totalPollingTime += pollingTimeTaken;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << " averageSchedulingTimeNs=" << (totalSchedulingTime / ITERATIONS) << std::endl;
    std::cout << "averagePollingTimeNs=" << (totalPollingTime / ITERATIONS) << std::endl;

    return 0;
}
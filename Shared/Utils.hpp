#pragma once

template<typename Func>
inline auto benchmarkAndReturn(Func func) {
    auto t1 = std::chrono::high_resolution_clock::now();
    auto result = func();
    auto t2 = std::chrono::high_resolution_clock::now();
    return std::make_pair(result, std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count());
}
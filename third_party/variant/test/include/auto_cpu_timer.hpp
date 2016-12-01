#pragma once

#include <chrono>
#include <iostream>

struct auto_cpu_timer {
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    auto_cpu_timer() : start(std::chrono::high_resolution_clock::now()) {
    }
    ~auto_cpu_timer() {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::microseconds elapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cerr << elapsed.count() << "us" << std::endl;
    }
};

#include "catch.hpp"

#include <osmium/util/timer.hpp>

#include <chrono>
#include <thread>

TEST_CASE("timer") {
    osmium::Timer timer;
    timer.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    timer.stop();
    REQUIRE(timer.elapsed_microseconds() == 0);
}


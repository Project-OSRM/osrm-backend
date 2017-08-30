#include "catch.hpp"

#include <chrono>
#include <thread>

#define OSMIUM_WITH_TIMER
#include <osmium/util/timer.hpp>

TEST_CASE("timer") {
    osmium::Timer timer;
    timer.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    timer.stop();
    REQUIRE(timer.elapsed_microseconds() > 900);
}


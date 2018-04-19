#include "catch.hpp"

#include <osmium/thread/queue.hpp>

TEST_CASE("Basic use of thread-safe queue") {
    osmium::thread::Queue<int> queue;
    REQUIRE(queue.empty());
    queue.push(22);
    REQUIRE_FALSE(queue.empty());
    REQUIRE(queue.size() == 1);
    int value = 0;
    queue.wait_and_pop(value);
    REQUIRE(value == 22);
    REQUIRE(queue.empty());
}

TEST_CASE("Queue can have max elements and can be named") {
    osmium::thread::Queue<int> queue{100, "Queue of max size 100"};
}


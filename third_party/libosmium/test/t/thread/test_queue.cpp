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
    const osmium::thread::Queue<int> queue{100, "Queue of max size 100"};
}

TEST_CASE("When queue is shut down, nothing goes in or out") {
    osmium::thread::Queue<std::string> queue;
    REQUIRE(queue.in_use());
    REQUIRE(queue.empty());
    queue.push("foo");
    queue.push("bar");
    queue.push("baz");
    REQUIRE(queue.size() == 3);

    std::string value;

    queue.wait_and_pop(value);
    REQUIRE(value == "foo");
    REQUIRE(queue.size() == 2);
    REQUIRE(queue.in_use());
    queue.shutdown();
    REQUIRE_FALSE(queue.in_use());
    REQUIRE(queue.empty());
    queue.push("lost");
    REQUIRE(queue.empty());

    value.clear();
    queue.try_pop(value);
    REQUIRE(value.empty());
    queue.wait_and_pop(value);
    REQUIRE(value.empty());
}

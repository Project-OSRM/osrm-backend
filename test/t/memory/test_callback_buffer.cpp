#include "catch.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/memory/callback_buffer.hpp>

#include <iterator>

using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)

TEST_CASE("Callback buffer") {
    osmium::memory::CallbackBuffer cb;

    REQUIRE(cb.buffer().committed() == 0);

    osmium::builder::add_node(cb.buffer(), _id(1));
    osmium::builder::add_node(cb.buffer(), _id(2));
    osmium::builder::add_node(cb.buffer(), _id(3));

    auto c = cb.buffer().committed();
    REQUIRE(c > 0);

    REQUIRE(std::distance(cb.buffer().begin(), cb.buffer().end()) == 3);
    auto buffer = cb.read();

    REQUIRE(cb.buffer().committed() == 0);
    REQUIRE(buffer.committed() == c);
    REQUIRE(std::distance(cb.buffer().begin(), cb.buffer().end()) == 0);

    // no callback defined, so nothing will happen
    cb.flush();
}

TEST_CASE("Callback buffer with callback triggering every time") {
    int run = 0;

    osmium::memory::CallbackBuffer cb{[&](osmium::memory::Buffer&& buffer){
        REQUIRE(buffer.committed() > 0);
        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 1);
        ++run;
    }, 1000, 10};

    osmium::builder::add_node(cb.buffer(), _id(1));
    REQUIRE(cb.buffer().committed() > 10);
    cb.possibly_flush();
    osmium::builder::add_node(cb.buffer(), _id(2));
    REQUIRE(cb.buffer().committed() > 10);
    cb.possibly_flush();
    osmium::builder::add_node(cb.buffer(), _id(3));
    REQUIRE(cb.buffer().committed() > 10);
    cb.possibly_flush();

    REQUIRE(run == 3);
    REQUIRE(std::distance(cb.buffer().begin(), cb.buffer().end()) == 0);
}

TEST_CASE("Callback buffer with callback triggering sometimes") {
    int run = 0;

    osmium::memory::CallbackBuffer cb{[&](osmium::memory::Buffer&& buffer){
        REQUIRE(buffer.committed() > 0);
        ++run;
    }, 1000, 100};

    osmium::builder::add_node(cb.buffer(), _id(1));
    REQUIRE(cb.buffer().committed() < 100);
    cb.possibly_flush();
    osmium::builder::add_node(cb.buffer(), _id(2));
    cb.possibly_flush();
    osmium::builder::add_node(cb.buffer(), _id(3));
    cb.possibly_flush();

    REQUIRE(run < 3);
}

TEST_CASE("Callback buffer with callback set later") {
    int run = 0;

    osmium::memory::CallbackBuffer cb{1000, 10};

    osmium::builder::add_node(cb.buffer(), _id(1));
    cb.possibly_flush();
    osmium::builder::add_node(cb.buffer(), _id(2));
    cb.possibly_flush();
    osmium::builder::add_node(cb.buffer(), _id(3));

    cb.set_callback([&](osmium::memory::Buffer&& buffer){
        REQUIRE(buffer.committed() > 0);
        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 3);
        ++run;
    });

    REQUIRE(std::distance(cb.buffer().begin(), cb.buffer().end()) == 3);

    cb.possibly_flush();

    REQUIRE(std::distance(cb.buffer().begin(), cb.buffer().end()) == 0);

    REQUIRE(run == 1);
}



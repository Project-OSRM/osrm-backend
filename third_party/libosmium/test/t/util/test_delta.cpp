#include "catch.hpp"

#include <vector>

#include <osmium/util/delta.hpp>

TEST_CASE("delta encode int") {

    osmium::util::DeltaEncode<int> x;

    SECTION("int") {
        REQUIRE(x.update(17) == 17);
        REQUIRE(x.update(10) == -7);
        REQUIRE(x.update(-10) == -20);
    }

}

TEST_CASE("delta decode int") {

    osmium::util::DeltaDecode<int> x;

    SECTION("int") {
        REQUIRE(x.update(17) == 17);
        REQUIRE(x.update(10) == 27);
        REQUIRE(x.update(-40) == -13);
    }

}

TEST_CASE("delta encode unsigned int") {

    osmium::util::DeltaEncode<unsigned int> x;

    SECTION("int") {
        REQUIRE(x.update(17) == 17);
        REQUIRE(x.update(10) == -7);
        REQUIRE(x.update(0) == -10);
    }

}

TEST_CASE("delta decode unsigned int") {

    osmium::util::DeltaDecode<unsigned int> x;

    SECTION("int") {
        REQUIRE(x.update(17) == 17);
        REQUIRE(x.update(10) == 27);
        REQUIRE(x.update(-15) == 12);
    }

}

TEST_CASE("delta encode and decode") {

    std::vector<int> a = { 5, -9, 22, 13, 0, 23 };

    osmium::util::DeltaEncode<int, int> de;
    std::vector<int> b;
    for (int x : a) {
        b.push_back(de.update(x));
    }

    osmium::util::DeltaDecode<int, int> dd;
    std::vector<int> c;
    for (int x : b) {
        c.push_back(dd.update(x));
    }

}


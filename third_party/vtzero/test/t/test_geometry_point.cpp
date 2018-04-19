
#include <test.hpp>

#include <vtzero/geometry.hpp>

#include <cstdint>
#include <vector>

using container = std::vector<uint32_t>;
using iterator = container::const_iterator;

class dummy_geom_handler {

    int value = 0;

public:

    void points_begin(const uint32_t /*count*/) noexcept {
        ++value;
    }

    void points_point(const vtzero::point /*point*/) noexcept {
        value += 100;
    }

    void points_end() noexcept {
        value += 10000;
    }

    int result() const noexcept {
        return value;
    }

}; // class dummy_geom_handler

TEST_CASE("Calling decode_point() with empty input") {
    const container g;
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(dummy_geom_handler{}),
                            "expected MoveTo command (spec 4.3.4.2)");
    }
}

TEST_CASE("Calling decode_point() with a valid point") {
    const container g = {9, 50, 34};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    dummy_geom_handler handler;
    decoder.decode_point(handler);
    REQUIRE(handler.result() == 10101);
}

TEST_CASE("Calling decode_point() with a valid multipoint") {
    const container g = {17, 10, 14, 3, 9};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    REQUIRE(decoder.decode_point(dummy_geom_handler{}) == 10201);
}

TEST_CASE("Calling decode_point() with a linestring geometry fails") {
    const container g = {9, 4, 4, 18, 0, 16, 16, 0}; // this is a linestring geometry
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(dummy_geom_handler{}),
                            "additional data after end of geometry (spec 4.3.4.2)");
    }
}

TEST_CASE("Calling decode_point() with a polygon geometry fails") {
    const container g = {9, 6, 12, 18, 10, 12, 24, 44, 15}; // this is a polygon geometry
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(dummy_geom_handler{}),
                            "additional data after end of geometry (spec 4.3.4.2)");
    }
}

TEST_CASE("Calling decode_point() with something other than MoveTo command") {
    const container g = {vtzero::detail::command_line_to(3)};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(dummy_geom_handler{}),
                            "expected command 1 but got 2");
    }
}

TEST_CASE("Calling decode_point() with a count of 0") {
    const container g = {vtzero::detail::command_move_to(0)};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(dummy_geom_handler{}),
                            "MoveTo command count is zero (spec 4.3.4.2)");
    }
}

TEST_CASE("Calling decode_point() with more data then expected") {
    const container g = {9, 50, 34, 9};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(dummy_geom_handler{}),
                            "additional data after end of geometry (spec 4.3.4.2)");
    }
}


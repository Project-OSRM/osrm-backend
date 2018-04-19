
#include <test.hpp>

#include <vtzero/geometry.hpp>

#include <cstdint>
#include <vector>

using container = std::vector<uint32_t>;
using iterator = container::const_iterator;

class dummy_geom_handler {

    int value = 0;

public:

    void linestring_begin(const uint32_t /*count*/) noexcept {
        ++value;
    }

    void linestring_point(const vtzero::point /*point*/) noexcept {
        value += 100;
    }

    void linestring_end() noexcept {
        value += 10000;
    }

    int result() const noexcept {
        return value;
    }

}; // class dummy_geom_handler

TEST_CASE("Calling decode_linestring_geometry() with empty input") {
    const container g;
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    dummy_geom_handler handler;
    decoder.decode_linestring(dummy_geom_handler{});
    REQUIRE(handler.result() == 0);
}

TEST_CASE("Calling decode_linestring_geometry() with a valid linestring") {
    const container g = {9, 4, 4, 18, 0, 16, 16, 0};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    REQUIRE(decoder.decode_linestring(dummy_geom_handler{}) == 10301);
}

TEST_CASE("Calling decode_linestring_geometry() with a valid multilinestring") {
    const container g = {9, 4, 4, 18, 0, 16, 16, 0, 9, 17, 17, 10, 4, 8};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    dummy_geom_handler handler;
    decoder.decode_linestring(handler);
    REQUIRE(handler.result() == 20502);
}

TEST_CASE("Calling decode_linestring_geometry() with a point geometry fails") {
    const container g = {9, 50, 34}; // this is a point geometry
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(dummy_geom_handler{}),
                            "expected LineTo command (spec 4.3.4.3)");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with a polygon geometry fails") {
    const container g = {9, 6, 12, 18, 10, 12, 24, 44, 15}; // this is a polygon geometry
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(dummy_geom_handler{}),
                            "expected command 1 but got 7");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with something other than MoveTo command") {
    const container g = {vtzero::detail::command_line_to(3)};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(dummy_geom_handler{}),
                            "expected command 1 but got 2");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with a count of 0") {
    const container g = {vtzero::detail::command_move_to(0)};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(dummy_geom_handler{}),
                            "MoveTo command count is not 1 (spec 4.3.4.3)");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with a count of 2") {
    const container g = {vtzero::detail::command_move_to(2), 10, 20, 20, 10};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(dummy_geom_handler{}),
                            "MoveTo command count is not 1 (spec 4.3.4.3)");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with 2nd command not a LineTo") {
    const container g = {vtzero::detail::command_move_to(1), 3, 4,
                         vtzero::detail::command_move_to(1)};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(dummy_geom_handler{}),
                            "expected command 2 but got 1");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with LineTo and 0 count") {
    const container g = {vtzero::detail::command_move_to(1), 3, 4,
                         vtzero::detail::command_line_to(0)};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(dummy_geom_handler{}),
                            "LineTo command count is zero (spec 4.3.4.3)");
    }
}


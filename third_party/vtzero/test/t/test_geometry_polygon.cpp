
#include <test.hpp>

#include <vtzero/geometry.hpp>

#include <cstdint>
#include <vector>

using container = std::vector<uint32_t>;
using iterator = container::const_iterator;

class dummy_geom_handler {

    int value = 0;

public:

    void ring_begin(const uint32_t /*count*/) noexcept {
        ++value;
    }

    void ring_point(const vtzero::point /*point*/) noexcept {
        value += 100;
    }

    void ring_end(vtzero::ring_type /*is_outer*/) noexcept {
        value += 10000;
    }

    int result() const noexcept {
        return value;
    }

}; // class dummy_geom_handler

TEST_CASE("Calling decode_polygon_geometry() with empty input") {
    const container g;
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    dummy_geom_handler handler;
    decoder.decode_polygon(dummy_geom_handler{});
    REQUIRE(handler.result() == 0);
}

TEST_CASE("Calling decode_polygon_geometry() with a valid polygon") {
    const container g = {9, 6, 12, 18, 10, 12, 24, 44, 15};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    REQUIRE(decoder.decode_polygon(dummy_geom_handler{}) == 10401);
}

TEST_CASE("Calling decode_polygon_geometry() with a duplicate end point") {
    const container g = {9, 6, 12, 26, 10, 12, 24, 44, 33, 55, 15};

    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};
    dummy_geom_handler handler;
    decoder.decode_polygon(handler);
    REQUIRE(handler.result() == 10501);
}

TEST_CASE("Calling decode_polygon_geometry() with a valid multipolygon") {
    const container g = {9, 0, 0, 26, 20, 0, 0, 20, 19, 0, 15, 9, 22, 2, 26, 18,
                         0, 0, 18, 17, 0, 15, 9, 4, 13, 26, 0, 8, 8, 0, 0, 7, 15};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    dummy_geom_handler handler;
    decoder.decode_polygon(handler);
    REQUIRE(handler.result() == 31503);
}

TEST_CASE("Calling decode_polygon_geometry() with a point geometry fails") {
    const container g = {9, 50, 34}; // this is a point geometry
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_polygon(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_polygon(dummy_geom_handler{}),
                            "expected LineTo command (spec 4.3.4.4)");
    }
}

TEST_CASE("Calling decode_polygon_geometry() with a linestring geometry fails") {
    const container g = {9, 4, 4, 18, 0, 16, 16, 0}; // this is a linestring geometry
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_polygon(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_polygon(dummy_geom_handler{}),
                            "expected ClosePath command (4.3.4.4)");
    }
}

TEST_CASE("Calling decode_polygon_geometry() with something other than MoveTo command") {
    const container g = {vtzero::detail::command_line_to(3)};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_polygon(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_polygon(dummy_geom_handler{}),
                            "expected command 1 but got 2");
    }
}

TEST_CASE("Calling decode_polygon_geometry() with a count of 0") {
    const container g = {vtzero::detail::command_move_to(0)};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_polygon(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_polygon(dummy_geom_handler{}),
                            "MoveTo command count is not 1 (spec 4.3.4.4)");
    }
}

TEST_CASE("Calling decode_polygon_geometry() with a count of 2") {
    const container g = {vtzero::detail::command_move_to(2), 1, 2, 3, 4};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_polygon(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_polygon(dummy_geom_handler{}),
                            "MoveTo command count is not 1 (spec 4.3.4.4)");
    }
}

TEST_CASE("Calling decode_polygon_geometry() with 2nd command not a LineTo") {
    const container g = {vtzero::detail::command_move_to(1), 3, 4,
                         vtzero::detail::command_move_to(1)};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_polygon(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_polygon(dummy_geom_handler{}),
                            "expected command 2 but got 1");
    }
}

TEST_CASE("Calling decode_polygon_geometry() with LineTo and 0 count") {
    const container g = {vtzero::detail::command_move_to(1), 3, 4,
                         vtzero::detail::command_line_to(0),
                         vtzero::detail::command_close_path()};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    dummy_geom_handler handler;
    decoder.decode_polygon(handler);
    REQUIRE(handler.result() == 10201);
}

TEST_CASE("Calling decode_polygon_geometry() with LineTo and 1 count") {
    const container g = {vtzero::detail::command_move_to(1), 3, 4,
                         vtzero::detail::command_line_to(1), 5, 6,
                         vtzero::detail::command_close_path()};

    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};
    dummy_geom_handler handler;
    decoder.decode_polygon(handler);
    REQUIRE(handler.result() == 10301);
}

TEST_CASE("Calling decode_polygon_geometry() with 3nd command not a ClosePath") {
    const container g = {vtzero::detail::command_move_to(1), 3, 4,
                         vtzero::detail::command_line_to(2), 4, 5, 6, 7,
                         vtzero::detail::command_line_to(0)};
    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_polygon(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_polygon(dummy_geom_handler{}),
                            "expected command 7 but got 2");
    }
}

TEST_CASE("Calling decode_polygon_geometry() on polygon with zero area") {
    const container g = {vtzero::detail::command_move_to(1), 0, 0,
                         vtzero::detail::command_line_to(3), 2, 0, 0, 4, 2, 0,
                         vtzero::detail::command_close_path()};

    vtzero::detail::geometry_decoder<container::const_iterator> decoder{g.begin(), g.end(), g.size() / 2};
    dummy_geom_handler handler;
    decoder.decode_polygon(handler);
    REQUIRE(handler.result() == 10501);
}


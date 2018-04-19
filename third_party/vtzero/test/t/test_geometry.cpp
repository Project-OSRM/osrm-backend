
#include <test.hpp>

#include <vtzero/geometry.hpp>

#include <cstdint>
#include <limits>
#include <vector>

using container = std::vector<uint32_t>;
using iterator = container::const_iterator;

TEST_CASE("geometry_decoder") {
    const container g = {};

    vtzero::detail::geometry_decoder<iterator> decoder{g.cbegin(), g.cend(), g.size() / 2};

    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE_THROWS_AS(decoder.next_point(), const assert_error&);
}

TEST_CASE("geometry_decoder with point") {
    const container g = {9, 50, 34};

    vtzero::detail::geometry_decoder<iterator> decoder{g.cbegin(), g.cend(), g.size() / 2};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE_THROWS_AS(decoder.next_point(), const assert_error&);

    SECTION("trying to get LineTo command") {
        REQUIRE_THROWS_AS(decoder.next_command(vtzero::detail::CommandId::LINE_TO), const vtzero::geometry_exception&);
    }

    SECTION("trying to get ClosePath command") {
        REQUIRE_THROWS_WITH(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH), "expected command 7 but got 1");
    }

    SECTION("trying to get MoveTo command") {
        REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
        REQUIRE_THROWS_AS(decoder.next_command(vtzero::detail::CommandId::MOVE_TO), const assert_error&);
        REQUIRE(decoder.count() == 1);
        REQUIRE(decoder.next_point() == vtzero::point(25, 17));

        REQUIRE(decoder.done());
        REQUIRE_FALSE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    }
}

TEST_CASE("geometry_decoder with incomplete point") {
    container g = {9, 50, 34};

    SECTION("half a point") {
        g.pop_back();
    }

    SECTION("missing point") {
        g.pop_back();
        g.pop_back();
    }

    vtzero::detail::geometry_decoder<iterator> decoder{g.cbegin(), g.cend(), 100};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE_THROWS_AS(decoder.next_point(), const vtzero::geometry_exception&);
}

TEST_CASE("geometry_decoder with multipoint") {
    const container g = {17, 10, 14, 3, 9};

    vtzero::detail::geometry_decoder<iterator> decoder{g.cbegin(), g.cend(), g.size() / 2};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point(5, 7));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(3, 2));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
}

TEST_CASE("geometry_decoder with linestring") {
    const container g = {9, 4, 4, 18, 0, 16, 16, 0};

    vtzero::detail::geometry_decoder<iterator> decoder{g.cbegin(), g.cend(), g.size() / 2};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(2, 2));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point(2, 10));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(10, 10));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
}

TEST_CASE("geometry_decoder with linestring with equal points") {
    const container g = {9, 4, 4, 18, 0, 16, 0, 0};

    vtzero::detail::geometry_decoder<iterator> decoder{g.cbegin(), g.cend(), g.size() / 2};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(2, 2));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point(2, 10));
    REQUIRE(decoder.count() == 1);

    REQUIRE(decoder.next_point() == vtzero::point(2, 10));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
}

TEST_CASE("geometry_decoder with multilinestring") {
    const container g = {9, 4, 4, 18, 0, 16, 16, 0, 9, 17, 17, 10, 4, 8};

    vtzero::detail::geometry_decoder<iterator> decoder{g.cbegin(), g.cend(), g.size() / 2};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(2, 2));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point(2, 10));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(10, 10));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(1, 1));
    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(3, 5));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
}

TEST_CASE("geometry_decoder with polygon") {
    const container g = {9, 6, 12, 18, 10, 12, 24, 44, 15};

    vtzero::detail::geometry_decoder<iterator> decoder{g.cbegin(), g.cend(), g.size() / 2};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(3, 6));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point(8, 12));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(20, 34));
    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
}

TEST_CASE("geometry_decoder with polygon with wrong ClosePath count 2") {
    const container g = {9, 6, 12, 18, 10, 12, 24, 44, 23};

    vtzero::detail::geometry_decoder<iterator> decoder{g.cbegin(), g.cend(), g.size() / 2};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.next_point() == vtzero::point(3, 6));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.next_point() == vtzero::point(8, 12));
    REQUIRE(decoder.next_point() == vtzero::point(20, 34));
    REQUIRE_THROWS_AS(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH), const vtzero::geometry_exception&);
    REQUIRE_THROWS_WITH(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH), "ClosePath command count is not 1");
}

TEST_CASE("geometry_decoder with polygon with wrong ClosePath count 0") {
    const container g = {9, 6, 12, 18, 10, 12, 24, 44, 7};

    vtzero::detail::geometry_decoder<iterator> decoder{g.cbegin(), g.cend(), g.size() / 2};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.next_point() == vtzero::point(3, 6));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.next_point() == vtzero::point(8, 12));
    REQUIRE(decoder.next_point() == vtzero::point(20, 34));
    REQUIRE_THROWS_AS(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH), const vtzero::geometry_exception&);
    REQUIRE_THROWS_WITH(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH), "ClosePath command count is not 1");
}

TEST_CASE("geometry_decoder with multipolygon") {
    const container g = {9, 0, 0, 26, 20, 0, 0, 20, 19, 0, 15, 9, 22, 2, 26, 18,
                         0, 0, 18, 17, 0, 15, 9, 4, 13, 26, 0, 8, 8, 0, 0, 7, 15};

    vtzero::detail::geometry_decoder<iterator> decoder{g.cbegin(), g.cend(), g.size() / 2};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(0, 0));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 3);
    REQUIRE(decoder.next_point() == vtzero::point(10, 0));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point(10, 10));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(0, 10));
    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(11, 11));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 3);
    REQUIRE(decoder.next_point() == vtzero::point(20, 11));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point(20, 20));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(11, 20));
    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(13, 13));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 3);
    REQUIRE(decoder.next_point() == vtzero::point(13, 17));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point(17, 17));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(17, 13));
    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
}

TEST_CASE("geometry_decoder decoding linestring with int32 overflow in x coordinate") {
    const container g = {vtzero::detail::command_move_to(1),
                           protozero::encode_zigzag32(std::numeric_limits<int32_t>::max()),
                           protozero::encode_zigzag32(0),
                         vtzero::detail::command_line_to(1),
                           protozero::encode_zigzag32(1),
                           protozero::encode_zigzag32(1)
                         };

    vtzero::detail::geometry_decoder<iterator> decoder{g.cbegin(), g.cend(), g.size() / 2};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(std::numeric_limits<int32_t>::max(), 0));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(std::numeric_limits<int32_t>::min(), 1));
}

TEST_CASE("geometry_decoder decoding linestring with int32 overflow in y coordinate") {
    const container g = {vtzero::detail::command_move_to(1),
                           protozero::encode_zigzag32(0),
                           protozero::encode_zigzag32(std::numeric_limits<int32_t>::min()),
                         vtzero::detail::command_line_to(1),
                           protozero::encode_zigzag32(-1),
                           protozero::encode_zigzag32(-1)
                         };

    vtzero::detail::geometry_decoder<iterator> decoder{g.cbegin(), g.cend(), g.size() / 2};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(0, std::numeric_limits<int32_t>::min()));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(-1, std::numeric_limits<int32_t>::max()));
}

TEST_CASE("geometry_decoder with multipoint with a huge count") {
    const uint32_t huge_value = (1ul << 29u) - 1;
    const container g = {vtzero::detail::command_move_to(huge_value), 10, 10};

    vtzero::detail::geometry_decoder<iterator> decoder{g.cbegin(), g.cend(), g.size() / 2};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE_THROWS_AS(decoder.next_command(vtzero::detail::CommandId::MOVE_TO), const vtzero::geometry_exception&);
}

TEST_CASE("geometry_decoder with multipoint with not enough points") {
    const container g = {vtzero::detail::command_move_to(2), 10};

    vtzero::detail::geometry_decoder<iterator> decoder{g.cbegin(), g.cend(), 1};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE_THROWS_AS(decoder.next_command(vtzero::detail::CommandId::MOVE_TO), const vtzero::geometry_exception&);
}


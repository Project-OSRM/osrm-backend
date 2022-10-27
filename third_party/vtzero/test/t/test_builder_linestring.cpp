
#include <test.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/geometry.hpp>
#include <vtzero/index.hpp>

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

using ls_type = std::vector<std::vector<vtzero::point>>;

struct linestring_handler {

    ls_type data;

    void linestring_begin(uint32_t count) {
        data.emplace_back();
        data.back().reserve(count);
    }

    void linestring_point(const vtzero::point point) {
        data.back().push_back(point);
    }

    void linestring_end() const noexcept {
    }

};

static void test_linestring_builder(bool with_id, bool with_prop) {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    {
        vtzero::linestring_feature_builder fbuilder{lbuilder};

        if (with_id) {
            fbuilder.set_id(17);
        }

        fbuilder.add_linestring(3);
        fbuilder.set_point(10, 20);
        fbuilder.set_point(vtzero::point{20, 30});
        fbuilder.set_point(mypoint{30, 40});

        if (with_prop) {
            fbuilder.add_property("foo", "bar");
        }

        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};

    auto layer = tile.next_layer();
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = layer.next_feature();
    REQUIRE(feature.id() == (with_id ? 17 : 0));

    linestring_handler handler;
    vtzero::decode_linestring_geometry(feature.geometry(), handler);

    const ls_type result = {{{10, 20}, {20, 30}, {30, 40}}};
    REQUIRE(handler.data == result);
}

TEST_CASE("linestring builder without id/without properties") {
    test_linestring_builder(false, false);
}

TEST_CASE("linestring builder without id/with properties") {
    test_linestring_builder(false, true);
}

TEST_CASE("linestring builder with id/without properties") {
    test_linestring_builder(true, false);
}

TEST_CASE("linestring builder with id/with properties") {
    test_linestring_builder(true, true);
}

TEST_CASE("Calling add_linestring() with bad values throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::linestring_feature_builder fbuilder{lbuilder};

    SECTION("0") {
        REQUIRE_THROWS_AS(fbuilder.add_linestring(0), const assert_error&);
    }
    SECTION("1") {
        REQUIRE_THROWS_AS(fbuilder.add_linestring(1), const assert_error&);
    }
    SECTION("2^29") {
        REQUIRE_THROWS_AS(fbuilder.add_linestring(1ul << 29u), const assert_error&);
    }
}

static void test_multilinestring_builder(bool with_id, bool with_prop) {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::linestring_feature_builder fbuilder{lbuilder};

    if (with_id) {
        fbuilder.set_id(17);
    }

    fbuilder.add_linestring(3);
    fbuilder.set_point(10, 20);
    fbuilder.set_point(vtzero::point{20, 30});
    fbuilder.set_point(mypoint{30, 40});

    fbuilder.add_linestring(2);
    fbuilder.set_point(1, 2);
    fbuilder.set_point(2, 1);

    if (with_prop) {
        fbuilder.add_property("foo", vtzero::encoded_property_value{"bar"});
    }

    fbuilder.commit();

    const std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};

    auto layer = tile.next_layer();
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = layer.next_feature();
    REQUIRE(feature.id() == (with_id ? 17 : 0));

    linestring_handler handler;
    vtzero::decode_linestring_geometry(feature.geometry(), handler);

    const ls_type result = {{{10, 20}, {20, 30}, {30, 40}}, {{1, 2}, {2, 1}}};
    REQUIRE(handler.data == result);
}


TEST_CASE("Multilinestring builder without id/without properties") {
    test_multilinestring_builder(false, false);
}

TEST_CASE("Multilinestring builder without id/with properties") {
    test_multilinestring_builder(false, true);
}

TEST_CASE("Multilinestring builder with id/without properties") {
    test_multilinestring_builder(true, false);
}

TEST_CASE("Multilinestring builder with id/with properties") {
    test_multilinestring_builder(true, true);
}

TEST_CASE("Calling add_linestring() twice throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::linestring_feature_builder fbuilder{lbuilder};

    fbuilder.add_linestring(3);
    REQUIRE_ASSERT(fbuilder.add_linestring(2));
}

TEST_CASE("Calling linestring_feature_builder::set_point() throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::linestring_feature_builder fbuilder{lbuilder};

    REQUIRE_THROWS_AS(fbuilder.set_point(10, 10), const assert_error&);
}

TEST_CASE("Calling linestring_feature_builder::set_point() with same point throws") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::linestring_feature_builder fbuilder{lbuilder};

    fbuilder.add_linestring(2);
    fbuilder.set_point(10, 10);
    REQUIRE_THROWS_AS(fbuilder.set_point(10, 10), const vtzero::geometry_exception&);
}

TEST_CASE("Calling linestring_feature_builder::set_point() too often throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::linestring_feature_builder fbuilder{lbuilder};

    fbuilder.add_linestring(2);
    fbuilder.set_point(10, 20);
    fbuilder.set_point(20, 20);
    REQUIRE_THROWS_AS(fbuilder.set_point(30, 20), const assert_error&);
}

TEST_CASE("Add linestring from container") {
    const ls_type points = {{{10, 20}, {20, 30}, {30, 40}}};

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    {
        vtzero::linestring_feature_builder fbuilder{lbuilder};

#if 0
        SECTION("using iterators") {
            fbuilder.add_linestring(points[0].cbegin(), points[0].cend());
        }

        SECTION("using iterators and size") {
            fbuilder.add_linestring(points[0].cbegin(), points[0].cend(), static_cast<uint32_t>(points[0].size()));
        }
#endif

        SECTION("using container directly") {
            fbuilder.add_linestring_from_container(points[0]);
        }

        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};

    auto layer = tile.next_layer();
    REQUIRE(layer);
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = layer.next_feature();

    linestring_handler handler;
    vtzero::decode_linestring_geometry(feature.geometry(), handler);

    REQUIRE(handler.data == points);
}

#if 0
TEST_CASE("Add linestring from iterator with wrong count throws assert") {
    const std::vector<vtzero::point> points = {{10, 20}, {20, 30}, {30, 40}};

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::linestring_feature_builder fbuilder{lbuilder};

    REQUIRE_THROWS_AS(fbuilder.add_linestring(points.cbegin(),
                                              points.cend(),
                                              static_cast<uint32_t>(points.size() + 1)), const assert_error&);
}
#endif

TEST_CASE("Adding several linestrings with feature rollback in the middle") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    {
        vtzero::linestring_feature_builder fbuilder{lbuilder};
        fbuilder.set_id(1);
        fbuilder.add_linestring(2);
        fbuilder.set_point(10, 10);
        fbuilder.set_point(20, 20);
        fbuilder.commit();
    }

    try {
        vtzero::linestring_feature_builder fbuilder{lbuilder};
        fbuilder.set_id(2);
        fbuilder.add_linestring(2);
        fbuilder.set_point(10, 10);
        fbuilder.set_point(10, 10);
        fbuilder.commit();
    } catch (vtzero::geometry_exception&) {
    }

    {
        vtzero::linestring_feature_builder fbuilder{lbuilder};
        fbuilder.set_id(3);
        fbuilder.add_linestring(2);
        fbuilder.set_point(10, 20);
        fbuilder.set_point(20, 10);
        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};

    auto layer = tile.next_layer();
    REQUIRE(layer);
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.num_features() == 2);

    auto feature = layer.next_feature();
    REQUIRE(feature.id() == 1);
    feature = layer.next_feature();
    REQUIRE(feature.id() == 3);
}


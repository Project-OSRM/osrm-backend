
#include <test.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/geometry.hpp>
#include <vtzero/index.hpp>

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

struct point_handler {

    std::vector<vtzero::point> data;

    void points_begin(uint32_t count) {
        data.reserve(count);
    }

    void points_point(const vtzero::point point) {
        data.push_back(point);
    }

    void points_end() const noexcept {
    }

};

static void test_point_builder(bool with_id, bool with_prop) {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    {
        vtzero::point_feature_builder fbuilder{lbuilder};

        if (with_id) {
            fbuilder.set_id(17);
        }

        SECTION("add point using coordinates / property using key/value") {
            fbuilder.add_point(10, 20);
            if (with_prop) {
                fbuilder.add_property("foo", vtzero::encoded_property_value{"bar"});
            }
        }

        SECTION("add point using vtzero::point / property using key/value") {
            fbuilder.add_point(vtzero::point{10, 20});
            if (with_prop) {
                fbuilder.add_property("foo", vtzero::encoded_property_value{22});
            }
        }

        SECTION("add point using mypoint / property using property") {
            vtzero::encoded_property_value pv{3.5};
            vtzero::property p{"foo", vtzero::property_value{pv.data()}};
            fbuilder.add_point(mypoint{10, 20});
            if (with_prop) {
                fbuilder.add_property(p);
            }
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

    point_handler handler;
    vtzero::decode_point_geometry(feature.geometry(), handler);

    const std::vector<vtzero::point> result = {{10, 20}};
    REQUIRE(handler.data == result);
}

TEST_CASE("Point builder without id/without properties") {
    test_point_builder(false, false);
}

TEST_CASE("Point builder without id/with properties") {
    test_point_builder(false, true);
}

TEST_CASE("Point builder with id/without properties") {
    test_point_builder(true, false);
}

TEST_CASE("Point builder with id/with properties") {
    test_point_builder(true, true);
}

TEST_CASE("Calling add_points() with bad values throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_feature_builder fbuilder{lbuilder};

    SECTION("0") {
        REQUIRE_THROWS_AS(fbuilder.add_points(0), const assert_error&);
    }
    SECTION("2^29") {
        REQUIRE_THROWS_AS(fbuilder.add_points(1ul << 29u), const assert_error&);
    }
}

static void test_multipoint_builder(bool with_id, bool with_prop) {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_feature_builder fbuilder{lbuilder};

    if (with_id) {
        fbuilder.set_id(17);
    }

    fbuilder.add_points(3);
    fbuilder.set_point(10, 20);
    fbuilder.set_point(vtzero::point{20, 30});
    fbuilder.set_point(mypoint{30, 40});

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

    point_handler handler;
    vtzero::decode_point_geometry(feature.geometry(), handler);

    const std::vector<vtzero::point> result = {{10, 20}, {20, 30}, {30, 40}};
    REQUIRE(handler.data == result);
}


TEST_CASE("Multipoint builder without id/without properties") {
    test_multipoint_builder(false, false);
}

TEST_CASE("Multipoint builder without id/with properties") {
    test_multipoint_builder(false, true);
}

TEST_CASE("Multipoint builder with id/without properties") {
    test_multipoint_builder(true, false);
}

TEST_CASE("Multipoint builder with id/with properties") {
    test_multipoint_builder(true, true);
}

TEST_CASE("Calling add_point() and then other geometry functions throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_feature_builder fbuilder{lbuilder};

    fbuilder.add_point(10, 20);

    SECTION("add_point()") {
        REQUIRE_THROWS_AS(fbuilder.add_point(10, 20), const assert_error&);
    }
    SECTION("add_points()") {
        REQUIRE_THROWS_AS(fbuilder.add_points(2), const assert_error&);
    }
    SECTION("set_point()") {
        REQUIRE_THROWS_AS(fbuilder.set_point(10, 10), const assert_error&);
    }
}

TEST_CASE("Calling point_feature_builder::set_point() throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_feature_builder fbuilder{lbuilder};

    REQUIRE_THROWS_AS(fbuilder.set_point(10, 10), const assert_error&);
}

TEST_CASE("Calling add_points() and then other geometry functions throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_feature_builder fbuilder{lbuilder};

    fbuilder.add_points(2);

    SECTION("add_point()") {
        REQUIRE_THROWS_AS(fbuilder.add_point(10, 20), const assert_error&);
    }
    SECTION("add_points()") {
        REQUIRE_THROWS_AS(fbuilder.add_points(2), const assert_error&);
    }
}

TEST_CASE("Calling point_feature_builder::set_point() too often throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_feature_builder fbuilder{lbuilder};

    fbuilder.add_points(2);
    fbuilder.set_point(10, 20);
    fbuilder.set_point(20, 20);
    REQUIRE_THROWS_AS(fbuilder.set_point(30, 20), const assert_error&);
}

TEST_CASE("Add points from container") {
    const std::vector<vtzero::point> points = {{10, 20}, {20, 30}, {30, 40}};

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    {
        vtzero::point_feature_builder fbuilder{lbuilder};

/*        SECTION("using iterators") {
            fbuilder.add_points(points.cbegin(), points.cend());
        }

        SECTION("using iterators and size") {
            fbuilder.add_points(points.cbegin(), points.cend(), static_cast<uint32_t>(points.size()));
        }*/

        SECTION("using container directly") {
            fbuilder.add_points_from_container(points);
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

    point_handler handler;
    vtzero::decode_point_geometry(feature.geometry(), handler);

    REQUIRE(handler.data == points);
}
/*
TEST_CASE("Add points from iterator with wrong count throws assert") {
    const std::vector<vtzero::point> points = {{10, 20}, {20, 30}, {30, 40}};

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_feature_builder fbuilder{lbuilder};

    REQUIRE_THROWS_AS(fbuilder.add_points(points.cbegin(),
                                          points.cend(),
                                          static_cast<uint32_t>(points.size() + 1)), const assert_error&);
}*/


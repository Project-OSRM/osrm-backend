
#include <test.hpp>

#include <vtzero/feature.hpp>
#include <vtzero/layer.hpp>
#include <vtzero/vector_tile.hpp>

TEST_CASE("default constructed feature") {
    vtzero::feature feature{};

    REQUIRE_FALSE(feature.valid());
    REQUIRE_FALSE(feature);
    REQUIRE(feature.id() == 0);
    REQUIRE_FALSE(feature.has_id());
    REQUIRE(feature.geometry_type() == vtzero::GeomType::UNKNOWN);
    REQUIRE_ASSERT(feature.geometry());
    REQUIRE(feature.empty());
    REQUIRE(feature.num_properties() == 0);
}

TEST_CASE("read a feature") {
    const auto data = load_test_tile();
    vtzero::vector_tile tile{data};

    auto layer = tile.get_layer_by_name("bridge");
    REQUIRE(layer.valid());

    auto feature = layer.next_feature();
    REQUIRE(feature.valid());
    REQUIRE(feature);
    REQUIRE(feature.id() == 0);
    REQUIRE(feature.has_id());
    REQUIRE(feature.geometry_type() == vtzero::GeomType::LINESTRING);
    REQUIRE_FALSE(feature.empty());
    REQUIRE(feature.num_properties() == 4);
}

TEST_CASE("iterate over all properties of a feature") {
    const auto data = load_test_tile();
    vtzero::vector_tile tile{data};
    auto layer = tile.get_layer_by_name("bridge");
    auto feature = layer.next_feature();

    int count = 0;
    SECTION("external iterator") {
        while (auto p = feature.next_property()) {
            ++count;
            if (p.key() == "type") {
                REQUIRE(p.value().type() == vtzero::property_value_type::string_value);
                REQUIRE(p.value().string_value() == "primary");
            }
        }
    }

    SECTION("internal iterator") {
        feature.for_each_property([&count](const vtzero::property& p) {
            ++count;
            if (p.key() == "type") {
                REQUIRE(p.value().type() == vtzero::property_value_type::string_value);
                REQUIRE(p.value().string_value() == "primary");
            }
            return true;
        });
    }

    REQUIRE(count == 4);
}

TEST_CASE("iterate over some properties of a feature") {
    const auto data = load_test_tile();
    vtzero::vector_tile tile{data};
    auto layer = tile.get_layer_by_name("bridge");
    REQUIRE(layer.valid());

    auto feature = layer.next_feature();
    REQUIRE(feature.valid());

    int count = 0;
    SECTION("external iterator") {
        while (auto p = feature.next_property()) {
            ++count;
            if (p.key() == "oneway") {
                break;
            }
        }
    }

    SECTION("internal iterator") {
        feature.for_each_property([&count](const vtzero::property& p) {
            ++count;
            return p.key() != "oneway";
        });
    }

    REQUIRE(count == 2);
}


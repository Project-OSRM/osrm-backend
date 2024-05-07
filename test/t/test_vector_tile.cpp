
#include <test.hpp>

#include <vtzero/vector_tile.hpp>

#include <string>
#include <vector>

TEST_CASE("open a vector tile with string") {
    const auto data = load_test_tile();
    REQUIRE(vtzero::is_vector_tile(data));
    vtzero::vector_tile tile{data};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 12);
}

TEST_CASE("open a vector tile with data_view") {
    const auto data = load_test_tile();
    const vtzero::data_view dv{data};
    vtzero::vector_tile tile{dv};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 12);
}

TEST_CASE("open a vector tile with pointer and size") {
    const auto data = load_test_tile();
    vtzero::vector_tile tile{data.data(), data.size()};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 12);
}

TEST_CASE("get layer by index") {
    const auto data = load_test_tile();
    vtzero::vector_tile tile{data};

    auto layer = tile.get_layer(0);
    REQUIRE(layer);
    REQUIRE(layer.name() == "landuse");

    layer = tile.get_layer(1);
    REQUIRE(layer);
    REQUIRE(layer.name() == "waterway");

    layer = tile.get_layer(11);
    REQUIRE(layer);
    REQUIRE(layer.name() == "waterway_label");

    layer = tile.get_layer(12);
    REQUIRE_FALSE(layer);
}

TEST_CASE("get layer by name") {
    const auto data = load_test_tile();
    vtzero::vector_tile tile{data};

    auto layer = tile.get_layer_by_name("landuse");
    REQUIRE(layer);
    REQUIRE(layer.name() == "landuse");

    layer = tile.get_layer_by_name(std::string{"road"});
    REQUIRE(layer);
    REQUIRE(layer.name() == "road");

    const vtzero::data_view name{"poi_label"};
    layer = tile.get_layer_by_name(name);
    REQUIRE(layer);
    REQUIRE(layer.name() == "poi_label");

    layer = tile.get_layer_by_name("unknown");
    REQUIRE_FALSE(layer);
}

TEST_CASE("iterate over layers") {
    const auto data = load_test_tile();
    vtzero::vector_tile tile{data};

    std::vector<std::string> names;

    SECTION("external iterator") {
        while (auto layer = tile.next_layer()) {
            names.emplace_back(layer.name());
        }
    }

    SECTION("internal iterator") {
        const bool done = tile.for_each_layer([&names](const vtzero::layer& layer) {
            names.emplace_back(layer.name());
            return true;
        });
        REQUIRE(done);
    }

    REQUIRE(names.size() == 12);

    static std::vector<std::string> expected = {
        "landuse", "waterway", "water", "barrier_line", "building", "road",
        "bridge", "place_label", "water_label", "poi_label", "road_label",
        "waterway_label"
    };

    REQUIRE(names == expected);

    tile.reset_layer();
    int num = 0;
    while (auto layer = tile.next_layer()) {
        ++num;
    }
    REQUIRE(num == 12);
}

TEST_CASE("iterate over some of the layers") {
    const auto data = load_test_tile();
    vtzero::vector_tile tile{data};

    int num_layers = 0;

    SECTION("external iterator") {
        while (auto layer = tile.next_layer()) {
            ++num_layers;
            if (layer.name() == "water") {
                break;
            }
        }
    }

    SECTION("internal iterator") {
        const bool done = tile.for_each_layer([&num_layers](const vtzero::layer& layer) noexcept {
            ++num_layers;
            return layer.name() != "water";
        });
        REQUIRE_FALSE(done);
    }

    REQUIRE(num_layers == 3);
}


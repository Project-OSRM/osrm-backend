
#include <test.hpp>

#include <string>
#include <vector>

// Input data.vector is encoded according to
// https://github.com/mapbox/mapnik-vector-tile/blob/master/proto/vector_tile.proto

static std::string get_name(protozero::pbf_reader layer) { // copy!
    while (layer.next(1)) { // required string name
        return layer.get_string();
    }
    REQUIRE(false); // should never be here
    return "";
}

TEST_CASE("reading vector tiles") {
    static const std::vector<std::string> expected_layer_names = {
        "landuse", "waterway", "water", "aeroway", "barrier_line", "building",
        "landuse_overlay", "tunnel", "road", "bridge", "admin",
        "country_label_line", "country_label", "marine_label", "state_label",
        "place_label", "water_label", "area_label", "rail_station_label",
        "airport_label", "road_label", "waterway_label", "building_label"
    };

    const std::string buffer = load_data("vector_tile/data.vector");
    protozero::pbf_reader item{buffer};
    std::vector<std::string> layer_names;

    SECTION("iterate over message using next()") {
        while (item.next()) {
            if (item.tag() == 3) { // repeated message Layer
                protozero::pbf_reader layer{item.get_message()};
                while (layer.next()) {
                    switch (layer.tag()) {
                        case 1: // required string name
                            layer_names.push_back(layer.get_string());
                            break;
                        default:
                            layer.skip();
                    }
                }
            } else {
                item.skip();
                REQUIRE(false); // should never be here
            }
        }

        REQUIRE(layer_names == expected_layer_names);
    }

    SECTION("iterate over message using next(tag)") {
        while (item.next(3)) { // repeated message Layer
            protozero::pbf_reader layermsg{item.get_message()};
            while (layermsg.next(1)) { // required string name
                layer_names.push_back(layermsg.get_string());
            }
        }

        REQUIRE(layer_names == expected_layer_names);
    }

    SECTION("iterate over message using next(tag, type)") {
        while (item.next(3, protozero::pbf_wire_type::length_delimited)) { // repeated message Layer
            protozero::pbf_reader layermsg{item.get_message()};
            while (layermsg.next(1, protozero::pbf_wire_type::length_delimited)) { // required string name
                layer_names.push_back(layermsg.get_string());
            }
        }

        REQUIRE(layer_names == expected_layer_names);
    }

    SECTION("iterate over features in road layer") {
        int n=0;
        int n_id = 0;
        int n_geomtype = 0;
        while (item.next(3)) { // repeated message Layer
            protozero::pbf_reader layer{item.get_message()};
            std::string name = get_name(layer);
            if (name == "road") {
                while (layer.next(2)) { // repeated Feature
                    ++n;
                    protozero::pbf_reader feature{layer.get_message()};
                    while (feature.next()) {
                        switch (feature.tag()) {
                            case 1: { // optional uint64 id
                                const auto id = feature.get_uint64();
                                REQUIRE(id >=   1ULL);
                                REQUIRE(id <= 504ULL);
                                ++n_id;
                                break;
                            }
                            case 3: { // optional GeomType
                                const auto geom_type = feature.get_uint32();
                                REQUIRE(geom_type >= 1UL);
                                REQUIRE(geom_type <= 3UL);
                                ++n_geomtype;
                                break;
                            }
                            default:
                                feature.skip();
                        }
                    }
                }
            }
        }

        REQUIRE(n == 502);
        REQUIRE(n_id == 502);
        REQUIRE(n_geomtype == 502);
    }

    SECTION("iterate over features in road layer using tag_and_type") {
        int n=0;
        int n_id = 0;
        int n_geomtype = 0;
        while (item.next(3)) { // repeated message Layer
            protozero::pbf_reader layer{item.get_message()};
            std::string name = get_name(layer);
            if (name == "road") {
                while (layer.next(2)) { // repeated Feature
                    ++n;
                    protozero::pbf_reader feature{layer.get_message()};
                    while (feature.next()) {
                        switch (feature.tag_and_type()) {
                            case protozero::tag_and_type(1, protozero::pbf_wire_type::varint): { // optional uint64 id
                                const auto id = feature.get_uint64();
                                REQUIRE(id >=   1ULL);
                                REQUIRE(id <= 504ULL);
                                ++n_id;
                                break;
                            }
                            case protozero::tag_and_type(3, protozero::pbf_wire_type::varint): { // optional GeomType
                                const auto geom_type = feature.get_uint32();
                                REQUIRE(geom_type >= 1UL);
                                REQUIRE(geom_type <= 3UL);
                                ++n_geomtype;
                                break;
                            }
                            default:
                                feature.skip();
                        }
                    }
                }
            }
        }

        REQUIRE(n == 502);
        REQUIRE(n_id == 502);
        REQUIRE(n_geomtype == 502);
    }
}


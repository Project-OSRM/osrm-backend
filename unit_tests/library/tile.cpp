#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "fixture.hpp"

#include "osrm/tile_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

#include "util/typedefs.hpp"
#include "util/vector_tile.hpp"

#include <boost/variant.hpp>
#include <vtzero/vector_tile.hpp>

#include <map>

#define CHECK_EQUAL_RANGE(R1, R2)                                                                  \
    BOOST_CHECK_EQUAL_COLLECTIONS(R1.begin(), R1.end(), R2.begin(), R2.end());

BOOST_AUTO_TEST_SUITE(tile)

using variant_type = boost::variant<std::string, float, double, int64_t, uint64_t, bool>;

std::string to_string(const protozero::data_view &view)
{
    return std::string{view.data(), view.size()};
}

void validate_feature_layer(vtzero::layer layer)
{
    BOOST_CHECK_EQUAL(layer.version(), 2);
    BOOST_CHECK_EQUAL(to_string(layer.name()), "speeds");
    BOOST_CHECK_EQUAL(layer.extent(), osrm::util::vector_tile::EXTENT);
    BOOST_CHECK_EQUAL(layer.key_table().size(), 8);
    BOOST_CHECK(layer.num_features() > 2500);

    while (auto feature = layer.next_feature())
    {
        BOOST_CHECK(feature.has_id());
        BOOST_CHECK(feature.geometry_type() == vtzero::GeomType::LINESTRING);
        BOOST_CHECK(!feature.empty());

        auto props = vtzero::create_properties_map<std::map<std::string, variant_type>>(feature);

        BOOST_CHECK(props.find("speed") != props.end());
        BOOST_CHECK(props["speed"].type() == typeid(uint64_t));

        BOOST_CHECK(props.find("rate") != props.end());
        BOOST_CHECK(props["rate"].type() == typeid(double));

        BOOST_CHECK(props.find("weight") != props.end());
        BOOST_CHECK(props["weight"].type() == typeid(double));

        BOOST_CHECK(props.find("duration") != props.end());
        BOOST_CHECK(props["duration"].type() == typeid(double));

        BOOST_CHECK(props.find("is_small") != props.end());
        BOOST_CHECK(props["is_small"].type() == typeid(bool));

        BOOST_CHECK(props.find("is_startpoint") != props.end());
        BOOST_CHECK(props["is_startpoint"].type() == typeid(bool));

        BOOST_CHECK(props.find("datasource") != props.end());
        BOOST_CHECK(props["datasource"].type() == typeid(std::string));

        BOOST_CHECK(props.find("name") != props.end());
        BOOST_CHECK(props["name"].type() == typeid(std::string));
    }

    auto number_of_uint_values =
        std::count_if(layer.value_table().begin(), layer.value_table().end(), [](auto v) {
            return v.type() == vtzero::property_value_type::uint_value;
        });
    BOOST_CHECK_EQUAL(number_of_uint_values, 78);
}

void validate_turn_layer(vtzero::layer layer)
{
    BOOST_CHECK_EQUAL(layer.version(), 2);
    BOOST_CHECK_EQUAL(to_string(layer.name()), "turns");
    BOOST_CHECK_EQUAL(layer.extent(), osrm::util::vector_tile::EXTENT);
    BOOST_CHECK_EQUAL(layer.key_table().size(), 6);
    BOOST_CHECK(layer.num_features() > 700);

    while (auto feature = layer.next_feature())
    {
        BOOST_CHECK(feature.has_id());
        BOOST_CHECK(feature.geometry_type() == vtzero::GeomType::POINT);
        BOOST_CHECK(!feature.empty());

        auto props = vtzero::create_properties_map<std::map<std::string, variant_type>>(feature);

        BOOST_CHECK(props.find("bearing_in") != props.end());
        BOOST_CHECK(props["bearing_in"].type() == typeid(std::int64_t));

        BOOST_CHECK(props.find("turn_angle") != props.end());
        BOOST_CHECK(props["turn_angle"].type() == typeid(std::int64_t));

        BOOST_CHECK(props.find("weight") != props.end());
        BOOST_CHECK(props["weight"].type() == typeid(float));

        BOOST_CHECK(props.find("cost") != props.end());
        BOOST_CHECK(props["cost"].type() == typeid(float));

        BOOST_CHECK(props.find("type") != props.end());
        BOOST_CHECK(props["type"].type() == typeid(std::string));

        BOOST_CHECK(props.find("modifier") != props.end());
        BOOST_CHECK(props["modifier"].type() == typeid(std::string));
    }

    auto number_of_float_values =
        std::count_if(layer.value_table().begin(), layer.value_table().end(), [](auto v) {
            return v.type() == vtzero::property_value_type::float_value;
        });

    BOOST_CHECK_EQUAL(number_of_float_values, 74);
}

void validate_node_layer(vtzero::layer layer)
{
    BOOST_CHECK_EQUAL(layer.version(), 2);
    BOOST_CHECK_EQUAL(to_string(layer.name()), "osmnodes");
    BOOST_CHECK_EQUAL(layer.extent(), osrm::util::vector_tile::EXTENT);
    BOOST_CHECK_EQUAL(layer.key_table().size(), 0);
    BOOST_CHECK_EQUAL(layer.num_features(), 1810);

    while (auto feature = layer.next_feature())
    {
        BOOST_CHECK(feature.has_id());
        BOOST_CHECK(feature.geometry_type() == vtzero::GeomType::POINT);
        BOOST_CHECK(feature.empty());
    }
}

void validate_internal_nodes_layer(vtzero::layer layer)
{
    BOOST_CHECK_EQUAL(layer.version(), 2);
    BOOST_CHECK_EQUAL(to_string(layer.name()), "internal-nodes");
    BOOST_CHECK_EQUAL(layer.extent(), osrm::util::vector_tile::EXTENT);
    BOOST_CHECK_EQUAL(layer.key_table().size(), 0);
    BOOST_CHECK_EQUAL(layer.num_features(), 24);

    while (auto feature = layer.next_feature())
    {
        BOOST_CHECK(!feature.has_id());
        BOOST_CHECK(feature.geometry_type() == vtzero::GeomType::LINESTRING);
        BOOST_CHECK(feature.empty());
    }
}

void validate_tile(const osrm::OSRM &osrm)
{
    using namespace osrm;

    // This tile should contain most of monaco
    TileParameters params{17059, 11948, 15};

    engine::api::ResultT result = std::string();

    const auto rc = osrm.Tile(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &str_result = result.get<std::string>();
    BOOST_CHECK(str_result.size() > 114000);

    vtzero::vector_tile tile{str_result};

    validate_feature_layer(tile.next_layer());
    validate_turn_layer(tile.next_layer());
    validate_node_layer(tile.next_layer());
    validate_internal_nodes_layer(tile.next_layer());
}

BOOST_AUTO_TEST_CASE(test_tile_ch)
{
    using namespace osrm;
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm", osrm::EngineConfig::Algorithm::CH);
    validate_tile(osrm);
}

BOOST_AUTO_TEST_CASE(test_tile_corech)
{
    // Note: this tests that given the CoreCH algorithm config option, configuration falls back to
    // CH and is compatible with CH data
    using namespace osrm;
    auto osrm =
        getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm", osrm::EngineConfig::Algorithm::CoreCH);
    validate_tile(osrm);
}

BOOST_AUTO_TEST_CASE(test_tile_mld)
{
    using namespace osrm;
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/mld/monaco.osrm", osrm::EngineConfig::Algorithm::MLD);
    validate_tile(osrm);
}

void test_tile_turns(const osrm::OSRM &osrm)
{
    using namespace osrm;

    // Small tile where we can test all the values
    TileParameters params{272953, 191177, 19};

    engine::api::ResultT result = std::string();
    const auto rc = osrm.Tile(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &str_result = result.get<std::string>();
    BOOST_CHECK_GT(str_result.size(), 128);

    vtzero::vector_tile tile{str_result};

    tile.next_layer();
    auto layer = tile.next_layer();
    BOOST_CHECK_EQUAL(to_string(layer.name()), "turns");

    std::vector<float> actual_time_turn_penalties;
    std::vector<float> actual_weight_turn_penalties;
    std::vector<std::string> actual_turn_types;
    std::vector<std::string> actual_turn_modifiers;
    std::vector<std::int64_t> actual_turn_angles;
    std::vector<std::int64_t> actual_turn_bearings;

    while (auto feature = layer.next_feature())
    {
        auto props = vtzero::create_properties_map<std::map<std::string, variant_type>>(feature);

        BOOST_CHECK(props["cost"].type() == typeid(float));
        actual_time_turn_penalties.push_back(boost::get<float>(props["cost"]));
        BOOST_CHECK(props["weight"].type() == typeid(float));
        actual_weight_turn_penalties.push_back(boost::get<float>(props["weight"]));
        BOOST_CHECK(props["turn_angle"].type() == typeid(std::int64_t));
        actual_turn_angles.push_back(boost::get<std::int64_t>(props["turn_angle"]));
        BOOST_CHECK(props["bearing_in"].type() == typeid(std::int64_t));
        actual_turn_bearings.push_back(boost::get<std::int64_t>(props["bearing_in"]));
        BOOST_CHECK(props["type"].type() == typeid(std::string));
        actual_turn_types.push_back(boost::get<std::string>(props["type"]));
        BOOST_CHECK(props["modifier"].type() == typeid(std::string));
        actual_turn_modifiers.push_back(boost::get<std::string>(props["modifier"]));
    }

    // Verify that we got the expected turn penalties
    std::sort(actual_time_turn_penalties.begin(), actual_time_turn_penalties.end());
    const std::vector<float> expected_time_turn_penalties = {
        0, 0, 0, 0, 0, 0, .1f, .1f, .3f, .4f, 1.2f, 1.9f, 5.3f, 5.5f, 5.8f, 7.1f, 7.2f, 7.2f};
    CHECK_EQUAL_RANGE(actual_time_turn_penalties, expected_time_turn_penalties);

    // Verify that we got the expected turn penalties
    std::sort(actual_weight_turn_penalties.begin(), actual_weight_turn_penalties.end());
    const std::vector<float> expected_weight_turn_penalties = {
        0, 0, 0, 0, 0, 0, .1f, .1f, .3f, .4f, 1.2f, 1.9f, 5.3f, 5.5f, 5.8f, 7.1f, 7.2f, 7.2f};
    CHECK_EQUAL_RANGE(actual_weight_turn_penalties, expected_weight_turn_penalties);

    // Verify that we got the expected turn types
    std::sort(actual_turn_types.begin(), actual_turn_types.end());
    const std::vector<std::string> expected_turn_types = {"(noturn)",
                                                          "(noturn)",
                                                          "(noturn)",
                                                          "(noturn)",
                                                          "(suppressed)",
                                                          "(suppressed)",
                                                          "end of road",
                                                          "end of road",
                                                          "fork",
                                                          "fork",
                                                          "turn",
                                                          "turn",
                                                          "turn",
                                                          "turn",
                                                          "turn",
                                                          "turn",
                                                          "turn",
                                                          "turn"};
    CHECK_EQUAL_RANGE(actual_turn_types, expected_turn_types);

    // Verify that we got the expected turn modifiers
    std::sort(actual_turn_modifiers.begin(), actual_turn_modifiers.end());
    const std::vector<std::string> expected_turn_modifiers = {"left",
                                                              "left",
                                                              "left",
                                                              "left",
                                                              "right",
                                                              "right",
                                                              "right",
                                                              "right",
                                                              "sharp left",
                                                              "sharp right",
                                                              "slight left",
                                                              "slight left",
                                                              "slight right",
                                                              "slight right",
                                                              "straight",
                                                              "straight",
                                                              "straight",
                                                              "straight"};
    CHECK_EQUAL_RANGE(actual_turn_modifiers, expected_turn_modifiers);

    // Verify the expected turn angles
    std::sort(actual_turn_angles.begin(), actual_turn_angles.end());
    const std::vector<std::int64_t> expected_turn_angles = {
        -122, -120, -117, -65, -57, -30, -28, -3, -2, 2, 3, 28, 30, 57, 65, 117, 120, 122};
    CHECK_EQUAL_RANGE(actual_turn_angles, expected_turn_angles);

    // Verify the expected bearings
    std::sort(actual_turn_bearings.begin(), actual_turn_bearings.end());
    const std::vector<std::int64_t> expected_turn_bearings = {
        49, 49, 107, 107, 169, 169, 171, 171, 229, 229, 257, 257, 286, 286, 349, 349, 352, 352};
    CHECK_EQUAL_RANGE(actual_turn_bearings, expected_turn_bearings);
}

BOOST_AUTO_TEST_CASE(test_tile_turns_ch)
{
    using namespace osrm;
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm", osrm::EngineConfig::Algorithm::CH);

    test_tile_turns(osrm);
}

BOOST_AUTO_TEST_CASE(test_tile_turns_corech)
{
    // Note: this tests that given the CoreCH algorithm config option, configuration falls back to
    // CH and is compatible with CH data
    using namespace osrm;
    auto osrm =
        getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm", osrm::EngineConfig::Algorithm::CoreCH);

    test_tile_turns(osrm);
}

BOOST_AUTO_TEST_CASE(test_tile_turns_mld)
{
    using namespace osrm;
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/mld/monaco.osrm", osrm::EngineConfig::Algorithm::MLD);

    test_tile_turns(osrm);
}

void test_tile_speeds(const osrm::OSRM &osrm)
{
    using namespace osrm;

    // Small tile so we can test all the values
    // TileParameters params{272953, 191177, 19};
    TileParameters params{136477, 95580, 18};

    engine::api::ResultT result = std::string();
    const auto rc = osrm.Tile(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &str_result = result.get<std::string>();
    BOOST_CHECK_GT(str_result.size(), 128);

    vtzero::vector_tile tile{str_result};

    auto layer = tile.next_layer();
    BOOST_CHECK_EQUAL(to_string(layer.name()), "speeds");

    std::vector<std::string> actual_names;
    while (auto feature = layer.next_feature())
    {
        auto props = vtzero::create_properties_map<std::map<std::string, variant_type>>(feature);

        BOOST_CHECK(props["name"].type() == typeid(std::string));
        actual_names.push_back(boost::get<std::string>(props["name"]));
    }
    std::sort(actual_names.begin(), actual_names.end());
    const std::vector<std::string> expected_names = {"Avenue du Carnier",
                                                     "Avenue du Carnier",
                                                     "Avenue du Carnier",
                                                     "Avenue du Carnier",
                                                     "Avenue du Carnier",
                                                     "Avenue du Maréchal Foch",
                                                     "Avenue du Maréchal Foch",
                                                     "Avenue du Maréchal Foch",
                                                     "Avenue du Maréchal Foch",
                                                     "Avenue du Maréchal Foch",
                                                     "Avenue du Maréchal Foch",
                                                     "Avenue du Professeur Langevin",
                                                     "Avenue du Professeur Langevin",
                                                     "Avenue du Professeur Langevin",
                                                     "Montée de la Crémaillère",
                                                     "Montée de la Crémaillère",
                                                     "Rue Jules Ferry",
                                                     "Rue Jules Ferry",
                                                     "Rue Professeur Calmette",
                                                     "Rue Professeur Calmette"};
    BOOST_CHECK(actual_names == expected_names);
}

BOOST_AUTO_TEST_CASE(test_tile_speeds_ch)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm", osrm::EngineConfig::Algorithm::CH);
    test_tile_speeds(osrm);
}

BOOST_AUTO_TEST_CASE(test_tile_speeds_corech)
{
    // Note: this tests that given the CoreCH algorithm config option, configuration falls back to
    // CH and is compatible with CH data
    using namespace osrm;

    auto osrm =
        getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm", osrm::EngineConfig::Algorithm::CoreCH);
    test_tile_speeds(osrm);
}

BOOST_AUTO_TEST_CASE(test_tile_speeds_mld)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/mld/monaco.osrm", osrm::EngineConfig::Algorithm::MLD);
    test_tile_speeds(osrm);
}

void test_tile_nodes(const osrm::OSRM &osrm)
{
    using namespace osrm;

    // Small tile so we can test all the values
    // TileParameters params{272953, 191177, 19};
    // TileParameters params{136477, 95580, 18};
    // Small tile where we can test all the values
    TileParameters params{272953, 191177, 19};

    engine::api::ResultT result = std::string();
    const auto rc = osrm.Tile(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &str_result = result.get<std::string>();
    BOOST_CHECK_GT(str_result.size(), 128);

    vtzero::vector_tile tile{str_result};

    tile.next_layer();
    tile.next_layer();
    auto layer = tile.next_layer();
    BOOST_CHECK_EQUAL(to_string(layer.name()), "osmnodes");

    std::vector<OSMNodeID::value_type> found_node_ids;
    while (auto feature = layer.next_feature())
    {
        found_node_ids.push_back(feature.id());
    }

    std::sort(found_node_ids.begin(), found_node_ids.end());
    const std::vector<OSMNodeID::value_type> expected_node_ids = {
        25191722, 25191725, 357300400, 1737389138, 1737389140, 2241375220};
    BOOST_CHECK(found_node_ids == expected_node_ids);
}

BOOST_AUTO_TEST_CASE(test_tile_nodes_ch)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm", osrm::EngineConfig::Algorithm::CH);
    test_tile_nodes(osrm);
}

BOOST_AUTO_TEST_CASE(test_tile_nodes_corech)
{
    // Note: this tests that given the CoreCH algorithm config option, configuration falls back to
    // CH and is compatible with CH data
    using namespace osrm;

    auto osrm =
        getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm", osrm::EngineConfig::Algorithm::CoreCH);
    test_tile_nodes(osrm);
}

BOOST_AUTO_TEST_CASE(test_tile_nodes_mld)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/mld/monaco.osrm", osrm::EngineConfig::Algorithm::MLD);
    test_tile_nodes(osrm);
}

BOOST_AUTO_TEST_SUITE_END()

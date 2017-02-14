#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "args.hpp"
#include "fixture.hpp"

#include "osrm/tile_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

#include "util/vector_tile.hpp"

#include <protozero/pbf_reader.hpp>

BOOST_AUTO_TEST_SUITE(tile)

BOOST_AUTO_TEST_CASE(test_tile)
{
    const auto args = get_args();
    auto osrm = getOSRM(args.at(0));

    using namespace osrm;

    // This tile should contain most of monaco
    TileParameters params{17059, 11948, 15};

    std::string result;
    const auto rc = osrm.Tile(params, result);
    BOOST_CHECK(rc == Status::Ok);

    BOOST_CHECK(result.size() > 115000);

    protozero::pbf_reader tile_message(result);
    tile_message.next();
    BOOST_CHECK_EQUAL(tile_message.tag(), util::vector_tile::LAYER_TAG); // must be a layer
    protozero::pbf_reader layer_message = tile_message.get_message();

    const auto check_feature = [](protozero::pbf_reader feature_message) {
        feature_message.next(); // advance parser to first entry
        BOOST_CHECK_EQUAL(feature_message.tag(), util::vector_tile::GEOMETRY_TAG);
        BOOST_CHECK_EQUAL(feature_message.get_enum(), util::vector_tile::GEOMETRY_TYPE_LINE);

        feature_message.next(); // advance to next entry
        BOOST_CHECK_EQUAL(feature_message.tag(), util::vector_tile::ID_TAG);
        feature_message.get_uint64(); // id

        feature_message.next(); // advance to next entry
        BOOST_CHECK_EQUAL(feature_message.tag(), util::vector_tile::FEATURE_ATTRIBUTES_TAG);
        // properties
        auto property_iter_pair = feature_message.get_packed_uint32();
        auto value_begin = property_iter_pair.begin();
        auto value_end = property_iter_pair.end();
        BOOST_CHECK_EQUAL(std::distance(value_begin, value_end), 10);
        auto iter = value_begin;
        BOOST_CHECK_EQUAL(*iter++, 0); // speed key
        BOOST_CHECK_LT(*iter++, 128);  // speed value
        BOOST_CHECK_EQUAL(*iter++, 1); // component key
        // component value
        BOOST_CHECK_GE(*iter, 128);
        BOOST_CHECK_LE(*iter, 129);
        iter++;
        BOOST_CHECK_EQUAL(*iter++, 2); // data source key
        *iter++;                       // skip value check, can be valud uint32
        BOOST_CHECK_EQUAL(*iter++, 3); // duration key
        BOOST_CHECK_GT(*iter++, 130);  // duration value
        // name
        BOOST_CHECK_EQUAL(*iter++, 4);
        BOOST_CHECK_GT(*iter++, 130);
        BOOST_CHECK(iter == value_end);
        // geometry
        feature_message.next();
        auto geometry_iter_pair = feature_message.get_packed_uint32();
        BOOST_CHECK_GT(std::distance(geometry_iter_pair.begin(), geometry_iter_pair.end()), 1);
    };

    const auto check_value = [](protozero::pbf_reader value) {
        while (value.next())
        {
            switch (value.tag())
            {
            case util::vector_tile::VARIANT_TYPE_BOOL:
                value.get_bool();
                break;
            case util::vector_tile::VARIANT_TYPE_DOUBLE:
                value.get_double();
                break;
            case util::vector_tile::VARIANT_TYPE_FLOAT:
                value.get_float();
                break;
            case util::vector_tile::VARIANT_TYPE_STRING:
                value.get_string();
                break;
            case util::vector_tile::VARIANT_TYPE_UINT64:
                value.get_uint64();
                break;
            case util::vector_tile::VARIANT_TYPE_SINT64:
                value.get_sint64();
                break;
            }
        }
    };

    auto number_of_speed_keys = 0u;
    auto number_of_speed_values = 0u;

    while (layer_message.next())
    {
        switch (layer_message.tag())
        {
        case util::vector_tile::VERSION_TAG:
            BOOST_CHECK_EQUAL(layer_message.get_uint32(), 2);
            break;
        case util::vector_tile::NAME_TAG:
            BOOST_CHECK_EQUAL(layer_message.get_string(), "speeds");
            break;
        case util::vector_tile::EXTENT_TAG:
            BOOST_CHECK_EQUAL(layer_message.get_uint32(), util::vector_tile::EXTENT);
            break;
        case util::vector_tile::FEATURE_TAG:
            check_feature(layer_message.get_message());
            break;
        case util::vector_tile::KEY_TAG:
            layer_message.get_string();
            number_of_speed_keys++;
            break;
        case util::vector_tile::VARIANT_TAG:
            check_value(layer_message.get_message());
            number_of_speed_values++;
            break;
        default:
            BOOST_CHECK(false); // invalid tag
            break;
        }
    }

    BOOST_CHECK_EQUAL(number_of_speed_keys, 5);
    BOOST_CHECK_GT(number_of_speed_values, 128); // speed value resolution

    tile_message.next();
    layer_message = tile_message.get_message();

    const auto check_turn_feature = [](protozero::pbf_reader feature_message) {
        feature_message.next(); // advance parser to first entry
        BOOST_CHECK_EQUAL(feature_message.tag(), util::vector_tile::GEOMETRY_TAG);
        BOOST_CHECK_EQUAL(feature_message.get_enum(), util::vector_tile::GEOMETRY_TYPE_POINT);

        feature_message.next(); // advance to next entry
        BOOST_CHECK_EQUAL(feature_message.tag(), util::vector_tile::ID_TAG);
        feature_message.get_uint64(); // id

        feature_message.next(); // advance to next entry
        BOOST_CHECK_EQUAL(feature_message.tag(), util::vector_tile::FEATURE_ATTRIBUTES_TAG);
        // properties
        auto feature_iter_pair = feature_message.get_packed_uint32();
        BOOST_CHECK_EQUAL(std::distance(feature_iter_pair.begin(), feature_iter_pair.end()), 6);
        auto iter = feature_iter_pair.begin();
        BOOST_CHECK_EQUAL(*iter++, 0); // bearing_in key
        *iter++;
        BOOST_CHECK_EQUAL(*iter++, 1); // turn_angle key
        *iter++;
        BOOST_CHECK_EQUAL(*iter++, 2); // cost key
        *iter++;                       // skip value check, can be valud uint32
        BOOST_CHECK(iter == feature_iter_pair.end());
        // geometry
        feature_message.next();
        auto geometry_iter_pair = feature_message.get_packed_uint32();
        BOOST_CHECK_GT(std::distance(geometry_iter_pair.begin(), geometry_iter_pair.end()), 1);
    };

    auto number_of_turn_keys = 0u;
    auto number_of_turns_found = 0u;

    while (layer_message.next())
    {
        switch (layer_message.tag())
        {
        case util::vector_tile::VERSION_TAG:
            BOOST_CHECK_EQUAL(layer_message.get_uint32(), 2);
            break;
        case util::vector_tile::NAME_TAG:
            BOOST_CHECK_EQUAL(layer_message.get_string(), "turns");
            break;
        case util::vector_tile::EXTENT_TAG:
            BOOST_CHECK_EQUAL(layer_message.get_uint32(), util::vector_tile::EXTENT);
            break;
        case util::vector_tile::FEATURE_TAG:
            check_turn_feature(layer_message.get_message());
            number_of_turns_found++;
            break;
        case util::vector_tile::KEY_TAG:
            layer_message.get_string();
            number_of_turn_keys++;
            break;
        case util::vector_tile::VARIANT_TAG:
            check_value(layer_message.get_message());
            break;
        default:
            BOOST_CHECK(false); // invalid tag
            break;
        }
    }

    BOOST_CHECK_EQUAL(number_of_turn_keys, 3);
    BOOST_CHECK(number_of_turns_found > 700);
}

BOOST_AUTO_TEST_CASE(test_tile_turns)
{
    const auto args = get_args();
    auto osrm = getOSRM(args.at(0));

    using namespace osrm;

    // Small tile where we can test all the values
    TileParameters params{272953, 191177, 19};

    std::string result;
    const auto rc = osrm.Tile(params, result);
    BOOST_CHECK(rc == Status::Ok);

    BOOST_CHECK_GT(result.size(), 128);

    protozero::pbf_reader tile_message(result);
    tile_message.next();
    BOOST_CHECK_EQUAL(tile_message.tag(), util::vector_tile::LAYER_TAG); // must be a layer
    // Skip the segments layer
    tile_message.skip();

    tile_message.next();
    auto layer_message = tile_message.get_message();

    std::vector<int> found_bearing_in_indexes;
    std::vector<int> found_turn_angles_indexes;
    std::vector<int> found_penalties_indexes;

    const auto check_turn_feature = [&](protozero::pbf_reader feature_message) {
        feature_message.next(); // advance parser to first entry
        BOOST_CHECK_EQUAL(feature_message.tag(), util::vector_tile::GEOMETRY_TAG);
        BOOST_CHECK_EQUAL(feature_message.get_enum(), util::vector_tile::GEOMETRY_TYPE_POINT);

        feature_message.next(); // advance to next entry
        BOOST_CHECK_EQUAL(feature_message.tag(), util::vector_tile::ID_TAG);
        feature_message.get_uint64(); // id

        feature_message.next(); // advance to next entry
        BOOST_CHECK_EQUAL(feature_message.tag(), util::vector_tile::FEATURE_ATTRIBUTES_TAG);
        // properties
        auto feature_iter_pair = feature_message.get_packed_uint32();
        BOOST_CHECK_EQUAL(std::distance(feature_iter_pair.begin(), feature_iter_pair.end()), 6);
        auto iter = feature_iter_pair.begin();
        BOOST_CHECK_EQUAL(*iter++, 0); // bearing_in key
        found_bearing_in_indexes.push_back(*iter++);
        BOOST_CHECK_EQUAL(*iter++, 1); // turn_angle key
        found_turn_angles_indexes.push_back(*iter++);
        BOOST_CHECK_EQUAL(*iter++, 2);              // cost key
        found_penalties_indexes.push_back(*iter++); // skip value check, can be valud uint32
        BOOST_CHECK(iter == feature_iter_pair.end());
        // geometry
        feature_message.next();
        auto geometry_iter_pair = feature_message.get_packed_uint32();
        BOOST_CHECK_GT(std::distance(geometry_iter_pair.begin(), geometry_iter_pair.end()), 1);
    };

    std::unordered_map<int, float> float_vals;
    std::unordered_map<int, std::int64_t> sint64_vals;

    int kv_index = 0;

    const auto check_value = [&](protozero::pbf_reader value) {
        while (value.next())
        {
            switch (value.tag())
            {
            case util::vector_tile::VARIANT_TYPE_FLOAT:
                float_vals[kv_index] = value.get_float();
                break;
            case util::vector_tile::VARIANT_TYPE_SINT64:
                sint64_vals[kv_index] = value.get_sint64();
                break;
            default:
                BOOST_CHECK(false);
            }
            kv_index++;
        }
    };

    auto number_of_turn_keys = 0u;
    auto number_of_turns_found = 0u;

    while (layer_message.next())
    {
        switch (layer_message.tag())
        {
        case util::vector_tile::VERSION_TAG:
            BOOST_CHECK_EQUAL(layer_message.get_uint32(), 2);
            break;
        case util::vector_tile::NAME_TAG:
            BOOST_CHECK_EQUAL(layer_message.get_string(), "turns");
            break;
        case util::vector_tile::EXTENT_TAG:
            BOOST_CHECK_EQUAL(layer_message.get_uint32(), util::vector_tile::EXTENT);
            break;
        case util::vector_tile::FEATURE_TAG:
            check_turn_feature(layer_message.get_message());
            number_of_turns_found++;
            break;
        case util::vector_tile::KEY_TAG:
            layer_message.get_string();
            number_of_turn_keys++;
            break;
        case util::vector_tile::VARIANT_TAG:
            check_value(layer_message.get_message());
            break;
        default:
            BOOST_CHECK(false); // invalid tag
            break;
        }
    }

    // Verify that we got the expected turn penalties
    std::vector<float> actual_turn_penalties;
    for (const auto &i : found_penalties_indexes)
    {
        BOOST_CHECK(float_vals.count(i) == 1);
        actual_turn_penalties.push_back(float_vals[i]);
    }
    std::sort(actual_turn_penalties.begin(), actual_turn_penalties.end());
    const std::vector<float> expected_turn_penalties = {
        0, 0, 0, 0, 0, 0, .1f, .1f, .3f, .4f, 1.2f, 1.9f, 5.3f, 5.5f, 5.8f, 7.1f, 7.2f, 7.2f};
    BOOST_CHECK(actual_turn_penalties == expected_turn_penalties);

    // Verify the expected turn angles
    std::vector<std::int64_t> actual_turn_angles;
    for (const auto &i : found_turn_angles_indexes)
    {
        BOOST_CHECK(sint64_vals.count(i) == 1);
        actual_turn_angles.push_back(sint64_vals[i]);
    }
    std::sort(actual_turn_angles.begin(), actual_turn_angles.end());
    const std::vector<std::int64_t> expected_turn_angles = {
        -122, -120, -117, -65, -57, -30, -28, -3, -2, 2, 3, 28, 30, 57, 65, 117, 120, 122};
    BOOST_CHECK(actual_turn_angles == expected_turn_angles);

    // Verify the expected bearings
    std::vector<std::int64_t> actual_turn_bearings;
    for (const auto &i : found_bearing_in_indexes)
    {
        BOOST_CHECK(sint64_vals.count(i) == 1);
        actual_turn_bearings.push_back(sint64_vals[i]);
    }
    std::sort(actual_turn_bearings.begin(), actual_turn_bearings.end());
    const std::vector<std::int64_t> expected_turn_bearings = {
        49, 49, 107, 107, 169, 169, 171, 171, 229, 229, 257, 257, 286, 286, 349, 349, 352, 352};
    BOOST_CHECK(actual_turn_bearings == expected_turn_bearings);
}

BOOST_AUTO_TEST_CASE(test_tile_speeds)
{
    const auto args = get_args();
    auto osrm = getOSRM(args.at(0));

    using namespace osrm;

    // Small tile so we can test all the values
    // TileParameters params{272953, 191177, 19};
    TileParameters params{136477, 95580, 18};

    std::string result;
    const auto rc = osrm.Tile(params, result);
    BOOST_CHECK(rc == Status::Ok);

    BOOST_CHECK_GT(result.size(), 128);

    protozero::pbf_reader tile_message(result);
    tile_message.next();
    BOOST_CHECK_EQUAL(tile_message.tag(), util::vector_tile::LAYER_TAG); // must be a layer
    protozero::pbf_reader layer_message = tile_message.get_message();

    std::vector<int> found_speed_indexes;
    std::vector<int> found_component_indexes;
    std::vector<int> found_datasource_indexes;
    std::vector<int> found_duration_indexes;
    std::vector<int> found_name_indexes;

    const auto check_feature = [&](protozero::pbf_reader feature_message) {
        feature_message.next(); // advance parser to first entry
        BOOST_CHECK_EQUAL(feature_message.tag(), util::vector_tile::GEOMETRY_TAG);
        BOOST_CHECK_EQUAL(feature_message.get_enum(), util::vector_tile::GEOMETRY_TYPE_LINE);

        feature_message.next(); // advance to next entry
        BOOST_CHECK_EQUAL(feature_message.tag(), util::vector_tile::ID_TAG);
        feature_message.get_uint64(); // id

        feature_message.next(); // advance to next entry
        BOOST_CHECK_EQUAL(feature_message.tag(), util::vector_tile::FEATURE_ATTRIBUTES_TAG);
        // properties
        auto property_iter_pair = feature_message.get_packed_uint32();
        auto value_begin = property_iter_pair.begin();
        auto value_end = property_iter_pair.end();
        BOOST_CHECK_EQUAL(std::distance(value_begin, value_end), 10);
        auto iter = value_begin;
        BOOST_CHECK_EQUAL(*iter++, 0); // speed key
        found_speed_indexes.push_back(*iter++);
        BOOST_CHECK_EQUAL(*iter++, 1); // component key
        // component value
        found_component_indexes.push_back(*iter++);
        BOOST_CHECK_EQUAL(*iter++, 2); // data source key
        found_datasource_indexes.push_back(*iter++);
        BOOST_CHECK_EQUAL(*iter++, 3); // duration key
        found_duration_indexes.push_back(*iter++);
        // name
        BOOST_CHECK_EQUAL(*iter++, 4);
        found_name_indexes.push_back(*iter++);
        BOOST_CHECK(iter == value_end);
        // geometry
        feature_message.next();
        auto geometry_iter_pair = feature_message.get_packed_uint32();
        BOOST_CHECK_GT(std::distance(geometry_iter_pair.begin(), geometry_iter_pair.end()), 1);
    };

    std::unordered_map<int, std::string> string_vals;
    std::unordered_map<int, bool> bool_vals;
    std::unordered_map<int, std::uint64_t> uint64_vals;
    std::unordered_map<int, double> double_vals;

    int kv_index = 0;

    const auto check_value = [&](protozero::pbf_reader value) {
        while (value.next())
        {
            switch (value.tag())
            {
            case util::vector_tile::VARIANT_TYPE_BOOL:
                bool_vals[kv_index] = value.get_bool();
                break;
            case util::vector_tile::VARIANT_TYPE_DOUBLE:
                double_vals[kv_index] = value.get_double();
                break;
            case util::vector_tile::VARIANT_TYPE_FLOAT:
                value.get_float();
                break;
            case util::vector_tile::VARIANT_TYPE_STRING:
                string_vals[kv_index] = value.get_string();
                break;
            case util::vector_tile::VARIANT_TYPE_UINT64:
                uint64_vals[kv_index] = value.get_uint64();
                break;
            case util::vector_tile::VARIANT_TYPE_SINT64:
                value.get_sint64();
                break;
            }
            kv_index++;
        }
    };

    auto number_of_speed_keys = 0u;
    auto number_of_speed_values = 0u;

    while (layer_message.next())
    {
        switch (layer_message.tag())
        {
        case util::vector_tile::VERSION_TAG:
            BOOST_CHECK_EQUAL(layer_message.get_uint32(), 2);
            break;
        case util::vector_tile::NAME_TAG:
            BOOST_CHECK_EQUAL(layer_message.get_string(), "speeds");
            break;
        case util::vector_tile::EXTENT_TAG:
            BOOST_CHECK_EQUAL(layer_message.get_uint32(), util::vector_tile::EXTENT);
            break;
        case util::vector_tile::FEATURE_TAG:
            check_feature(layer_message.get_message());
            break;
        case util::vector_tile::KEY_TAG:
            layer_message.get_string();
            number_of_speed_keys++;
            break;
        case util::vector_tile::VARIANT_TAG:
            check_value(layer_message.get_message());
            number_of_speed_values++;
            break;
        default:
            BOOST_CHECK(false); // invalid tag
            break;
        }
    }

    std::vector<std::string> actual_names;
    for (const auto &i : found_name_indexes)
    {
        BOOST_CHECK(string_vals.count(i) == 1);
        actual_names.push_back(string_vals[i]);
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

BOOST_AUTO_TEST_SUITE_END()

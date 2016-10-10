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

    BOOST_CHECK_GT(result.size(), 128);

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
}

BOOST_AUTO_TEST_SUITE_END()

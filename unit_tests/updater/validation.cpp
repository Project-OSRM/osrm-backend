#include "util/exception.hpp"
#include "util/geojson_validation.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(geojson_validation)

using namespace osrm;

BOOST_AUTO_TEST_CASE(timezone_coordinate_validation_test)
{
    rapidjson::Document doc;
    char valid_coord[] = "[8.28369,48.88277]";
    doc.Parse(valid_coord);
    BOOST_CHECK_NO_THROW(util::validateCoordinate(doc));

    char non_array[] = "{\"x\": 48.88277}";
    doc.Parse(non_array);
    BOOST_CHECK_THROW(util::validateCoordinate(doc), util::exception);

    char too_many[] = "[8.28369, 48.88277, 8.2806]";
    doc.Parse(too_many);
    BOOST_CHECK_THROW(util::validateCoordinate(doc), util::exception);

    char nan[] = "[8.28369, y]";
    doc.Parse(nan);
    BOOST_CHECK_THROW(util::validateCoordinate(doc), util::exception);
}

BOOST_AUTO_TEST_CASE(timezone_validation_test)
{
    char json[] =
        "{ \"type\" : \"Feature\","
        "\"properties\" : { \"TZID\" : \"Europe/Berlin\"}, \"geometry\" : { \"type\": \"polygon\", "
        "\"coordinates\": [[[8.28369,48.88277], [8.57757, "
        "48.88277], [8.57757, 49.07206], [8.28369, "
        "49.07206], [8.28369, 48.88277]]] }}";
    rapidjson::Document doc;
    doc.Parse(json);
    BOOST_CHECK_NO_THROW(util::validateFeature(doc));

    char missing_type[] = "{\"properties\" : { \"TZID\" : \"Europe/Berlin\"}, \"geometry\" : { "
                          "\"type\": \"polygon\", "
                          "\"coordinates\": [[[8.28369,48.88277], [8.57757, "
                          "48.88277], [8.57757, 49.07206], [8.28369, "
                          "49.07206], [8.28369, 48.88277]]] }}";
    doc.Parse(missing_type);
    BOOST_CHECK_THROW(util::validateFeature(doc), util::exception);

    char missing_props[] =
        "{ \"type\" : \"Feature\","
        "\"props\" : { \"TZID\" : \"Europe/Berlin\"}, \"geometry\" : { \"type\": \"polygon\", "
        "\"coordinates\": [[[8.28369,48.88277], [8.57757, "
        "48.88277], [8.57757, 49.07206], [8.28369, "
        "49.07206], [8.28369, 48.88277]]] }}";
    doc.Parse(missing_props);
    BOOST_CHECK_THROW(util::validateFeature(doc), util::exception);

    char nonobj_props[] =
        "{ \"type\" : \"Feature\","
        "\"properties\" : [ \"TZID\", \"Europe/Berlin\"], \"geometry\" : { \"type\": \"polygon\", "
        "\"coordinates\": [[[8.28369,48.88277], [8.57757, "
        "48.88277], [8.57757, 49.07206], [8.28369, "
        "49.07206], [8.28369, 48.88277]]] }}";
    doc.Parse(nonobj_props);
    BOOST_CHECK_THROW(util::validateFeature(doc), util::exception);

    char missing_tzid[] = "{ \"type\" : \"Feature\","
                          "\"properties\" : { }, \"geometry\" : { \"type\": \"polygon\", "
                          "\"coordinates\": [[[8.28369,48.88277], [8.57757, "
                          "48.88277], [8.57757, 49.07206], [8.28369, "
                          "49.07206], [8.28369, 48.88277]]] }}";
    doc.Parse(missing_tzid);
    BOOST_CHECK_THROW(util::validateFeature(doc), util::exception);

    char tzid_err[] = "{ \"type\" : \"Feature\","
                      "\"properties\" : { \"TZID\" : []}, \"geometry\" : { \"type\": \"polygon\", "
                      "\"coordinates\": [[[8.28369,48.88277], [8.57757, "
                      "48.88277], [8.57757, 49.07206], [8.28369, "
                      "49.07206], [8.28369, 48.88277]]] }}";
    doc.Parse(tzid_err);
    BOOST_CHECK_THROW(util::validateFeature(doc), util::exception);

    char missing_geom[] = "{ \"type\" : \"Feature\","
                          "\"properties\" : { \"TZID\" : \"Europe/Berlin\"}, \"geometries\" : { "
                          "\"type\": \"polygon\", "
                          "\"coordinates\": [[[8.28369,48.88277], [8.57757, "
                          "48.88277], [8.57757, 49.07206], [8.28369, "
                          "49.07206], [8.28369, 48.88277]]] }}";
    doc.Parse(missing_geom);
    BOOST_CHECK_THROW(util::validateFeature(doc), util::exception);

    char nonobj_geom[] =
        "{ \"type\" : \"Feature\","
        "\"properties\" : { \"TZID\" : \"Europe/Berlin\"}, \"geometry\" : [ \"type\", \"polygon\", "
        "\"coordinates\", [[[8.28369,48.88277], [8.57757, "
        "48.88277], [8.57757, 49.07206], [8.28369, "
        "49.07206], [8.28369, 48.88277]]] ]}";
    doc.Parse(nonobj_geom);
    BOOST_CHECK_THROW(util::validateFeature(doc), util::exception);

    char missing_geom_type[] = "{ \"type\" : \"Feature\","
                               "\"properties\" : { \"TZID\" : \"Europe/Berlin\"}, \"geometry\" : { "
                               "\"no_type\": \"polygon\", "
                               "\"coordinates\": [[[8.28369,48.88277], [8.57757, "
                               "48.88277], [8.57757, 49.07206], [8.28369, "
                               "49.07206], [8.28369, 48.88277]]] }}";
    doc.Parse(missing_geom_type);
    BOOST_CHECK_THROW(util::validateFeature(doc), util::exception);

    char nonstring_geom_type[] = "{ \"type\" : \"Feature\","
                                 "\"properties\" : { \"TZID\" : \"Europe/Berlin\"}, \"geometry\" : "
                                 "{ \"type\": [\"polygon\"], "
                                 "\"coordinates\": [[[8.28369,48.88277], [8.57757, "
                                 "48.88277], [8.57757, 49.07206], [8.28369, "
                                 "49.07206], [8.28369, 48.88277]]] }}";
    doc.Parse(nonstring_geom_type);
    BOOST_CHECK_THROW(util::validateFeature(doc), util::exception);

    char missing_coords[] =
        "{ \"type\" : \"Feature\","
        "\"properties\" : { \"TZID\" : \"Europe/Berlin\"}, \"geometry\" : { \"type\": \"polygon\", "
        "\"coords\": [[[8.28369,48.88277], [8.57757, "
        "48.88277], [8.57757, 49.07206], [8.28369, "
        "49.07206], [8.28369, 48.88277]]] }}";
    doc.Parse(missing_coords);
    BOOST_CHECK_THROW(util::validateFeature(doc), util::exception);

    char missing_outerring[] =
        "{ \"type\" : \"Feature\","
        "\"properties\" : { \"TZID\" : \"Europe/Berlin\"}, \"geometry\" : { \"type\": \"polygon\", "
        "\"coordinates\": [[8.28369,48.88277], [8.57757, "
        "48.88277], [8.57757, 49.07206], [8.28369, "
        "49.07206], [8.28369, 48.88277]] }}";
    doc.Parse(missing_outerring);
    BOOST_CHECK_THROW(util::validateFeature(doc), util::exception);
}
BOOST_AUTO_TEST_SUITE_END()

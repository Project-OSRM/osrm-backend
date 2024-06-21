#include "util/exception.hpp"
#include "util/geojson_validation.hpp"
#include "util/timezones.hpp"

#include <boost/test/unit_test.hpp>

#include <filesystem>

BOOST_AUTO_TEST_SUITE(timezoner)

using namespace osrm;
using namespace osrm::updater;

BOOST_AUTO_TEST_CASE(timezoner_test)
{
    const char json[] =
        "{ \"type\" : \"FeatureCollection\", \"features\": ["
        "{ \"type\" : \"Feature\","
        "\"properties\" : { \"TZID\" : \"Europe/Berlin\"}, \"geometry\" : { \"type\": \"polygon\", "
        "\"coordinates\": [[[8.28369,48.88277], [8.57757, "
        "48.88277], [8.57757, 49.07206], [8.28369, "
        "49.07206], [8.28369, 48.88277]]] }} ]}";
    std::time_t now = time(0);
    BOOST_CHECK_NO_THROW(Timezoner tz(json, now));

    std::filesystem::path test_path(TEST_DATA_DIR "/test.geojson");
    BOOST_CHECK_NO_THROW(Timezoner tz(test_path, now));

    // missing opening bracket
    const char bad[] =
        "\"type\" : \"FeatureCollection\", \"features\": ["
        "{ \"type\" : \"Feature\","
        "\"properties\" : { \"TZID\" : \"Europe/Berlin\"}, \"geometry\" : { \"type\": \"polygon\", "
        "\"coordinates\": [[[8.28369,48.88277], [8.57757, "
        "48.88277], [8.57757, 49.07206], [8.28369, "
        "49.07206], [8.28369, 48.88277]]] }} ]}";
    BOOST_CHECK_THROW(Timezoner tz(bad, now), util::exception);

    // missing/malformed FeatureCollection type field
    const char missing_type[] =
        "{ \"FeatureCollection\", \"features\": ["
        "{ \"type\" : \"Feature\","
        "\"properties\" : { \"TZID\" : \"Europe/Berlin\"}, \"geometry\" : { \"type\": \"polygon\", "
        "\"coordinates\": [[[8.28369,48.88277], [8.57757, "
        "48.88277], [8.57757, 49.07206], [8.28369, "
        "49.07206], [8.28369, 48.88277]]] }} ]}";
    BOOST_CHECK_THROW(Timezoner tz(missing_type, now), util::exception);

    const char missing_featc[] =
        "{ \"type\" : \"Collection\", \"features\": ["
        "{ \"type\" : \"Feature\","
        "\"properties\" : { \"TZID\" : \"Europe/Berlin\"}, \"geometry\" : { \"type\": \"polygon\", "
        "\"coordinates\": [[[8.28369,48.88277], [8.57757, "
        "48.88277], [8.57757, 49.07206], [8.28369, "
        "49.07206], [8.28369, 48.88277]]] }} ]}";
    BOOST_CHECK_THROW(Timezoner tz(missing_featc, now), util::exception);

    char missing_tzid[] = "{ \"type\" : \"Feature\","
                          "\"properties\" : { }, \"geometry\" : { \"type\": \"polygon\", "
                          "\"coordinates\": [[[8.28369,48.88277], [8.57757, "
                          "48.88277], [8.57757, 49.07206], [8.28369, "
                          "49.07206], [8.28369, 48.88277]]] }}";
    BOOST_CHECK_THROW(Timezoner tz(missing_tzid, now), util::exception);

    char tzid_err[] = "{ \"type\" : \"Feature\","
                      "\"properties\" : { \"TZID\" : []}, \"geometry\" : { \"type\": \"polygon\", "
                      "\"coordinates\": [[[8.28369,48.88277], [8.57757, "
                      "48.88277], [8.57757, 49.07206], [8.28369, "
                      "49.07206], [8.28369, 48.88277]]] }}";
    BOOST_CHECK_THROW(Timezoner tz(tzid_err, now), util::exception);
}
BOOST_AUTO_TEST_SUITE_END()

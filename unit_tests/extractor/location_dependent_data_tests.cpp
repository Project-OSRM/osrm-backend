#include "extractor/location_dependent_data.hpp"

#include "../common/range_tools.hpp"

#include <boost/filesystem.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <fstream>
#include <vector>

BOOST_AUTO_TEST_SUITE(location_dependent_data_tests)

using namespace osrm;
using namespace osrm::extractor;
using point_t = LocationDependentData::point_t;

struct LocationDataFixture
{
    LocationDataFixture(const std::string &json) : temporary_file(boost::filesystem::unique_path())
    {
        std::ofstream file(temporary_file.string());
        file << json;
    }
    ~LocationDataFixture() { remove(temporary_file); }

    boost::filesystem::path temporary_file;
};

BOOST_AUTO_TEST_CASE(polygon_tests)
{
    LocationDataFixture fixture(R"json({
"type": "FeatureCollection",
"features": [
{
    "type": "Feature",
    "properties": {
      "answer": 42
    },
    "geometry": { "type": "Polygon", "coordinates": [
        [ [3, 0], [1, 1], [0, 3], [-1, 1], [-3, 0], [-1, -1], [0, -3], [1, -1], [3, 0] ],
        [ [1, 0], [0, 1], [-1, 0], [0, -1], [1, 0] ]
    ] }
},
{
    "type": "Feature",
    "properties": {
      "answer": true
    },
    "geometry": { "type": "Polygon", "coordinates": [ [ [0, 10], [3, 5], [1, 5], [10, 0], [-1, 5], [-3, 5], [0, 10] ] ] }
}
]})json");

    LocationDependentData data({fixture.temporary_file});

    BOOST_CHECK(data.GetPropertyIndexes(point_t(0, 0)).empty());
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(1, 1)), 0);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(0, 1)), 0);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(0.5, -0.5)), 0);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(0, -3)), 0);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(-0.75, 0.75)), 0);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(2, 0)), 0);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(1, 7)), 1);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(-2, 6)), 1);

    BOOST_CHECK_EQUAL(data.FindByKey({}, "answer").which(), 0);
    BOOST_CHECK_EQUAL(data.FindByKey({0}, "foo").which(), 0);
    BOOST_CHECK_EQUAL(boost::get<double>(data.FindByKey({0}, "answer")), 42);
    BOOST_CHECK_EQUAL(boost::get<bool>(data.FindByKey({1}, "answer")), true);
}

BOOST_AUTO_TEST_CASE(multy_polygon_tests)
{
    LocationDataFixture fixture(R"json({
"type": "FeatureCollection",
"features": [
{
    "type": "Feature",
    "properties": {
      "answer": 42
    },
    "geometry": { "type": "MultiPolygon", "coordinates": [
    [ [ [1, 0], [0, 1], [-1, 0], [0, -1], [1, 0] ] ],
    [ [ [6, 0], [5, 1], [4, 0], [5, -1], [6, 0] ] ],
    [ [ [-4, 0], [-5, 1], [-6, 0], [-5, -1], [-4, 0] ] ]
 ] }
}
]})json");

    LocationDependentData data({fixture.temporary_file});

    BOOST_CHECK(data.GetPropertyIndexes(point_t(0, 2)).empty());
    BOOST_CHECK(data.GetPropertyIndexes(point_t(0, -3)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(0, 0)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(5, 0)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(-5, 0)).empty());
}

BOOST_AUTO_TEST_CASE(polygon_merging_tests)
{
    LocationDataFixture fixture(R"json({
"type": "FeatureCollection",
"features": [
{
    "type": "Feature",
    "properties": { "answer": "a" },
    "geometry": { "type": "Polygon", "coordinates": [ [ [3, 3], [-3, 3], [-3, -3], [3, -3], [3, 3] ] ] }
},
{
    "type": "Feature",
    "properties": { "answer": "b" },
    "geometry": { "type": "Polygon", "coordinates": [ [ [7, 3], [1, 3], [1, -3], [7, -3], [7, 3] ] ] }
},
{
    "type": "Feature",
    "properties": { "answer": "c" },
    "geometry": { "type": "Polygon", "coordinates": [ [ [8, 3], [2, 3], [2, -3], [8, -3], [8, 3] ] ] }
}
]})json");

    LocationDependentData data({fixture.temporary_file});

    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(-3, 3)), 0);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(-3, 1)), 0);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(-3, -3)), 0);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(0, 3)), 0);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(1, 0)), 0, 1);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(2, -3)), 0, 1, 2);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(3, 0)), 0, 1, 2);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(4, 3)), 1, 2);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(6, 1)), 1, 2);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(7, 0)), 1, 2);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(8, 3)), 2);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(8, -1)), 2);
    CHECK_EQUAL_RANGE(data.GetPropertyIndexes(point_t(8, -3)), 2);

    BOOST_CHECK_EQUAL(boost::get<std::string>(data.FindByKey({0}, "answer")), "a");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data.FindByKey({1}, "answer")), "b");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data.FindByKey({2}, "answer")), "c");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data.FindByKey({0, 1, 2}, "answer")), "a");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data.FindByKey({1, 2}, "answer")), "b");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data.FindByKey({2, 1, 0}, "answer")), "c");
}

BOOST_AUTO_TEST_CASE(staircase_polygon)
{
    LocationDataFixture fixture(R"json({
"type": "FeatureCollection",
"features": [
{
    "type": "Feature",
    "properties": { "answer": "a" },
    "geometry": { "type": "Polygon", "coordinates": [ [ [0, 0], [3, 0], [3, 3], [2, 3], [2, 2], [1, 2], [1, 1], [0, 1], [0, 0] ] ] }
}
]})json");

    LocationDependentData data({fixture.temporary_file});

    // all corners
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(0, 0)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(0, 1)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(1, 1)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(1, 2)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(2, 2)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(2, 3)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(3, 3)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(3, 0)).empty());

    // at x = 1
    BOOST_CHECK(data.GetPropertyIndexes(point_t(1, -0.5)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(1, 0)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(1, 0.5)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(1, 1.5)).empty());
    BOOST_CHECK(data.GetPropertyIndexes(point_t(1, 2.5)).empty());
    BOOST_CHECK(data.GetPropertyIndexes(point_t(3.5, 2)).empty());

    // at y = 2
    BOOST_CHECK(data.GetPropertyIndexes(point_t(0.5, 2)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(1, 2)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(1.5, 2)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(2, 2)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(2.5, 2)).empty());
    BOOST_CHECK(!data.GetPropertyIndexes(point_t(3, 2)).empty());
    BOOST_CHECK(data.GetPropertyIndexes(point_t(3.5, 2)).empty());
}

BOOST_AUTO_TEST_SUITE_END()

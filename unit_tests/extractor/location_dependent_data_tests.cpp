#include "extractor/location_dependent_data.hpp"

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

    BOOST_CHECK_EQUAL(data(point_t(0, 0), "answer").which(), 0);
    BOOST_CHECK_EQUAL(boost::get<double>(data(point_t(1, 1), "answer")), 42);
    BOOST_CHECK_EQUAL(boost::get<double>(data(point_t(0, 1), "answer")), 42);
    BOOST_CHECK_EQUAL(boost::get<double>(data(point_t(0.5, -0.5), "answer")), 42);
    BOOST_CHECK_EQUAL(boost::get<double>(data(point_t(0, -3), "answer")), 42);
    BOOST_CHECK_EQUAL(boost::get<double>(data(point_t(-0.75, 0.75), "answer")), 42);
    BOOST_CHECK_EQUAL(boost::get<double>(data(point_t(2, 0), "answer")), 42.);
    BOOST_CHECK_EQUAL(boost::get<bool>(data(point_t(1, 7), "answer")), true);
    BOOST_CHECK_EQUAL(boost::get<bool>(data(point_t(-2, 6), "answer")), true);
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

    BOOST_CHECK_EQUAL(data(point_t(0, 2), "answer").which(), 0);
    BOOST_CHECK_EQUAL(data(point_t(0, -3), "answer").which(), 0);
    BOOST_CHECK_EQUAL(boost::get<double>(data(point_t(0, 0), "answer")), 42.);
    BOOST_CHECK_EQUAL(boost::get<double>(data(point_t(5, 0), "answer")), 42.);
    BOOST_CHECK_EQUAL(boost::get<double>(data(point_t(-5, 0), "answer")), 42.);
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

    BOOST_CHECK_EQUAL(boost::get<std::string>(data(point_t(-3, 3), "answer")), "a");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data(point_t(-3, 1), "answer")), "a");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data(point_t(-3, -3), "answer")), "a");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data(point_t(0, 3), "answer")), "a");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data(point_t(1, 0), "answer")), "a");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data(point_t(2, -3), "answer")), "a");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data(point_t(3, 0), "answer")), "a");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data(point_t(4, 3), "answer")), "b");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data(point_t(6, 1), "answer")), "b");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data(point_t(7, 0), "answer")), "b");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data(point_t(8, 3), "answer")), "c");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data(point_t(8, -1), "answer")), "c");
    BOOST_CHECK_EQUAL(boost::get<std::string>(data(point_t(8, -3), "answer")), "c");
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
    BOOST_CHECK_NE(data(point_t(0, 0), "answer").which(), 0);
    BOOST_CHECK_NE(data(point_t(0, 1), "answer").which(), 0);
    BOOST_CHECK_NE(data(point_t(1, 1), "answer").which(), 0);
    BOOST_CHECK_NE(data(point_t(1, 2), "answer").which(), 0);
    BOOST_CHECK_NE(data(point_t(2, 2), "answer").which(), 0);
    BOOST_CHECK_NE(data(point_t(2, 3), "answer").which(), 0);
    BOOST_CHECK_NE(data(point_t(3, 3), "answer").which(), 0);
    BOOST_CHECK_NE(data(point_t(3, 0), "answer").which(), 0);

    // // at x = 1
    BOOST_CHECK_EQUAL(data(point_t(1, -0.5), "answer").which(), 0);
    BOOST_CHECK_NE(data(point_t(1, 0), "answer").which(), 0);
    BOOST_CHECK_NE(data(point_t(1, 0.5), "answer").which(), 0);
    BOOST_CHECK_NE(data(point_t(1, 1.5), "answer").which(), 0);
    BOOST_CHECK_EQUAL(data(point_t(1, 2.5), "answer").which(), 0);
    BOOST_CHECK_EQUAL(data(point_t(3.5, 2), "answer").which(), 0);

    // // at y = 2
    BOOST_CHECK_EQUAL(data(point_t(0.5, 2), "answer").which(), 0);
    BOOST_CHECK_NE(data(point_t(1, 2), "answer").which(), 0);
    BOOST_CHECK_NE(data(point_t(1.5, 2), "answer").which(), 0);
    BOOST_CHECK_NE(data(point_t(2, 2), "answer").which(), 0);
    BOOST_CHECK_NE(data(point_t(2.5, 2), "answer").which(), 0);
    BOOST_CHECK_NE(data(point_t(3, 2), "answer").which(), 0);
    BOOST_CHECK_EQUAL(data(point_t(3.5, 2), "answer").which(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

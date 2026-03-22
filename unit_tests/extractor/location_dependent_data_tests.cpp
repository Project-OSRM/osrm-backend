#include "extractor/location_dependent_data.hpp"

#include "../common/range_tools.hpp"
#include "../common/temporary_file.hpp"

#include <boost/test/unit_test.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>

BOOST_AUTO_TEST_SUITE(location_dependent_data_tests)

using namespace osrm;
using namespace osrm::extractor;
using point_t = LocationDependentData::point_t;

static const std::string SIMPLE_BOX_JSON = R"json({
"type": "FeatureCollection",
"features": [
{
    "type": "Feature",
    "properties": { "iso_a3_eh": "CHE", "answer": 42 },
    "geometry": { "type": "Polygon", "coordinates": [ [ [0,0],[10,0],[10,10],[0,10],[0,0] ] ] }
}
]})json";

struct LocationDataFixture
{
    LocationDataFixture(const std::string &json)
    {
        std::ofstream file(temporary_file.path.string());
        file << json;
    }

    TemporaryFile temporary_file;
};

// Helper: create a temporary directory that is removed on destruction.
struct TemporaryDirectory
{
    TemporaryDirectory()
        : path(std::filesystem::temp_directory_path() / random_string(8))
    {
        std::filesystem::create_directory(path);
    }
    ~TemporaryDirectory() { std::filesystem::remove_all(path); }

    std::filesystem::path path;
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

    LocationDependentData data({fixture.temporary_file.path});

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

    LocationDependentData data({fixture.temporary_file.path});

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

    LocationDependentData data({fixture.temporary_file.path});

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

    LocationDependentData data({fixture.temporary_file.path});

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

BOOST_AUTO_TEST_CASE(empty_when_no_files)
{    // No files provided → data is empty → has_location_data() equivalent returns true for empty()
    LocationDependentData data({});
    BOOST_CHECK(data.empty());
    BOOST_CHECK(data.GetPropertyIndexes(point_t(5, 5)).empty());
}

BOOST_AUTO_TEST_CASE(not_empty_when_file_loaded)
{
    LocationDataFixture fixture(SIMPLE_BOX_JSON);
    LocationDependentData data({fixture.temporary_file.path});
    BOOST_CHECK(!data.empty());
}

BOOST_AUTO_TEST_CASE(directory_loading)
{
    // Write two GeoJSON files into a temp directory: CHE (box [0,0]-[10,10])
    // and FRA (box [20,0]-[30,10]).  Pass the directory to the constructor.
    TemporaryDirectory dir;

    auto write = [&](const std::string &filename, const std::string &iso, double x0, double x1)
    {
        std::ofstream f(dir.path / filename);
        f << R"json({"type":"FeatureCollection","features":[{"type":"Feature",)json"
          << R"json("properties":{"iso_a3_eh":")json" << iso << R"json("},)json"
          << R"json("geometry":{"type":"Polygon","coordinates":[[[)json"
          << x0 << ",0],[" << x1 << ",0],[" << x1 << ",10],[" << x0 << ",10],[" << x0
          << R"json(,0]]]}},)json"
          << R"json("type":"Feature"]})json";
        // well-formed GeoJSON via explicit construction below
        f.close();

        // Overwrite with properly-formed GeoJSON
        std::ofstream g(dir.path / filename);
        g << "{\"type\":\"FeatureCollection\",\"features\":["
          << "{\"type\":\"Feature\","
          << "\"properties\":{\"iso_a3_eh\":\"" << iso << "\"},"
          << "\"geometry\":{\"type\":\"Polygon\","
          << "\"coordinates\":[[[" << x0 << ",0],[" << x1 << ",0],["
          << x1 << ",10],[" << x0 << ",10],[" << x0 << ",0]]]}}"
          << "]}";
    };

    write("CHE.geojson", "CHE", 0, 10);
    write("FRA.geojson", "FRA", 20, 30);

    // Also write a non-geojson file that must be ignored
    { std::ofstream f(dir.path / "README.txt"); f << "ignore me"; }

    LocationDependentData data({dir.path});

    BOOST_CHECK(!data.empty());

    // Point inside CHE polygon
    auto che_indexes = data.GetPropertyIndexes(point_t(5, 5));
    BOOST_CHECK(!che_indexes.empty());
    BOOST_CHECK_EQUAL(boost::get<std::string>(data.FindByKey(che_indexes, "iso_a3_eh")), "CHE");

    // Point inside FRA polygon
    auto fra_indexes = data.GetPropertyIndexes(point_t(25, 5));
    BOOST_CHECK(!fra_indexes.empty());
    BOOST_CHECK_EQUAL(boost::get<std::string>(data.FindByKey(fra_indexes, "iso_a3_eh")), "FRA");

    // Point between the two polygons — outside both
    BOOST_CHECK(data.GetPropertyIndexes(point_t(15, 5)).empty());
}

BOOST_AUTO_TEST_CASE(directory_and_file_combined)
{
    // Pass a directory (CHE) plus an individual file (GRC) in the same call.
    TemporaryDirectory dir;

    auto write_file = [](const std::filesystem::path &path,
                         const std::string &iso,
                         double x0,
                         double x1)
    {
        std::ofstream f(path);
        f << "{\"type\":\"FeatureCollection\",\"features\":["
          << "{\"type\":\"Feature\","
          << "\"properties\":{\"iso_a3_eh\":\"" << iso << "\"},"
          << "\"geometry\":{\"type\":\"Polygon\","
          << "\"coordinates\":[[[" << x0 << ",0],[" << x1 << ",0],["
          << x1 << ",10],[" << x0 << ",10],[" << x0 << ",0]]]}}"
          << "]}";
    };

    write_file(dir.path / "CHE.geojson", "CHE", 0, 10);

    TemporaryFile grc_file;
    write_file(grc_file.path, "GRC", 40, 50);

    LocationDependentData data({dir.path, grc_file.path});

    BOOST_CHECK(!data.empty());
    auto che = data.GetPropertyIndexes(point_t(5, 5));
    BOOST_CHECK_EQUAL(boost::get<std::string>(data.FindByKey(che, "iso_a3_eh")), "CHE");

    auto grc = data.GetPropertyIndexes(point_t(45, 5));
    BOOST_CHECK_EQUAL(boost::get<std::string>(data.FindByKey(grc, "iso_a3_eh")), "GRC");
}

BOOST_AUTO_TEST_CASE(overlapping_polygons)
{
    // Two polygons that overlap in the region [2,2]-[4,4].
    // A covers [0,0]-[5,5], B covers [2,2]-[7,7].
    // A point in the overlap area must be inside both polygons;
    // GetPropertyIndexes returns multiple indexes — GIGO, first wins.
    LocationDataFixture fixture(R"json({
"type": "FeatureCollection",
"features": [
{
    "type": "Feature",
    "properties": { "iso_a3_eh": "AAA", "rank": 1 },
    "geometry": { "type": "Polygon", "coordinates":
        [ [ [0,0],[5,0],[5,5],[0,5],[0,0] ] ] }
},
{
    "type": "Feature",
    "properties": { "iso_a3_eh": "BBB", "rank": 2 },
    "geometry": { "type": "Polygon", "coordinates":
        [ [ [2,2],[7,2],[7,7],[2,7],[2,2] ] ] }
}
]})json");

    LocationDependentData data({fixture.temporary_file.path});
    BOOST_CHECK(!data.empty());

    // Point only in A
    auto a_only = data.GetPropertyIndexes(point_t(1, 1));
    BOOST_REQUIRE_EQUAL(a_only.size(), 1u);
    BOOST_CHECK_EQUAL(boost::get<std::string>(data.FindByKey(a_only, "iso_a3_eh")), "AAA");

    // Point only in B
    auto b_only = data.GetPropertyIndexes(point_t(6, 6));
    BOOST_REQUIRE_EQUAL(b_only.size(), 1u);
    BOOST_CHECK_EQUAL(boost::get<std::string>(data.FindByKey(b_only, "iso_a3_eh")), "BBB");

    // Point in the overlap — must be inside both polygons
    auto overlap = data.GetPropertyIndexes(point_t(3, 3));
    BOOST_CHECK_GE(overlap.size(), 2u);

    // FindByKey returns the first match — result is one of AAA or BBB,
    // not a crash or a mixture of the two.
    const auto winner = boost::get<std::string>(data.FindByKey(overlap, "iso_a3_eh"));
    BOOST_CHECK(winner == "AAA" || winner == "BBB");
}

BOOST_AUTO_TEST_SUITE_END()


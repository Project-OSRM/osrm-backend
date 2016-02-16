#include "server/api/parameters_parser.hpp"

#include <fstream>

namespace osrm
{
namespace engine
{
namespace api
{
std::ostream &operator<<(std::ostream &out, api::RouteParameters::GeometriesType geometries)
{
    switch (geometries)
    {
    case api::RouteParameters::GeometriesType::GeoJSON:
        out << "GeoJSON";
        break;
    case api::RouteParameters::GeometriesType::Polyline:
        out << "Polyline";
        break;
    default:
        BOOST_ASSERT_MSG(false, "GeometriesType not fully captured");
    }
    return out;
}
std::ostream &operator<<(std::ostream &out, api::RouteParameters::OverviewType overview)
{
    switch (overview)
    {
    case api::RouteParameters::OverviewType::False:
        out << "False";
        break;
    case api::RouteParameters::OverviewType::Full:
        out << "Full";
        break;
    case api::RouteParameters::OverviewType::Simplified:
        out << "Simplified";
        break;
    default:
        BOOST_ASSERT_MSG(false, "OverviewType not fully captured");
    }
    return out;
}
std::ostream &operator<<(std::ostream &out, api::RouteParameters::Bearing bearing)
{
    out << bearing.bearing << "," << bearing.range;
    return out;
}
}
}
}

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/optional/optional_io.hpp>

#define CHECK_EQUAL_RANGE(R1, R2)                                                                  \
    BOOST_CHECK_EQUAL_COLLECTIONS(R1.begin(), R1.end(), R2.begin(), R2.end());

BOOST_AUTO_TEST_SUITE(api_parameters_parser)

using namespace osrm;
using namespace osrm::server;

// returns distance to front
template <typename ParameterT> std::size_t testInvalidOptions(std::string options)
{
    auto iter = options.begin();
    auto result = api::parseParameters<ParameterT>(iter, options.end());
    BOOST_CHECK(!result);
    return std::distance(options.begin(), iter);
}

BOOST_AUTO_TEST_CASE(invalid_route_urls)
{
    BOOST_CHECK_EQUAL(testInvalidOptions<engine::api::RouteParameters>("overview=false&bla=foo"),
                      14UL);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<engine::api::RouteParameters>("overview=false&bearings=foo"), 24UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<engine::api::RouteParameters>("overview=false&uturns=foo"),
                      22UL);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<engine::api::RouteParameters>("overview=false&radiuses=foo"), 24UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<engine::api::RouteParameters>("overview=false&hints=foo"),
                      14UL);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<engine::api::RouteParameters>("overview=false&geometries=foo"), 14UL);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<engine::api::RouteParameters>("overview=false&overview=foo"), 14UL);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<engine::api::RouteParameters>("overview=false&alternative=foo"), 14UL);
}

BOOST_AUTO_TEST_CASE(invalid_table_urls)
{
    BOOST_CHECK_EQUAL(testInvalidOptions<engine::api::TableParameters>("sources=1&bla=foo"), 9UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<engine::api::TableParameters>("destinations=1&bla=foo"), 14UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<engine::api::TableParameters>("sources=1&destinations=1&bla=foo"), 24UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<engine::api::TableParameters>("sources=foo"), 8UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<engine::api::TableParameters>("destinations=foo"), 13UL);
}

BOOST_AUTO_TEST_CASE(valid_route_urls)
{
    engine::api::RouteParameters reference_1{};
    auto result_1 = api::parseParameters<engine::api::RouteParameters>("");
    BOOST_CHECK(result_1);
    BOOST_CHECK_EQUAL(reference_1.steps, result_1->steps);
    BOOST_CHECK_EQUAL(reference_1.alternative, result_1->alternative);
    BOOST_CHECK_EQUAL(reference_1.geometries, result_1->geometries);
    BOOST_CHECK_EQUAL(reference_1.overview, result_1->overview);
    CHECK_EQUAL_RANGE(reference_1.uturns, result_1->uturns);
    CHECK_EQUAL_RANGE(reference_1.bearings, result_1->bearings);
    CHECK_EQUAL_RANGE(reference_1.radiuses, result_1->radiuses);
    CHECK_EQUAL_RANGE(reference_1.coordinates, result_1->coordinates);

    engine::api::RouteParameters reference_2{};
    auto result_2 = api::parseParameters<engine::api::RouteParameters>(
        "steps=true&alternative=true&geometries=polyline&overview=simplified");
    BOOST_CHECK(result_2);
    BOOST_CHECK_EQUAL(reference_2.steps, result_2->steps);
    BOOST_CHECK_EQUAL(reference_2.alternative, result_2->alternative);
    BOOST_CHECK_EQUAL(reference_2.geometries, result_2->geometries);
    BOOST_CHECK_EQUAL(reference_2.overview, result_2->overview);
    CHECK_EQUAL_RANGE(reference_2.uturns, result_2->uturns);
    CHECK_EQUAL_RANGE(reference_2.bearings, result_2->bearings);
    CHECK_EQUAL_RANGE(reference_2.radiuses, result_2->radiuses);
    CHECK_EQUAL_RANGE(reference_2.coordinates, result_2->coordinates);

    std::vector<boost::optional<bool>> uturns_3 = {true, false, boost::none};
    engine::api::RouteParameters reference_3{
        false, false, engine::api::RouteParameters::GeometriesType::GeoJSON,
        engine::api::RouteParameters::OverviewType::False, uturns_3};
    auto result_3 = api::parseParameters<engine::api::RouteParameters>(
        "steps=false&alternative=false&geometries=geojson&overview=false&uturns=true;false;");
    BOOST_CHECK(result_3);
    BOOST_CHECK_EQUAL(reference_3.steps, result_3->steps);
    BOOST_CHECK_EQUAL(reference_3.alternative, result_3->alternative);
    BOOST_CHECK_EQUAL(reference_3.geometries, result_3->geometries);
    BOOST_CHECK_EQUAL(reference_3.overview, result_3->overview);
    CHECK_EQUAL_RANGE(reference_3.uturns, result_3->uturns);
    CHECK_EQUAL_RANGE(reference_3.bearings, result_3->bearings);
    CHECK_EQUAL_RANGE(reference_3.radiuses, result_3->radiuses);
    CHECK_EQUAL_RANGE(reference_3.coordinates, result_3->coordinates);

    std::vector<boost::optional<engine::Hint>> hints_4 = {
        engine::Hint::FromBase64(
            "rVghAzxMzABMAwAA5h4CAKMIAAAQAAAAGAAAAAYAAAAAAAAAch8BAJ4AAACpWCED_0vMAAEAAQGLSzmR"),
        engine::Hint::FromBase64(
            "_4ghA4JuzAD_IAAAo28BAOYAAAAzAAAAAgAAAEwAAAAAAAAAdIwAAJ4AAAAXiSEDfm7MAAEAAQGLSzmR"),
        engine::Hint::FromBase64(
            "03AhA0vnzAA_SAAA_____3wEAAAYAAAAQAAAAB4AAABAAAAAoUYBAJ4AAADlcCEDSefMAAMAAQGLSzmR")};
    engine::api::RouteParameters reference_4{
        false,
        true,
        engine::api::RouteParameters::GeometriesType::Polyline,
        engine::api::RouteParameters::OverviewType::Simplified,
        std::vector<boost::optional<bool>>{},
        std::vector<util::FixedPointCoordinate>{},
        hints_4,
        std::vector<boost::optional<double>>{},
        std::vector<boost::optional<engine::api::BaseParameters::Bearing>>{}};
    auto result_4 = api::parseParameters<engine::api::RouteParameters>(
        "steps=false&hints=rVghAzxMzABMAwAA5h4CAKMIAAAQAAAAGAAAAAYAAAAAAAAAch8BAJ4AAACpWCED_"
        "0vMAAEAAQGLSzmR;_4ghA4JuzAD_"
        "IAAAo28BAOYAAAAzAAAAAgAAAEwAAAAAAAAAdIwAAJ4AAAAXiSEDfm7MAAEAAQGLSzmR;03AhA0vnzAA_SAAA_____"
        "3wEAAAYAAAAQAAAAB4AAABAAAAAoUYBAJ4AAADlcCEDSefMAAMAAQGLSzmR");
    BOOST_CHECK(result_4);
    BOOST_CHECK_EQUAL(reference_4.steps, result_4->steps);
    BOOST_CHECK_EQUAL(reference_4.alternative, result_4->alternative);
    BOOST_CHECK_EQUAL(reference_4.geometries, result_4->geometries);
    BOOST_CHECK_EQUAL(reference_4.overview, result_4->overview);
    CHECK_EQUAL_RANGE(reference_4.uturns, result_4->uturns);
    CHECK_EQUAL_RANGE(reference_4.bearings, result_4->bearings);
    CHECK_EQUAL_RANGE(reference_4.radiuses, result_4->radiuses);
    CHECK_EQUAL_RANGE(reference_4.coordinates, result_4->coordinates);

    std::vector<boost::optional<engine::api::BaseParameters::Bearing>> bearings_4 = {
        boost::none,
        engine::api::BaseParameters::Bearing {200, 10},
        engine::api::BaseParameters::Bearing {100, 5},
    };
    engine::api::RouteParameters reference_5{
        false,
        true,
        engine::api::RouteParameters::GeometriesType::Polyline,
        engine::api::RouteParameters::OverviewType::Simplified,
        std::vector<boost::optional<bool>>{},
        std::vector<util::FixedPointCoordinate>{},
        std::vector<boost::optional<engine::Hint>> {},
        std::vector<boost::optional<double>>{},
        bearings_4};
    auto result_5 = api::parseParameters<engine::api::RouteParameters>(
        "steps=false&bearings=;200,10;100,5");
    BOOST_CHECK(result_5);
    BOOST_CHECK_EQUAL(reference_5.steps, result_5->steps);
    BOOST_CHECK_EQUAL(reference_5.alternative, result_5->alternative);
    BOOST_CHECK_EQUAL(reference_5.geometries, result_5->geometries);
    BOOST_CHECK_EQUAL(reference_5.overview, result_5->overview);
    CHECK_EQUAL_RANGE(reference_5.uturns, result_5->uturns);
    CHECK_EQUAL_RANGE(reference_5.bearings, result_5->bearings);
    CHECK_EQUAL_RANGE(reference_5.radiuses, result_5->radiuses);
    CHECK_EQUAL_RANGE(reference_5.coordinates, result_5->coordinates);
}

BOOST_AUTO_TEST_CASE(valid_table_urls)
{
    engine::api::TableParameters reference_1{};
    auto result_1 = api::parseParameters<engine::api::TableParameters>("");
    BOOST_CHECK(result_1);
    CHECK_EQUAL_RANGE(reference_1.sources, result_1->sources);
    CHECK_EQUAL_RANGE(reference_1.destinations, result_1->destinations);
    CHECK_EQUAL_RANGE(reference_1.bearings, result_1->bearings);
    CHECK_EQUAL_RANGE(reference_1.radiuses, result_1->radiuses);
    CHECK_EQUAL_RANGE(reference_1.coordinates, result_1->coordinates);

    std::vector<std::size_t> sources_2 = {1, 2, 3};
    std::vector<std::size_t> destinations_2 = {4, 5};
    engine::api::TableParameters reference_2{sources_2, destinations_2};
    auto result_2 = api::parseParameters<engine::api::TableParameters>("sources=1;2;3&destinations=4;5");
    BOOST_CHECK(result_2);
    CHECK_EQUAL_RANGE(reference_2.sources,      result_2->sources);
    CHECK_EQUAL_RANGE(reference_2.destinations, result_2->destinations);
    CHECK_EQUAL_RANGE(reference_2.bearings,     result_2->bearings);
    CHECK_EQUAL_RANGE(reference_2.radiuses,     result_2->radiuses);
    CHECK_EQUAL_RANGE(reference_2.coordinates,  result_2->coordinates);
}

BOOST_AUTO_TEST_SUITE_END()

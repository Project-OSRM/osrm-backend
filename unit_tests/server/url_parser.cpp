#include "server/api/url_parser.hpp"

#include <fstream>

// needed for BOOST_CHECK_EQUAL
namespace osrm
{
namespace server
{
namespace api
{
std::ostream& operator<<(std::ostream& out, const osrm::server::api::ParsedURL& url)
{
    out << url.service << ", " << url.version << ", " << url.profile << ", ";
    for (auto c : url.coordinates)
    {
        out << c << " ";
    }
    out << ", " << url.options;

    return out;
}
}
}
}


#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#define CHECK_EQUAL_RANGE(R1, R2) \
  BOOST_CHECK_EQUAL_COLLECTIONS(R1.begin(), R1.end(), R2.begin(), R2.end());

BOOST_AUTO_TEST_SUITE(api_url_parser)

using namespace osrm;
using namespace osrm::server;

// returns distance to front
std::size_t testInvalidURL(std::string url)
{
    auto iter = url.begin();
    auto result = api::parseURL(iter, url.end());
    BOOST_CHECK(!result);
    return std::distance(url.begin(), iter);
}

BOOST_AUTO_TEST_CASE(invalid_urls)
{
    BOOST_CHECK_EQUAL(testInvalidURL("/route/"), 0UL);
    BOOST_CHECK_EQUAL(testInvalidURL("/route/bla"), 0UL);
    BOOST_CHECK_EQUAL(testInvalidURL("/route/1/1,2;3;4"), 0UL);
    BOOST_CHECK_EQUAL(testInvalidURL("/route/v1/pro_file/1,2;3,4"), 0UL);
    BOOST_CHECK_EQUAL(testInvalidURL("/route/v1/profile"), 0UL);
    BOOST_CHECK_EQUAL(testInvalidURL("/route/v1/profile/"), 0UL);
}

BOOST_AUTO_TEST_CASE(valid_urls)
{
    std::vector<util::FixedPointCoordinate> coords_1 = {
        // lat,lon
        util::FixedPointCoordinate(1 * COORDINATE_PRECISION, 0 * COORDINATE_PRECISION),
        util::FixedPointCoordinate(3 * COORDINATE_PRECISION, 2 * COORDINATE_PRECISION),
        util::FixedPointCoordinate(5 * COORDINATE_PRECISION, 4 * COORDINATE_PRECISION),
    };
    api::ParsedURL reference_1{"route", 1, "profile", coords_1, "options=value&foo=bar"};
    auto result_1 = api::parseURL("/route/v1/profile/0,1;2,3;4,5?options=value&foo=bar");
    BOOST_CHECK(result_1);
    BOOST_CHECK_EQUAL(reference_1.service,     result_1->service);
    BOOST_CHECK_EQUAL(reference_1.version,     result_1->version);
    BOOST_CHECK_EQUAL(reference_1.profile,     result_1->profile);
    CHECK_EQUAL_RANGE(reference_1.coordinates, result_1->coordinates);
    BOOST_CHECK_EQUAL(reference_1.options,     result_1->options);

    // no options
    api::ParsedURL reference_2{"route", 1, "profile", coords_1, ""};
    auto result_2 = api::parseURL("/route/v1/profile/0,1;2,3;4,5");
    BOOST_CHECK(result_2);
    BOOST_CHECK_EQUAL(reference_2.service,     result_2->service);
    BOOST_CHECK_EQUAL(reference_2.version,     result_2->version);
    BOOST_CHECK_EQUAL(reference_2.profile,     result_2->profile);
    CHECK_EQUAL_RANGE(reference_2.coordinates, result_2->coordinates);
    BOOST_CHECK_EQUAL(reference_2.options,     result_2->options);

    // one coordinate
    std::vector<util::FixedPointCoordinate> coords_3 = {
        // lat,lon
        util::FixedPointCoordinate(1 * COORDINATE_PRECISION, 0 * COORDINATE_PRECISION),
    };
    api::ParsedURL reference_3{"route", 1, "profile", coords_3, ""};
    auto result_3 = api::parseURL("/route/v1/profile/0,1");
    BOOST_CHECK(result_3);
    BOOST_CHECK_EQUAL(reference_3.service,     result_3->service);
    BOOST_CHECK_EQUAL(reference_3.version,     result_3->version);
    BOOST_CHECK_EQUAL(reference_3.profile,     result_3->profile);
    CHECK_EQUAL_RANGE(reference_3.coordinates, result_3->coordinates);
    BOOST_CHECK_EQUAL(reference_3.options,     result_3->options);

    // polyline
    api::ParsedURL reference_5{"route", 1, "profile", coords_1, ""};
    auto result_5 = api::parseURL("/route/v1/profile/polyline(_ibE?_seK_seK_seK_seK)?");
    BOOST_CHECK(result_5);
    BOOST_CHECK_EQUAL(reference_5.service,     result_5->service);
    BOOST_CHECK_EQUAL(reference_5.version,     result_5->version);
    BOOST_CHECK_EQUAL(reference_5.profile,     result_5->profile);
    CHECK_EQUAL_RANGE(reference_5.coordinates, result_5->coordinates);
    BOOST_CHECK_EQUAL(reference_5.options,     result_5->options);
}

BOOST_AUTO_TEST_SUITE_END()

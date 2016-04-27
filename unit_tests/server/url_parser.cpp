#include "server/api/url_parser.hpp"

#include <fstream>

// needed for BOOST_CHECK_EQUAL
namespace osrm
{
namespace server
{
namespace api
{
std::ostream &operator<<(std::ostream &out, const osrm::server::api::ParsedURL &url)
{
    out << url.service << ", " << url.version << ", " << url.profile << ", " << url.query;

    return out;
}
}
}
}

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#define CHECK_EQUAL_RANGE(R1, R2)                                                                  \
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
    BOOST_CHECK_EQUAL(testInvalidURL("/route/"), 7UL);
    BOOST_CHECK_EQUAL(testInvalidURL("/route/bla"), 7UL);
    BOOST_CHECK_EQUAL(testInvalidURL("/route/1/1,2;3;4"), 7UL);
    BOOST_CHECK_EQUAL(testInvalidURL("/route/v1/pro_file/1,2;3,4"), 13UL);
    BOOST_CHECK_EQUAL(testInvalidURL("/route/v1/profile"), 17UL);
    BOOST_CHECK_EQUAL(testInvalidURL("/route/v1/profile/"), 18UL);
}

BOOST_AUTO_TEST_CASE(valid_urls)
{
    api::ParsedURL reference_1{"route", 1, "profile", "0,1;2,3;4,5?options=value&foo=bar"};
    auto result_1 = api::parseURL("/route/v1/profile/0,1;2,3;4,5?options=value&foo=bar");
    BOOST_CHECK(result_1);
    BOOST_CHECK_EQUAL(reference_1.service, result_1->service);
    BOOST_CHECK_EQUAL(reference_1.version, result_1->version);
    BOOST_CHECK_EQUAL(reference_1.profile, result_1->profile);
    CHECK_EQUAL_RANGE(reference_1.query, result_1->query);

    // no options
    api::ParsedURL reference_2{"route", 1, "profile", "0,1;2,3;4,5"};
    auto result_2 = api::parseURL("/route/v1/profile/0,1;2,3;4,5");
    BOOST_CHECK(result_2);
    BOOST_CHECK_EQUAL(reference_2.service, result_2->service);
    BOOST_CHECK_EQUAL(reference_2.version, result_2->version);
    BOOST_CHECK_EQUAL(reference_2.profile, result_2->profile);
    CHECK_EQUAL_RANGE(reference_2.query, result_2->query);

    // one coordinate
    std::vector<util::Coordinate> coords_3 = {
        util::Coordinate(util::FloatLongitude(0), util::FloatLatitude(1)),
    };
    api::ParsedURL reference_3{"route", 1, "profile", "0,1"};
    auto result_3 = api::parseURL("/route/v1/profile/0,1");
    BOOST_CHECK(result_3);
    BOOST_CHECK_EQUAL(reference_3.service, result_3->service);
    BOOST_CHECK_EQUAL(reference_3.version, result_3->version);
    BOOST_CHECK_EQUAL(reference_3.profile, result_3->profile);
    CHECK_EQUAL_RANGE(reference_3.query, result_3->query);

    // polyline
    api::ParsedURL reference_5{"route", 1, "profile", "polyline(_ibE?_seK_seK_seK_seK)?"};
    auto result_5 = api::parseURL("/route/v1/profile/polyline(_ibE?_seK_seK_seK_seK)?");
    BOOST_CHECK(result_5);
    BOOST_CHECK_EQUAL(reference_5.service, result_5->service);
    BOOST_CHECK_EQUAL(reference_5.version, result_5->version);
    BOOST_CHECK_EQUAL(reference_5.profile, result_5->profile);
    CHECK_EQUAL_RANGE(reference_5.query, result_5->query);

    // tile
    api::ParsedURL reference_6{"route", 1, "profile", "tile(1,2,3).mvt"};
    auto result_6 = api::parseURL("/route/v1/profile/tile(1,2,3).mvt");
    BOOST_CHECK(result_5);
    BOOST_CHECK_EQUAL(reference_6.service, result_6->service);
    BOOST_CHECK_EQUAL(reference_6.version, result_6->version);
    BOOST_CHECK_EQUAL(reference_6.profile, result_6->profile);
    CHECK_EQUAL_RANGE(reference_6.query, result_6->query);
}

BOOST_AUTO_TEST_SUITE_END()

#include "server/api/parameters_parser.hpp"
#include "server/api/url_parser.hpp"

#include "osrm/isochrone_parameters.hpp"

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <cmath>
#include <limits>
#include <string>

BOOST_AUTO_TEST_SUITE(api_isochrone_parameters)

BOOST_AUTO_TEST_CASE(defaults_and_required_contours_seconds)
{
    auto parameters =
        osrm::server::api::parseParameters<osrm::IsochroneParameters>("1,2?contours_seconds=300");
    BOOST_REQUIRE(parameters);
    BOOST_REQUIRE_EQUAL(parameters->coordinates.size(), 1U);
    BOOST_REQUIRE_EQUAL(parameters->contours_seconds.size(), 1U);
    BOOST_CHECK_EQUAL(parameters->contours_seconds.front(), 300.);
    BOOST_CHECK(parameters->direction == osrm::IsochroneParameters::Direction::Outbound);
    BOOST_CHECK(parameters->polygons);
    BOOST_CHECK(!parameters->generalize);
    BOOST_CHECK(!parameters->denoise);
    BOOST_CHECK(parameters->IsValid());

    auto missing_contours_seconds =
        osrm::server::api::parseParameters<osrm::IsochroneParameters>("1,2");
    BOOST_REQUIRE(missing_contours_seconds);
    BOOST_CHECK(!missing_contours_seconds->IsValid());

    auto empty_contours_seconds =
        osrm::server::api::parseParameters<osrm::IsochroneParameters>("1,2?contours_seconds=");
    BOOST_CHECK(!empty_contours_seconds);
}

BOOST_AUTO_TEST_CASE(parses_multiple_contours_seconds_and_base_options)
{
    auto parameters = osrm::server::api::parseParameters<osrm::IsochroneParameters>(
        "1,2?contours_seconds=300,600.5&direction=inbound&polygons=false&generalize=12.5&denoise=0."
        "25&"
        "radiuses=60&bearings=200,10&approaches=curb&hints=&exclude=ferry,motorway");
    BOOST_REQUIRE(parameters);

    BOOST_REQUIRE_EQUAL(parameters->coordinates.size(), 1U);
    BOOST_REQUIRE_EQUAL(parameters->contours_seconds.size(), 2U);
    BOOST_CHECK_EQUAL(parameters->contours_seconds[0], 300.);
    BOOST_CHECK_EQUAL(parameters->contours_seconds[1], 600.5);
    BOOST_CHECK(parameters->direction == osrm::IsochroneParameters::Direction::Inbound);
    BOOST_CHECK(!parameters->polygons);
    BOOST_REQUIRE(parameters->generalize);
    BOOST_CHECK_EQUAL(*parameters->generalize, 12.5);
    BOOST_REQUIRE(parameters->denoise);
    BOOST_CHECK_EQUAL(*parameters->denoise, 0.25);

    BOOST_REQUIRE_EQUAL(parameters->radiuses.size(), 1U);
    BOOST_CHECK_EQUAL(*parameters->radiuses[0], 60.);

    BOOST_REQUIRE_EQUAL(parameters->bearings.size(), 1U);
    BOOST_REQUIRE(parameters->bearings[0]);
    BOOST_CHECK_EQUAL(parameters->bearings[0]->bearing, 200);
    BOOST_CHECK_EQUAL(parameters->bearings[0]->range, 10);

    BOOST_REQUIRE_EQUAL(parameters->approaches.size(), 1U);
    BOOST_REQUIRE(parameters->approaches[0]);
    BOOST_CHECK(parameters->approaches[0] == osrm::engine::Approach::CURB);

    BOOST_REQUIRE_EQUAL(parameters->hints.size(), 1U);
    BOOST_CHECK(!parameters->hints[0]);
    BOOST_REQUIRE_EQUAL(parameters->exclude.size(), 2U);
    BOOST_CHECK_EQUAL(parameters->exclude[0], "ferry");
    BOOST_CHECK_EQUAL(parameters->exclude[1], "motorway");
    BOOST_CHECK(parameters->IsValid());
}

BOOST_AUTO_TEST_CASE(parses_a_nonnegative_generalize_tolerance)
{
    auto zero = osrm::server::api::parseParameters<osrm::IsochroneParameters>(
        "1,2?contours_seconds=300&generalize=0");
    BOOST_REQUIRE(zero);
    BOOST_REQUIRE(zero->generalize);
    BOOST_CHECK_EQUAL(*zero->generalize, 0.);
    BOOST_CHECK(zero->IsValid());

    auto positive = osrm::server::api::parseParameters<osrm::IsochroneParameters>(
        "1,2?contours_seconds=300&generalize=123456789.125");
    BOOST_REQUIRE(positive);
    BOOST_REQUIRE(positive->generalize);
    BOOST_CHECK_EQUAL(*positive->generalize, 123456789.125);
    BOOST_CHECK(positive->IsValid());
}

BOOST_AUTO_TEST_CASE(parses_a_bounded_denoise_threshold)
{
    for (const auto expected : {0., 0.5, 1.})
    {
        auto parameters = osrm::server::api::parseParameters<osrm::IsochroneParameters>(
            "1,2?contours_seconds=300&denoise=" + std::to_string(expected));
        BOOST_REQUIRE(parameters);
        BOOST_REQUIRE(parameters->denoise);
        BOOST_CHECK_EQUAL(*parameters->denoise, expected);
        BOOST_CHECK(parameters->IsValid());
    }
}

BOOST_AUTO_TEST_CASE(rejects_generic_contours_and_validates_contours_seconds)
{
    BOOST_CHECK(!osrm::server::api::parseParameters<osrm::IsochroneParameters>(
        "1,2?contours_seconds=10&direction=sideways"));
    BOOST_CHECK(!osrm::server::api::parseParameters<osrm::IsochroneParameters>(
        "1,2?contours_seconds=10&polygons=maybe"));
    BOOST_CHECK(!osrm::server::api::parseParameters<osrm::IsochroneParameters>(
        "1,2?contours_seconds=10,nope"));
    BOOST_CHECK(!osrm::server::api::parseParameters<osrm::IsochroneParameters>("1,2?contours=10"));
    BOOST_CHECK(!osrm::server::api::parseParameters<osrm::IsochroneParameters>(
        "1,2?contours_seconds=10&generalize=nope"));
    BOOST_CHECK(!osrm::server::api::parseParameters<osrm::IsochroneParameters>(
        "1,2?contours_seconds=10&generalize=nan"));
    BOOST_CHECK(!osrm::server::api::parseParameters<osrm::IsochroneParameters>(
        "1,2?contours_seconds=10&generalize=inf"));
    BOOST_CHECK(!osrm::server::api::parseParameters<osrm::IsochroneParameters>(
        "1,2?contours_seconds=10&denoise=nope"));
    BOOST_CHECK(!osrm::server::api::parseParameters<osrm::IsochroneParameters>(
        "1,2?contours_seconds=10&denoise=nan"));
    BOOST_CHECK(!osrm::server::api::parseParameters<osrm::IsochroneParameters>(
        "1,2?contours_seconds=10&denoise=inf"));

    auto non_positive =
        osrm::server::api::parseParameters<osrm::IsochroneParameters>("1,2?contours_seconds=0,-1");
    BOOST_REQUIRE(non_positive);
    BOOST_CHECK(!non_positive->IsValid());

    osrm::IsochroneParameters non_finite;
    non_finite.coordinates = {{{osrm::util::FloatLongitude{1}, osrm::util::FloatLatitude{2}}}};
    non_finite.contours_seconds = {std::numeric_limits<double>::infinity()};
    BOOST_CHECK(!non_finite.IsValid());

    auto negative_generalize = osrm::server::api::parseParameters<osrm::IsochroneParameters>(
        "1,2?contours_seconds=10&generalize=-1");
    BOOST_REQUIRE(negative_generalize);
    BOOST_REQUIRE(negative_generalize->generalize);
    BOOST_CHECK(!negative_generalize->IsValid());

    osrm::IsochroneParameters invalid_generalize;
    invalid_generalize.coordinates = {
        {{osrm::util::FloatLongitude{1}, osrm::util::FloatLatitude{2}}}};
    invalid_generalize.contours_seconds = {10.};
    invalid_generalize.generalize = std::numeric_limits<double>::infinity();
    BOOST_CHECK(!invalid_generalize.IsValid());
    invalid_generalize.generalize = -std::numeric_limits<double>::infinity();
    BOOST_CHECK(!invalid_generalize.IsValid());
    invalid_generalize.generalize = std::numeric_limits<double>::quiet_NaN();
    BOOST_CHECK(!invalid_generalize.IsValid());

    osrm::IsochroneParameters invalid_denoise;
    invalid_denoise.coordinates = {{{osrm::util::FloatLongitude{1}, osrm::util::FloatLatitude{2}}}};
    invalid_denoise.contours_seconds = {10.};
    for (const auto denoise : {-1.,
                               1.1,
                               std::numeric_limits<double>::infinity(),
                               std::numeric_limits<double>::quiet_NaN()})
    {
        invalid_denoise.denoise = denoise;
        BOOST_CHECK(!invalid_denoise.IsValid());
    }

    osrm::IsochroneParameters invalid_direction;
    invalid_direction.coordinates = {
        {{osrm::util::FloatLongitude{1}, osrm::util::FloatLatitude{2}}}};
    invalid_direction.contours_seconds = {10.};
    invalid_direction.direction = static_cast<osrm::IsochroneParameters::Direction>(99);
    BOOST_CHECK(!invalid_direction.IsValid());
}

BOOST_AUTO_TEST_CASE(url_and_query_parsing_are_available_without_service_dispatch)
{
    auto url = osrm::server::api::parseURL(
        "/isochrone/v1/driving/1,2?contours_seconds=300,600&direction=inbound");
    BOOST_REQUIRE(url);
    BOOST_CHECK_EQUAL(url->service, "isochrone");
    BOOST_CHECK_EQUAL(url->version, 1U);
    BOOST_CHECK_EQUAL(url->profile, "driving");

    auto parameters = osrm::server::api::parseParameters<osrm::IsochroneParameters>(url->query);
    BOOST_REQUIRE(parameters);
    BOOST_CHECK(parameters->direction == osrm::IsochroneParameters::Direction::Inbound);
    BOOST_CHECK(parameters->IsValid());
}

BOOST_AUTO_TEST_CASE(requires_exactly_one_coordinate)
{
    auto parameters = osrm::server::api::parseParameters<osrm::IsochroneParameters>(
        "1,2;3,4?contours_seconds=300");
    BOOST_REQUIRE(parameters);
    BOOST_CHECK(!parameters->IsValid());
}

BOOST_AUTO_TEST_SUITE_END()

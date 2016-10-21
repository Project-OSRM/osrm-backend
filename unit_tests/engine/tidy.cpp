#include "engine/api/match_parameters_tidy.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <iterator>
#include <vector>

BOOST_AUTO_TEST_SUITE(tidy_test)

using namespace osrm;
using namespace osrm::util;
using namespace osrm::engine::api;

BOOST_AUTO_TEST_CASE(two_item_trace_already_tidied_test)
{
    MatchParameters params;
    params.coordinates.emplace_back(FloatLongitude{13.207993}, FloatLatitude{52.446379});
    params.coordinates.emplace_back(FloatLongitude{13.231658}, FloatLatitude{52.465416});

    params.timestamps.emplace_back(1477090402);
    params.timestamps.emplace_back(1477090663);

    tidy::Thresholds thresholds;
    thresholds.distance_in_meters = 15.;
    thresholds.duration_in_seconds = 5;

    auto result = tidy::tidy(params, thresholds);

    BOOST_CHECK_EQUAL(result.can_be_removed.size(), 2);
    BOOST_CHECK_EQUAL(result.original_to_tidied.size(), 2);

    BOOST_CHECK(result.can_be_removed[0] == false);
    BOOST_CHECK(result.can_be_removed[1] == false);
    BOOST_CHECK_EQUAL(result.original_to_tidied[0], 0);
    BOOST_CHECK_EQUAL(result.original_to_tidied[1], 1);
}

BOOST_AUTO_TEST_CASE(two_item_trace_needs_tidiying_test)
{
    MatchParameters params;
    params.coordinates.emplace_back(FloatLongitude{13.207993}, FloatLatitude{52.446379});
    params.coordinates.emplace_back(FloatLongitude{13.231658}, FloatLatitude{52.465416});

    params.timestamps.emplace_back(1477090402);
    params.timestamps.emplace_back(1477090663);

    tidy::Thresholds thresholds;
    thresholds.distance_in_meters = 5000;
    thresholds.duration_in_seconds = 5 * 60;

    auto result = tidy::tidy(params, thresholds);

    BOOST_CHECK_EQUAL(result.can_be_removed.size(), 2);
    BOOST_CHECK_EQUAL(result.original_to_tidied.size(), 2);

    BOOST_CHECK_EQUAL(result.can_be_removed[0], false);
    BOOST_CHECK_EQUAL(result.can_be_removed[1], true);
    BOOST_CHECK_EQUAL(result.original_to_tidied[0], 0);
    BOOST_CHECK_EQUAL(result.original_to_tidied[1], 0);
}

BOOST_AUTO_TEST_CASE(two_blobs_in_traces_needs_tidiying_test)
{
    MatchParameters params;

    params.coordinates.emplace_back(FloatLongitude{13.207993}, FloatLatitude{52.446379});
    params.coordinates.emplace_back(FloatLongitude{13.207994}, FloatLatitude{52.446380});
    params.coordinates.emplace_back(FloatLongitude{13.207995}, FloatLatitude{52.446381});

    params.coordinates.emplace_back(FloatLongitude{13.231658}, FloatLatitude{52.465416});
    params.coordinates.emplace_back(FloatLongitude{13.231659}, FloatLatitude{52.465417});
    params.coordinates.emplace_back(FloatLongitude{13.231660}, FloatLatitude{52.465417});

    params.timestamps.emplace_back(1477090402);
    params.timestamps.emplace_back(1477090403);
    params.timestamps.emplace_back(1477090404);

    params.timestamps.emplace_back(1477090661);
    params.timestamps.emplace_back(1477090662);
    params.timestamps.emplace_back(1477090663);

    tidy::Thresholds thresholds;
    thresholds.distance_in_meters = 15;
    thresholds.duration_in_seconds = 5;

    auto result = tidy::tidy(params, thresholds);

    BOOST_CHECK_EQUAL(result.can_be_removed.size(), params.coordinates.size());
    BOOST_CHECK_EQUAL(result.original_to_tidied.size(), params.coordinates.size());

    auto valid = [](auto index) { return index == 0 || index == 3; };
    auto ok = std::all_of(begin(result.original_to_tidied), end(result.original_to_tidied), valid);
    BOOST_CHECK(ok);

    BOOST_CHECK(std::is_sorted(begin(result.original_to_tidied), end(result.original_to_tidied)));

    const auto redundant = result.can_be_removed.count();
    BOOST_CHECK_EQUAL(redundant, params.coordinates.size() - 2);

    BOOST_CHECK_EQUAL(result.can_be_removed[0], false);
    BOOST_CHECK_EQUAL(result.can_be_removed[3], false);

    auto second_blob_start = std::partition_point(begin(result.original_to_tidied),
                                                  end(result.original_to_tidied),
                                                  [](auto index) { return index == 0; });

    BOOST_CHECK(begin(result.original_to_tidied) < second_blob_start);
    BOOST_CHECK(second_blob_start < end(result.original_to_tidied));
    BOOST_CHECK(second_blob_start == begin(result.original_to_tidied) + 3); // 3 in first blob
}

BOOST_AUTO_TEST_CASE(two_blobs_in_traces_needs_tidiying_no_timestamps_test)
{
    MatchParameters params;

    params.coordinates.emplace_back(FloatLongitude{13.207993}, FloatLatitude{52.446379});
    params.coordinates.emplace_back(FloatLongitude{13.207994}, FloatLatitude{52.446380});
    params.coordinates.emplace_back(FloatLongitude{13.207995}, FloatLatitude{52.446381});

    params.coordinates.emplace_back(FloatLongitude{13.231658}, FloatLatitude{52.465416});
    params.coordinates.emplace_back(FloatLongitude{13.231659}, FloatLatitude{52.465417});
    params.coordinates.emplace_back(FloatLongitude{13.231660}, FloatLatitude{52.465417});

    tidy::Thresholds thresholds;
    thresholds.distance_in_meters = 15;
    thresholds.duration_in_seconds = 5;

    auto result = tidy::tidy(params, thresholds);

    BOOST_CHECK_EQUAL(result.can_be_removed.size(), params.coordinates.size());
    BOOST_CHECK_EQUAL(result.original_to_tidied.size(), params.coordinates.size());

    auto valid = [](auto index) { return index == 0 || index == 3; };
    auto ok = std::all_of(begin(result.original_to_tidied), end(result.original_to_tidied), valid);
    BOOST_CHECK(ok);

    BOOST_CHECK(std::is_sorted(begin(result.original_to_tidied), end(result.original_to_tidied)));

    const auto redundant = result.can_be_removed.count();
    BOOST_CHECK_EQUAL(redundant, params.coordinates.size() - 2);

    BOOST_CHECK_EQUAL(result.can_be_removed[0], false);
    BOOST_CHECK_EQUAL(result.can_be_removed[3], false);

    auto second_blob_start = std::partition_point(begin(result.original_to_tidied),
                                                  end(result.original_to_tidied),
                                                  [](auto index) { return index == 0; });

    BOOST_CHECK(begin(result.original_to_tidied) < second_blob_start);
    BOOST_CHECK(second_blob_start < end(result.original_to_tidied));
    BOOST_CHECK(second_blob_start == begin(result.original_to_tidied) + 3); // 3 in first blob
}

BOOST_AUTO_TEST_SUITE_END()

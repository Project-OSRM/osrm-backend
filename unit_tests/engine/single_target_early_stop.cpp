#include "engine/routing_algorithms/single_target_early_stop.hpp"

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <limits>
#include <type_traits>

BOOST_AUTO_TEST_SUITE(ch_single_target_early_stop)

BOOST_AUTO_TEST_CASE(requires_a_finite_result_and_a_nonempty_target_search)
{
    osrm::engine::routing_algorithms::ch::SingleTargetEarlyStop bound;
    BOOST_CHECK(!bound.shouldStop(EdgeWeight{100}, EdgeWeight{5}));
    bound.addTarget(EdgeWeight{0}, EdgeDuration{0}, EdgeDistance{0}, EdgeWeight{0});
    BOOST_CHECK(!bound.shouldStop(EdgeWeight{100}, INVALID_EDGE_WEIGHT));
    BOOST_CHECK(bound.shouldStop(EdgeWeight{100}, EdgeWeight{5}));
}

BOOST_AUTO_TEST_CASE(keeps_equal_weight_duration_and_distance_ties)
{
    osrm::engine::routing_algorithms::ch::SingleTargetEarlyStop bound;
    bound.addTarget(EdgeWeight{-2}, EdgeDuration{0}, EdgeDistance{0}, EdgeWeight{0});
    BOOST_CHECK(!bound.shouldStop(EdgeWeight{10}, EdgeWeight{8}));
    BOOST_CHECK(bound.shouldStop(EdgeWeight{11}, EdgeWeight{8}));
}

BOOST_AUTO_TEST_CASE(excludes_open_area_graph_weight_loop_corrections)
{
    osrm::engine::routing_algorithms::ch::SingleTargetEarlyStop bound;
    bound.addTarget(EdgeWeight{0}, EdgeDuration{0}, EdgeDistance{0}, EdgeWeight{40});
    bound.addSource(EdgeDuration{0}, EdgeDistance{0}, EdgeWeight{30});
    BOOST_CHECK(!bound.shouldStop(EdgeWeight{69}, EdgeWeight{5}));
    BOOST_CHECK(bound.shouldStop(EdgeWeight{70}, EdgeWeight{5}));
}

BOOST_AUTO_TEST_CASE(waiting_negative_duration_labels_can_propagate_before_becoming_safe)
{
    osrm::engine::routing_algorithms::ch::SingleTargetEarlyStop bound;
    bound.addTarget(EdgeWeight{0}, EdgeDuration{2}, EdgeDistance{2}, EdgeWeight{0});
    bound.addSource(EdgeDuration{-10}, EdgeDistance{0}, EdgeWeight{0});
    BOOST_CHECK(!bound.shouldStop(EdgeWeight{100}, EdgeWeight{5}));
    bound.removeLabel(EdgeDuration{-10}, EdgeDistance{0});
    bound.insertLabel(EdgeDuration{-5}, EdgeDistance{0});
    BOOST_CHECK(!bound.shouldStop(EdgeWeight{100}, EdgeWeight{5}));
    bound.removeLabel(EdgeDuration{-5}, EdgeDistance{0});
    bound.insertLabel(EdgeDuration{0}, EdgeDistance{0});
    BOOST_CHECK(bound.shouldStop(EdgeWeight{100}, EdgeWeight{5}));
}

BOOST_AUTO_TEST_CASE(negative_distance_alone_delays_termination)
{
    osrm::engine::routing_algorithms::ch::SingleTargetEarlyStop bound;
    bound.addTarget(EdgeWeight{0}, EdgeDuration{0}, EdgeDistance{2}, EdgeWeight{0});
    bound.addSource(EdgeDuration{1}, EdgeDistance{-3}, EdgeWeight{0});
    BOOST_CHECK(!bound.shouldStop(EdgeWeight{100}, EdgeWeight{5}));
    bound.removeLabel(EdgeDuration{1}, EdgeDistance{-3});
    BOOST_CHECK(bound.shouldStop(EdgeWeight{100}, EdgeWeight{5}));
}

BOOST_AUTO_TEST_CASE(uses_minimum_metrics_from_all_target_buckets)
{
    osrm::engine::routing_algorithms::ch::SingleTargetEarlyStop bound;
    bound.addTarget(EdgeWeight{0}, EdgeDuration{5}, EdgeDistance{20}, EdgeWeight{0});
    bound.addTarget(EdgeWeight{20}, EdgeDuration{1}, EdgeDistance{2}, EdgeWeight{0});
    bound.addSource(EdgeDuration{-3}, EdgeDistance{0}, EdgeWeight{0});
    BOOST_CHECK(!bound.shouldStop(EdgeWeight{100}, EdgeWeight{5}));
    bound.removeLabel(EdgeDuration{-3}, EdgeDistance{0});
    BOOST_CHECK(bound.shouldStop(EdgeWeight{100}, EdgeWeight{5}));
}

BOOST_AUTO_TEST_CASE(changing_a_heap_label_updates_the_negative_metric_count)
{
    osrm::engine::routing_algorithms::ch::SingleTargetEarlyStop bound;
    bound.addTarget(EdgeWeight{0}, EdgeDuration{0}, EdgeDistance{0}, EdgeWeight{0});
    bound.addSource(EdgeDuration{-1}, EdgeDistance{0}, EdgeWeight{0});
    bound.insertLabel(EdgeDuration{0}, EdgeDistance{0});
    bound.removeLabel(EdgeDuration{0}, EdgeDistance{0});
    bound.insertLabel(EdgeDuration{-2}, EdgeDistance{-1});
    bound.removeLabel(EdgeDuration{-1}, EdgeDistance{0});
    BOOST_CHECK(!bound.shouldStop(EdgeWeight{100}, EdgeWeight{5}));
    bound.removeLabel(EdgeDuration{-2}, EdgeDistance{-1});
    BOOST_CHECK(bound.shouldStop(EdgeWeight{100}, EdgeWeight{5}));
}

BOOST_AUTO_TEST_CASE(widens_weight_arithmetic_and_keeps_constant_size_state)
{
    osrm::engine::routing_algorithms::ch::SingleTargetEarlyStop bound;
    const int maximum = std::numeric_limits<EdgeWeight::value_type>::max();
    bound.addTarget(EdgeWeight{100}, EdgeDuration{0}, EdgeDistance{0}, EdgeWeight{0});
    BOOST_CHECK(bound.shouldStop(EdgeWeight{maximum - 10}, EdgeWeight{maximum - 1}));
    static_assert(
        std::is_trivially_copyable_v<osrm::engine::routing_algorithms::ch::SingleTargetEarlyStop>);
    BOOST_CHECK_LE(sizeof(bound), 96);
}

BOOST_AUTO_TEST_CASE(approach_walks_cannot_hide_a_pending_negative_graph_metric)
{
    osrm::engine::routing_algorithms::ch::SingleTargetEarlyStop bound;
    bound.addTargetApproach(EdgeWeight{20}, EdgeDuration{3}, EdgeDistance{3});
    bound.addTarget(EdgeWeight{0}, EdgeDuration{5}, EdgeDistance{5}, EdgeWeight{20});
    bound.addSourceApproach(EdgeWeight{30}, EdgeDuration{3}, EdgeDistance{3});
    bound.addSource(EdgeDuration{0}, EdgeDistance{1}, EdgeWeight{30});
    BOOST_CHECK(!bound.shouldStop(EdgeWeight{100}, EdgeWeight{5}));
    bound.removeLabel(EdgeDuration{0}, EdgeDistance{1});
    bound.insertLabel(EdgeDuration{2}, EdgeDistance{2});
    BOOST_CHECK(bound.shouldStop(EdgeWeight{100}, EdgeWeight{5}));
}

BOOST_AUTO_TEST_SUITE_END()

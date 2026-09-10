#include "engine/isochrone/duration_graph_builder.hpp"

#include "util/exception.hpp"

#include <boost/test/unit_test.hpp>

#include <vector>

namespace
{

using osrm::engine::isochrone::buildDurationGraph;
using osrm::engine::isochrone::DurationGraph;
using osrm::extractor::IsochroneTransition;

DurationGraph build(const std::vector<IsochroneTransition> &transitions,
                    const std::vector<EdgeWeight> &node_weights,
                    const std::vector<EdgeDuration> &node_durations,
                    const std::vector<TurnPenalty> &turn_weight_penalties,
                    const std::vector<TurnPenalty> &turn_duration_penalties,
                    const std::vector<EdgeDuration> &node_duration_lower_bounds = {})
{
    return buildDurationGraph(static_cast<EdgeID>(node_weights.size()),
                              transitions,
                              node_weights,
                              node_durations,
                              turn_weight_penalties,
                              turn_duration_penalties,
                              node_duration_lower_bounds);
}

} // namespace

BOOST_AUTO_TEST_SUITE(duration_graph_builder)

BOOST_AUTO_TEST_CASE(parallel_transitions_keep_the_duration_winner)
{
    const auto graph =
        build({{0, 1, 0}, {0, 1, 1}}, {{100}, {0}}, {{10}, {0}}, {{0}, {0}}, {{50}, {5}});

    BOOST_REQUIRE_EQUAL(graph.forward_arcs.size(), 1);
    BOOST_CHECK_EQUAL(graph.forward_arcs.front().node, 1);
    BOOST_CHECK_EQUAL(graph.forward_arcs.front().duration, EdgeDuration{15});
    BOOST_REQUIRE_EQUAL(graph.reverse_arcs.size(), 1);
    BOOST_CHECK_EQUAL(graph.reverse_arcs.front().node, 0);
    BOOST_CHECK_EQUAL(graph.reverse_arcs.front().duration, EdgeDuration{15});
}

BOOST_AUTO_TEST_CASE(preserves_asymmetric_transition_durations_in_both_csrs)
{
    const auto graph =
        build({{0, 1, 0}, {1, 0, 1}}, {{100}, {100}}, {{10}, {100}}, {{0}, {0}}, {{1}, {2}});

    BOOST_REQUIRE_EQUAL(graph.forward_arcs.size(), 2);
    BOOST_CHECK_EQUAL(graph.forward_arcs[0].node, 1);
    BOOST_CHECK_EQUAL(graph.forward_arcs[0].duration, EdgeDuration{11});
    BOOST_CHECK_EQUAL(graph.forward_arcs[1].node, 0);
    BOOST_CHECK_EQUAL(graph.forward_arcs[1].duration, EdgeDuration{102});

    BOOST_REQUIRE_EQUAL(graph.reverse_arcs.size(), 2);
    BOOST_CHECK_EQUAL(graph.reverse_arcs[0].node, 1);
    BOOST_CHECK_EQUAL(graph.reverse_arcs[0].duration, EdgeDuration{102});
    BOOST_CHECK_EQUAL(graph.reverse_arcs[1].node, 0);
    BOOST_CHECK_EQUAL(graph.reverse_arcs[1].duration, EdgeDuration{11});
}

BOOST_AUTO_TEST_CASE(omits_a_transition_disabled_by_a_conditional_restriction)
{
    const auto graph = build({{0, 1, 0}, {0, 2, 1}},
                             {{100}, {0}, {0}},
                             {{10}, {0}, {0}},
                             {INVALID_TURN_PENALTY, {0}},
                             {{0}, {5}});

    BOOST_REQUIRE_EQUAL(graph.forward_arcs.size(), 1);
    BOOST_CHECK_EQUAL(graph.forward_arcs.front().node, 2);
    BOOST_CHECK_EQUAL(graph.forward_arcs.front().duration, EdgeDuration{15});
}

BOOST_AUTO_TEST_CASE(omits_a_transition_to_an_unavailable_geometry)
{
    const auto graph = build({{0, 1, 0}}, {{100}, INVALID_EDGE_WEIGHT}, {{10}, {0}}, {{0}}, {{0}});

    BOOST_CHECK(graph.forward_arcs.empty());
}

BOOST_AUTO_TEST_CASE(uses_metrics_after_traffic_reopens_an_initially_unavailable_source)
{
    const std::vector<IsochroneTransition> transitions = {{0, 1, 0}};
    const auto before_traffic =
        build(transitions, {INVALID_EDGE_WEIGHT, {0}}, {{10}, {0}}, {{0}}, {{0}});
    BOOST_CHECK(before_traffic.forward_arcs.empty());

    // The updater replaces an unavailable source metric when a speed update reopens its
    // geometry.  The duration graph is built from those post-update metrics.
    const auto after_traffic = build(transitions, {{25}, {0}}, {{25}, {0}}, {{0}}, {{0}});
    BOOST_REQUIRE_EQUAL(after_traffic.forward_arcs.size(), 1);
    BOOST_CHECK_EQUAL(after_traffic.forward_arcs.front().node, 1);
    BOOST_CHECK_EQUAL(after_traffic.forward_arcs.front().duration, EdgeDuration{25});
}

BOOST_AUTO_TEST_CASE(applies_a_traffic_duration_lower_bound_to_the_complete_transition)
{
    const auto graph = build({{0, 1, 0}}, {{100}, {0}}, {{1}, {0}}, {{0}}, {{0}}, {{2}, {0}});

    BOOST_REQUIRE_EQUAL(graph.forward_arcs.size(), 1);
    BOOST_CHECK_EQUAL(graph.forward_arcs.front().duration, EdgeDuration{2});
}

BOOST_AUTO_TEST_CASE(does_not_add_the_traffic_duration_lower_bound_to_a_positive_turn)
{
    const auto graph = build({{0, 1, 0}}, {{100}, {0}}, {{1}, {0}}, {{0}}, {{1}}, {{2}, {0}});

    BOOST_REQUIRE_EQUAL(graph.forward_arcs.size(), 1);
    BOOST_CHECK_EQUAL(graph.forward_arcs.front().duration, EdgeDuration{2});
}

BOOST_AUTO_TEST_CASE(applies_the_node_lower_bound_before_a_turn_when_the_route_clamp_triggers)
{
    const auto graph = build({{0, 1, 0}}, {{100}, {0}}, {{1}, {0}}, {{0}}, {{1}}, {{3}, {0}});

    BOOST_REQUIRE_EQUAL(graph.forward_arcs.size(), 1);
    BOOST_CHECK_EQUAL(graph.forward_arcs.front().duration, EdgeDuration{4});
}

BOOST_AUTO_TEST_CASE(rejects_duration_lower_bounds_with_the_wrong_node_count)
{
    BOOST_CHECK_THROW(build({{0, 1, 0}}, {{100}, {0}}, {{1}, {0}}, {{0}}, {{0}}, {{2}}),
                      osrm::util::exception);
}

BOOST_AUTO_TEST_CASE(rejects_an_enabled_negative_turn_duration_even_when_the_arc_is_nonnegative)
{
    BOOST_CHECK_THROW(build({{0, 1, 0}}, {{100}, {0}}, {{100}, {0}}, {{0}}, {{-1}}),
                      osrm::util::exception);
}

BOOST_AUTO_TEST_CASE(rejects_a_negative_duration_on_a_parallel_transition_dropped_by_routing)
{
    // The first transition could be the primary-weight winner that Updater visits.  The second
    // one is retained only by the isochrone sidecar, so it cannot inherit that clamp.
    BOOST_CHECK_THROW(
        build({{0, 1, 0}, {0, 1, 1}}, {{100}, {0}}, {{100}, {0}}, {{0}, {0}}, {{0}, {-1}}),
        osrm::util::exception);
}

BOOST_AUTO_TEST_CASE(ignores_a_negative_turn_duration_on_a_disabled_transition)
{
    const auto graph =
        build({{0, 1, 0}}, {{100}, {0}}, {{100}, {0}}, {INVALID_TURN_PENALTY}, {{-1}});

    BOOST_CHECK(graph.forward_arcs.empty());
}

BOOST_AUTO_TEST_SUITE_END()

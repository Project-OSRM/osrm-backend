#include "updater/updater.hpp"

#include "extractor/edge_based_graph_factory.hpp"
#include "extractor/files.hpp"
#include "engine/isochrone/duration_graph_builder.hpp"

#include "../common/temporary_file.hpp"

#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <fstream>
#include <vector>

namespace
{

struct TemporaryDirectory
{
    TemporaryDirectory() : path(std::filesystem::temp_directory_path() / random_string(8))
    { std::filesystem::create_directories(path); }

    ~TemporaryDirectory() { std::filesystem::remove_all(path); }

    std::filesystem::path path;
};

void writeMinimalUpdateDataset(
    const std::filesystem::path &base,
    const std::vector<osrm::extractor::EdgeBasedEdge> &edge_based_edges = {},
    const TurnPenalty turn_duration_penalty = TurnPenalty{0},
    const EdgeWeight source_weight = INVALID_EDGE_WEIGHT,
    const EdgeWeight target_weight = EdgeWeight{0})
{
    osrm::extractor::files::writeEdgeBasedGraph(
        base.string() + ".osrm.ebg", 2, edge_based_edges, 0);
    osrm::extractor::files::writeEdgeBasedNodeWeightsDurationsDistances(
        base.string() + ".osrm.enw",
        std::vector<EdgeWeight>{source_weight, target_weight},
        std::vector<EdgeDuration>{{10}, {0}},
        std::vector<EdgeDistance>{{1}, {0}});
    osrm::extractor::files::writeTurnWeightPenalty(base.string() + ".osrm.turn_weight_penalties",
                                                   std::vector<TurnPenalty>{{0}});
    osrm::extractor::files::writeTurnDurationPenalty(
        base.string() + ".osrm.turn_duration_penalties",
        std::vector<TurnPenalty>{turn_duration_penalty});

    osrm::extractor::ProfileProperties properties;
    osrm::extractor::files::writeProfileProperties(base.string() + ".osrm.properties", properties);

    std::vector<osrm::util::Coordinate> coordinates = {
        {osrm::util::FixedLongitude{0}, osrm::util::FixedLatitude{0}},
        {osrm::util::FixedLongitude{1}, osrm::util::FixedLatitude{0}},
        {osrm::util::FixedLongitude{2}, osrm::util::FixedLatitude{0}},
        {osrm::util::FixedLongitude{3}, osrm::util::FixedLatitude{0}}};
    osrm::extractor::PackedOSMIDs osm_node_ids = {{1}, {2}, {3}, {4}};
    osrm::extractor::files::writeNodes(
        base.string() + ".osrm.nbg_nodes", coordinates, osm_node_ids);

    std::vector<osrm::extractor::EdgeBasedNode> nodes = {{{0, true}, {1, false}, 0, false},
                                                         {{1, true}, {2, false}, 0, false}};
    std::vector<osrm::extractor::NodeBasedEdgeAnnotation> annotations(1);
    osrm::extractor::EdgeBasedNodeDataContainer node_data{std::move(nodes), std::move(annotations)};
    osrm::extractor::files::writeNodeData(base.string() + ".osrm.ebg_nodes", node_data);

    using SegmentData = osrm::extractor::SegmentDataContainer;
    SegmentData segment_data{{0, 2, 4},
                             {0, 1, 2, 3},
                             SegmentData::SegmentWeightVector{{0}, {10}, {0}, {10}},
                             SegmentData::SegmentWeightVector{{10}, {0}, {10}, {0}},
                             SegmentData::SegmentDurationVector{{0}, {10}, {0}, {10}},
                             SegmentData::SegmentDurationVector{{10}, {0}, {10}, {0}},
                             {0, 0, 0, 0},
                             {0, 0, 0, 0}};
    osrm::extractor::files::writeSegmentData(base.string() + ".osrm.geometry", segment_data);
}

} // namespace

BOOST_AUTO_TEST_SUITE(updater_isochrone_metrics)

BOOST_AUTO_TEST_CASE(traffic_reopens_a_raw_source_omitted_from_the_partitioned_graph)
{
    TemporaryDirectory directory;
    const auto base = directory.path / "test";
    writeMinimalUpdateDataset(base);

    TemporaryFile speeds;
    {
        std::ofstream stream(speeds.path);
        stream << "1,2,36\n";
    }

    osrm::updater::UpdaterConfig config;
    config.UseDefaultOutputNames(base.string() + ".osrm");
    config.segment_speed_lookup_paths = {speeds.path.string()};

    std::vector<osrm::extractor::EdgeBasedEdge> edge_based_edges;
    std::vector<EdgeWeight> node_weights;
    std::vector<EdgeDuration> node_durations;
    std::uint32_t checksum = 0;
    const std::vector<osrm::extractor::IsochroneTransition> transitions = {{0, 1, 0}};
    osrm::updater::TurnPenaltyMetrics penalties;
    const osrm::updater::EdgeExpandedGraphUpdateOptions options{&transitions, &penalties};

    const auto number_of_nodes = osrm::updater::Updater{config}.LoadAndUpdateEdgeExpandedGraph(
        edge_based_edges, node_weights, node_durations, checksum, options);

    BOOST_CHECK_EQUAL(number_of_nodes, 2);
    BOOST_CHECK(edge_based_edges.empty());
    BOOST_CHECK(node_weights[0] != INVALID_EDGE_WEIGHT);
    BOOST_CHECK(node_durations[0] != EdgeDuration{10});

    const auto graph = osrm::engine::isochrone::buildDurationGraph(number_of_nodes,
                                                                   transitions,
                                                                   node_weights,
                                                                   node_durations,
                                                                   penalties.weight_penalties,
                                                                   penalties.duration_penalties);
    BOOST_REQUIRE_EQUAL(graph.forward_arcs.size(), 1);
    BOOST_CHECK_EQUAL(graph.forward_arcs.front().node, 1);
}

BOOST_AUTO_TEST_CASE(traffic_reopens_a_raw_target_omitted_from_the_partitioned_graph)
{
    TemporaryDirectory directory;
    const auto base = directory.path / "test";
    writeMinimalUpdateDataset(base, {}, TurnPenalty{0}, EdgeWeight{10}, INVALID_EDGE_WEIGHT);

    TemporaryFile speeds;
    {
        std::ofstream stream(speeds.path);
        stream << "3,4,36\n";
    }

    osrm::updater::UpdaterConfig config;
    config.UseDefaultOutputNames(base.string() + ".osrm");
    config.segment_speed_lookup_paths = {speeds.path.string()};

    std::vector<osrm::extractor::EdgeBasedEdge> edge_based_edges;
    std::vector<EdgeWeight> node_weights;
    std::vector<EdgeDuration> node_durations;
    std::uint32_t checksum = 0;
    const std::vector<osrm::extractor::IsochroneTransition> transitions = {{0, 1, 0}};
    osrm::updater::TurnPenaltyMetrics penalties;
    const osrm::updater::EdgeExpandedGraphUpdateOptions options{&transitions, &penalties};

    const auto number_of_nodes = osrm::updater::Updater{config}.LoadAndUpdateEdgeExpandedGraph(
        edge_based_edges, node_weights, node_durations, checksum, options);

    BOOST_CHECK(node_weights[1] != INVALID_EDGE_WEIGHT);
    const auto graph = osrm::engine::isochrone::buildDurationGraph(number_of_nodes,
                                                                   transitions,
                                                                   node_weights,
                                                                   node_durations,
                                                                   penalties.weight_penalties,
                                                                   penalties.duration_penalties);
    BOOST_REQUIRE_EQUAL(graph.forward_arcs.size(), 1);
    BOOST_CHECK_EQUAL(graph.forward_arcs.front().node, 1);
}

BOOST_AUTO_TEST_CASE(traffic_updates_raw_targets_for_unpartitioned_ch_isochrone_data)
{
    TemporaryDirectory directory;
    const auto base = directory.path / "test";
    writeMinimalUpdateDataset(
        base,
        {{0, 1, 0, EdgeWeight{10}, EdgeDuration{10}, EdgeDistance{1}, true, false}},
        TurnPenalty{0},
        EdgeWeight{10},
        INVALID_EDGE_WEIGHT);

    TemporaryFile speeds;
    {
        std::ofstream stream(speeds.path);
        stream << "3,4,36\n";
    }

    osrm::updater::UpdaterConfig config;
    config.UseDefaultOutputNames(base.string() + ".osrm");
    config.segment_speed_lookup_paths = {speeds.path.string()};

    std::vector<osrm::extractor::EdgeBasedEdge> edge_based_edges;
    std::vector<EdgeWeight> node_weights;
    std::vector<EdgeDuration> node_durations;
    std::vector<osrm::extractor::IsochroneTransition> transitions;
    std::uint32_t checksum = 0;
    osrm::updater::TurnPenaltyMetrics penalties;
    const osrm::updater::EdgeExpandedGraphUpdateOptions options{nullptr, &penalties, &transitions};

    const auto number_of_nodes = osrm::updater::Updater{config}.LoadAndUpdateEdgeExpandedGraph(
        edge_based_edges, node_weights, node_durations, checksum, options);

    BOOST_REQUIRE_EQUAL(transitions.size(), 1);
    BOOST_CHECK_EQUAL(transitions.front().source, 0);
    BOOST_CHECK_EQUAL(transitions.front().target, 1);
    BOOST_CHECK(node_weights[1] != INVALID_EDGE_WEIGHT);

    const auto graph = osrm::engine::isochrone::buildDurationGraph(number_of_nodes,
                                                                   transitions,
                                                                   node_weights,
                                                                   node_durations,
                                                                   penalties.weight_penalties,
                                                                   penalties.duration_penalties);
    BOOST_REQUIRE_EQUAL(graph.forward_arcs.size(), 1);
    BOOST_CHECK_EQUAL(graph.forward_arcs.front().node, 1);
}

BOOST_AUTO_TEST_CASE(turn_update_skips_a_coalesced_away_raw_transition)
{
    TemporaryDirectory directory;
    const auto base = directory.path / "test";

    // A partitioned graph retains only turn 0, while isochrone data keeps raw turn 1 as a
    // parallel transition. The turn-penalty index still addresses the uncoalesced turn IDs.
    writeMinimalUpdateDataset(
        base,
        {{0, 1, 0, EdgeWeight{10}, EdgeDuration{10}, EdgeDistance{1}, true, false}},
        TurnPenalty{0},
        EdgeWeight{10},
        INVALID_EDGE_WEIGHT);
    osrm::extractor::files::writeTurnWeightPenalty(base.string() + ".osrm.turn_weight_penalties",
                                                   std::vector<TurnPenalty>{{0}, {0}});
    osrm::extractor::files::writeTurnDurationPenalty(
        base.string() + ".osrm.turn_duration_penalties", std::vector<TurnPenalty>{{0}, {0}});
    osrm::extractor::files::writeTurnPenaltiesIndex(
        base.string() + ".osrm.turn_penalties_index",
        std::vector<osrm::extractor::lookup::TurnIndexBlock>{{0, 1, 2}, {1, 2, 3}});

    TemporaryFile turns;
    {
        std::ofstream stream(turns.path);
        stream << "2,3,4,7,11\n";
    }

    osrm::updater::UpdaterConfig config;
    config.UseDefaultOutputNames(base.string() + ".osrm");
    config.turn_penalty_lookup_paths = {turns.path.string()};

    std::vector<osrm::extractor::EdgeBasedEdge> edge_based_edges;
    std::vector<EdgeWeight> node_weights;
    std::vector<EdgeDuration> node_durations;
    std::uint32_t checksum = 0;
    const std::vector<osrm::extractor::IsochroneTransition> transitions = {{0, 1, 0}, {0, 1, 1}};
    osrm::updater::TurnPenaltyMetrics penalties;
    const osrm::updater::EdgeExpandedGraphUpdateOptions options{&transitions, &penalties};

    osrm::updater::Updater{config}.LoadAndUpdateEdgeExpandedGraph(
        edge_based_edges, node_weights, node_durations, checksum, options);

    BOOST_REQUIRE_EQUAL(edge_based_edges.size(), 1);
    BOOST_CHECK_EQUAL(edge_based_edges.front().data.turn_id, 0);
    BOOST_REQUIRE_EQUAL(penalties.weight_penalties.size(), 2);
    BOOST_REQUIRE_EQUAL(penalties.duration_penalties.size(), 2);
    BOOST_CHECK_EQUAL(penalties.weight_penalties[1], TurnPenalty{110});
    BOOST_CHECK_EQUAL(penalties.duration_penalties[1], TurnPenalty{70});
}

BOOST_AUTO_TEST_CASE(returns_the_final_in_memory_penalty_after_a_traffic_only_clamp)
{
    TemporaryDirectory directory;
    const auto base = directory.path / "test";
    writeMinimalUpdateDataset(
        base,
        {{0, 1, 0, EdgeWeight{10}, EdgeDuration{10}, EdgeDistance{1}, true, false}},
        TurnPenalty{-10});

    TemporaryFile speeds;
    {
        std::ofstream stream(speeds.path);
        stream << "1,2,36\n";
    }

    osrm::updater::UpdaterConfig config;
    config.UseDefaultOutputNames(base.string() + ".osrm");
    config.segment_speed_lookup_paths = {speeds.path.string()};

    std::vector<osrm::extractor::EdgeBasedEdge> edge_based_edges;
    std::vector<EdgeWeight> node_weights;
    std::vector<EdgeDuration> node_durations;
    std::uint32_t checksum = 0;
    osrm::updater::TurnPenaltyMetrics penalties;
    const osrm::updater::EdgeExpandedGraphUpdateOptions options{nullptr, &penalties};

    osrm::updater::Updater{config}.LoadAndUpdateEdgeExpandedGraph(
        edge_based_edges, node_weights, node_durations, checksum, options);

    BOOST_REQUIRE_EQUAL(penalties.duration_penalties.size(), 1);
    BOOST_CHECK_EQUAL(penalties.duration_penalties.front(), TurnPenalty{1});
    BOOST_REQUIRE_EQUAL(edge_based_edges.size(), 1);
    BOOST_CHECK_EQUAL(edge_based_edges.front().data.duration, 2);
}

BOOST_AUTO_TEST_CASE(returns_the_duration_lower_bound_applied_to_a_traffic_update)
{
    TemporaryDirectory directory;
    const auto base = directory.path / "test";
    writeMinimalUpdateDataset(
        base, {{0, 1, 0, EdgeWeight{10}, EdgeDuration{10}, EdgeDistance{1}, true, false}});

    TemporaryFile speeds;
    {
        std::ofstream stream(speeds.path);
        stream << "1,2,36\n";
    }

    osrm::updater::UpdaterConfig config;
    config.UseDefaultOutputNames(base.string() + ".osrm");
    config.segment_speed_lookup_paths = {speeds.path.string()};

    std::vector<osrm::extractor::EdgeBasedEdge> edge_based_edges;
    std::vector<EdgeWeight> node_weights;
    std::vector<EdgeDuration> node_durations;
    std::vector<EdgeDuration> node_duration_lower_bounds;
    std::uint32_t checksum = 0;
    const std::vector<osrm::extractor::IsochroneTransition> transitions = {{0, 1, 0}};
    osrm::updater::TurnPenaltyMetrics penalties;
    const osrm::updater::EdgeExpandedGraphUpdateOptions options{
        &transitions, &penalties, nullptr, &node_duration_lower_bounds};

    const auto number_of_nodes = osrm::updater::Updater{config}.LoadAndUpdateEdgeExpandedGraph(
        edge_based_edges, node_weights, node_durations, checksum, options);

    BOOST_REQUIRE_EQUAL(edge_based_edges.size(), 1);
    BOOST_CHECK_EQUAL(edge_based_edges.front().data.duration, 2);
    BOOST_REQUIRE_EQUAL(node_duration_lower_bounds.size(), 2);
    BOOST_CHECK_EQUAL(node_duration_lower_bounds[0], EdgeDuration{2});

    const auto graph = osrm::engine::isochrone::buildDurationGraph(number_of_nodes,
                                                                   transitions,
                                                                   node_weights,
                                                                   node_durations,
                                                                   penalties.weight_penalties,
                                                                   penalties.duration_penalties,
                                                                   node_duration_lower_bounds);
    BOOST_REQUIRE_EQUAL(graph.forward_arcs.size(), 1);
    BOOST_CHECK_EQUAL(graph.forward_arcs.front().duration,
                      EdgeDuration{edge_based_edges.front().data.duration});
}

BOOST_AUTO_TEST_SUITE_END()

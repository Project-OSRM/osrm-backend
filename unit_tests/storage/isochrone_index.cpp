#include "storage/shared_data_index.hpp"
#include "storage/shared_datatype.hpp"
#include "storage/view_factory.hpp"

#include "contractor/query_graph.hpp"

#include "engine/isochrone/duration_graph.hpp"

#include "util/exception.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace
{
constexpr const char *METRIC_PREFIX = "/ch/metrics/duration";

class ViewIndex
{
  public:
    explicit ViewIndex(std::unique_ptr<osrm::storage::BaseDataLayout> layout)
        : memory(layout->GetSizeOfLayout()), index(makeIndex(memory.data(), std::move(layout)))
    {
    }

    template <typename T, std::size_t Size>
    void write(const std::string &name, const std::array<T, Size> &values)
    {
        BOOST_REQUIRE_EQUAL(index.GetBlockEntries(name), Size);
        std::copy(values.begin(), values.end(), index.GetBlockPtr<T>(name));
    }

    const osrm::storage::SharedDataIndex &get() const { return index; }

  private:
    static osrm::storage::SharedDataIndex
    makeIndex(char *memory, std::unique_ptr<osrm::storage::BaseDataLayout> layout)
    {
        std::vector<osrm::storage::SharedDataIndex::AllocatedRegion> regions;
        regions.push_back({memory, std::move(layout)});
        return {std::move(regions)};
    }

    std::vector<char> memory;
    osrm::storage::SharedDataIndex index;
};

std::unique_ptr<osrm::storage::BaseDataLayout> makeMetricLayout(const std::size_t block_count)
{
    using Arc = osrm::engine::isochrone::DurationGraphArc;
    using NodeArrayEntry = osrm::contractor::QueryGraphView::NodeArrayEntry;
    using EdgeArrayEntry = osrm::contractor::QueryGraphView::EdgeArrayEntry;

    auto layout = std::make_unique<osrm::storage::ContiguousDataLayout>();
    layout->SetBlock(std::string(METRIC_PREFIX) + "/contracted_graph/node_array",
                     osrm::storage::make_block<NodeArrayEntry>(4));
    layout->SetBlock(std::string(METRIC_PREFIX) + "/contracted_graph/edge_array",
                     osrm::storage::make_block<EdgeArrayEntry>(2));

    const std::array names = {std::string(METRIC_PREFIX) + "/isochrone/forward_offsets",
                              std::string(METRIC_PREFIX) + "/isochrone/forward_arcs",
                              std::string(METRIC_PREFIX) + "/isochrone/reverse_offsets",
                              std::string(METRIC_PREFIX) + "/isochrone/reverse_arcs"};
    if (block_count > 0)
        layout->SetBlock(names[0], osrm::storage::make_block<::EdgeID>(4));
    if (block_count > 1)
        layout->SetBlock(names[1], osrm::storage::make_block<Arc>(2));
    if (block_count > 2)
        layout->SetBlock(names[2], osrm::storage::make_block<::EdgeID>(4));
    if (block_count > 3)
        layout->SetBlock(names[3], osrm::storage::make_block<Arc>(2));
    return layout;
}

std::unique_ptr<osrm::storage::BaseDataLayout> makeEmptyIsochroneGraphLayout()
{
    using Arc = osrm::engine::isochrone::DurationGraphArc;
    using NodeArrayEntry = osrm::contractor::QueryGraphView::NodeArrayEntry;
    using EdgeArrayEntry = osrm::contractor::QueryGraphView::EdgeArrayEntry;

    auto layout = std::make_unique<osrm::storage::ContiguousDataLayout>();
    layout->SetBlock(std::string(METRIC_PREFIX) + "/contracted_graph/node_array",
                     osrm::storage::make_block<NodeArrayEntry>(4));
    layout->SetBlock(std::string(METRIC_PREFIX) + "/contracted_graph/edge_array",
                     osrm::storage::make_block<EdgeArrayEntry>(2));

    const std::array names = {std::string(METRIC_PREFIX) + "/isochrone/forward_offsets",
                              std::string(METRIC_PREFIX) + "/isochrone/forward_arcs",
                              std::string(METRIC_PREFIX) + "/isochrone/reverse_offsets",
                              std::string(METRIC_PREFIX) + "/isochrone/reverse_arcs"};
    layout->SetBlock(names[0], osrm::storage::make_block<::EdgeID>(0));
    layout->SetBlock(names[1], osrm::storage::make_block<Arc>(0));
    layout->SetBlock(names[2], osrm::storage::make_block<::EdgeID>(0));
    layout->SetBlock(names[3], osrm::storage::make_block<Arc>(0));
    return layout;
}

std::unique_ptr<osrm::storage::BaseDataLayout> makeEmptyMldIsochroneGraphLayout()
{
    using Arc = osrm::engine::isochrone::DurationGraphArc;
    using Graph = osrm::customizer::MultiLevelEdgeBasedGraphView;

    constexpr auto GRAPH_NAME = "/mld/multilevelgraph";
    auto layout = std::make_unique<osrm::storage::ContiguousDataLayout>();
    layout->SetBlock(std::string(GRAPH_NAME) + "/node_array",
                     osrm::storage::make_block<Graph::NodeArrayEntry>(4));
    layout->SetBlock(std::string(GRAPH_NAME) + "/edge_array",
                     osrm::storage::make_block<Graph::EdgeArrayEntry>(2));
    layout->SetBlock(std::string(GRAPH_NAME) + "/node_to_edge_offset",
                     osrm::storage::make_block<Graph::EdgeOffset>(1));
    layout->SetBlock(std::string(GRAPH_NAME) + "/node_weights",
                     osrm::storage::make_block<::EdgeWeight>(3));
    layout->SetBlock(std::string(GRAPH_NAME) + "/node_durations",
                     osrm::storage::make_block<::EdgeDuration>(3));
    layout->SetBlock(std::string(GRAPH_NAME) + "/node_distances",
                     osrm::storage::make_block<::EdgeDistance>(3));
    layout->SetBlock(std::string(GRAPH_NAME) + "/is_forward_edge",
                     osrm::storage::make_block<bool>(2));
    layout->SetBlock(std::string(GRAPH_NAME) + "/is_backward_edge",
                     osrm::storage::make_block<bool>(2));

    const std::array names = {std::string(GRAPH_NAME) + "/isochrone/forward_offsets",
                              std::string(GRAPH_NAME) + "/isochrone/forward_arcs",
                              std::string(GRAPH_NAME) + "/isochrone/reverse_offsets",
                              std::string(GRAPH_NAME) + "/isochrone/reverse_arcs"};
    layout->SetBlock(names[0], osrm::storage::make_block<::EdgeID>(0));
    layout->SetBlock(names[1], osrm::storage::make_block<Arc>(0));
    layout->SetBlock(names[2], osrm::storage::make_block<::EdgeID>(0));
    layout->SetBlock(names[3], osrm::storage::make_block<Arc>(0));
    return layout;
}

std::unique_ptr<osrm::storage::BaseDataLayout> makeLegacyDurationOnlyMetricLayout()
{
    auto layout = makeMetricLayout(4);
    constexpr auto legacy_arc_size = sizeof(::NodeID) + sizeof(::EdgeDuration);
    layout->SetBlock(std::string(METRIC_PREFIX) + "/isochrone/forward_arcs",
                     osrm::storage::Block{2, 2 * legacy_arc_size});
    layout->SetBlock(std::string(METRIC_PREFIX) + "/isochrone/reverse_arcs",
                     osrm::storage::Block{2, 2 * legacy_arc_size});
    return layout;
}

std::unique_ptr<osrm::storage::BaseDataLayout> makeLegacyDurationOnlyMldLayout()
{
    constexpr auto GRAPH_NAME = "/mld/multilevelgraph";
    auto layout = makeEmptyMldIsochroneGraphLayout();
    constexpr auto legacy_arc_size = sizeof(::NodeID) + sizeof(::EdgeDuration);
    layout->SetBlock(std::string(GRAPH_NAME) + "/isochrone/forward_offsets",
                     osrm::storage::make_block<::EdgeID>(4));
    layout->SetBlock(std::string(GRAPH_NAME) + "/isochrone/forward_arcs",
                     osrm::storage::Block{2, 2 * legacy_arc_size});
    layout->SetBlock(std::string(GRAPH_NAME) + "/isochrone/reverse_offsets",
                     osrm::storage::make_block<::EdgeID>(4));
    layout->SetBlock(std::string(GRAPH_NAME) + "/isochrone/reverse_arcs",
                     osrm::storage::Block{2, 2 * legacy_arc_size});
    return layout;
}

void writeQueryGraph(ViewIndex &view)
{
    using NodeArrayEntry = osrm::contractor::QueryGraphView::NodeArrayEntry;
    using EdgeArrayEntry = osrm::contractor::QueryGraphView::EdgeArrayEntry;
    using EdgeData = osrm::contractor::QueryEdge::EdgeData;

    const std::array<NodeArrayEntry, 4> nodes = {{{0}, {1}, {2}, {2}}};
    const std::array<EdgeArrayEntry, 2> edges = {{{1, EdgeData{}}, {2, EdgeData{}}}};
    view.write(std::string(METRIC_PREFIX) + "/contracted_graph/node_array", nodes);
    view.write(std::string(METRIC_PREFIX) + "/contracted_graph/edge_array", edges);
}

void writeValidIsochroneGraph(ViewIndex &view)
{
    using Arc = osrm::engine::isochrone::DurationGraphArc;
    const std::array<::EdgeID, 4> forward_offsets = {0, 1, 2, 2};
    const std::array<Arc, 2> forward_arcs = {
        {{1, ::EdgeWeight{10}, ::EdgeDuration{10}}, {2, ::EdgeWeight{20}, ::EdgeDuration{20}}}};
    const std::array<::EdgeID, 4> reverse_offsets = {0, 0, 1, 2};
    const std::array<Arc, 2> reverse_arcs = {
        {{0, ::EdgeWeight{10}, ::EdgeDuration{10}}, {1, ::EdgeWeight{20}, ::EdgeDuration{20}}}};

    view.write(std::string(METRIC_PREFIX) + "/isochrone/forward_offsets", forward_offsets);
    view.write(std::string(METRIC_PREFIX) + "/isochrone/forward_arcs", forward_arcs);
    view.write(std::string(METRIC_PREFIX) + "/isochrone/reverse_offsets", reverse_offsets);
    view.write(std::string(METRIC_PREFIX) + "/isochrone/reverse_arcs", reverse_arcs);
}
} // namespace

BOOST_AUTO_TEST_SUITE(isochrone_index)

BOOST_AUTO_TEST_CASE(mmap_metric_view_accepts_a_legacy_metric)
{
    ViewIndex view(makeMetricLayout(0));
    writeQueryGraph(view);

    BOOST_CHECK_NO_THROW(osrm::storage::validateIsochroneIndex(view.get()));
    const auto metric = osrm::storage::make_contracted_metric_view(view.get(), METRIC_PREFIX);
    BOOST_CHECK(metric.isochrone_graph.empty());
}

BOOST_AUTO_TEST_CASE(mmap_metric_view_rejects_a_partial_isochrone_graph)
{
    ViewIndex view(makeMetricLayout(1));

    BOOST_CHECK_THROW(osrm::storage::make_contracted_metric_view(view.get(), METRIC_PREFIX),
                      osrm::util::exception);
    BOOST_CHECK_THROW(osrm::storage::validateIsochroneIndex(view.get()), osrm::util::exception);
}

BOOST_AUTO_TEST_CASE(mmap_metric_view_rejects_a_present_but_empty_isochrone_graph)
{
    ViewIndex view(makeEmptyIsochroneGraphLayout());
    writeQueryGraph(view);

    BOOST_CHECK_THROW(osrm::storage::make_contracted_metric_view(view.get(), METRIC_PREFIX),
                      osrm::util::exception);
    BOOST_CHECK_THROW(osrm::storage::validateIsochroneIndex(view.get()), osrm::util::exception);
}

BOOST_AUTO_TEST_CASE(mmap_mld_graph_view_rejects_a_present_but_empty_isochrone_graph)
{
    ViewIndex view(makeEmptyMldIsochroneGraphLayout());

    BOOST_CHECK_THROW(
        osrm::storage::make_multi_level_graph_view(view.get(), "/mld/multilevelgraph"),
        osrm::util::exception);
    BOOST_CHECK_THROW(osrm::storage::validateIsochroneIndex(view.get()), osrm::util::exception);
}

BOOST_AUTO_TEST_CASE(mmap_metric_view_rejects_legacy_duration_only_arc_blocks)
{
    ViewIndex view(makeLegacyDurationOnlyMetricLayout());

    BOOST_CHECK_THROW(osrm::storage::make_contracted_metric_view(view.get(), METRIC_PREFIX),
                      osrm::util::exception);
    BOOST_CHECK_THROW(osrm::storage::validateIsochroneIndex(view.get()), osrm::util::exception);
}

BOOST_AUTO_TEST_CASE(mmap_mld_graph_view_rejects_legacy_duration_only_arc_blocks)
{
    ViewIndex view(makeLegacyDurationOnlyMldLayout());

    BOOST_CHECK_THROW(
        osrm::storage::make_multi_level_graph_view(view.get(), "/mld/multilevelgraph"),
        osrm::util::exception);
    BOOST_CHECK_THROW(osrm::storage::validateIsochroneIndex(view.get()), osrm::util::exception);
}

BOOST_AUTO_TEST_CASE(mmap_metric_view_rejects_a_semantically_corrupt_isochrone_graph)
{
    using Arc = osrm::engine::isochrone::DurationGraphArc;
    ViewIndex view(makeMetricLayout(4));
    writeQueryGraph(view);
    writeValidIsochroneGraph(view);

    BOOST_CHECK_NO_THROW(osrm::storage::validateIsochroneIndex(view.get()));

    const std::array<Arc, 2> corrupt_reverse_arcs = {
        {{0, ::EdgeWeight{10}, ::EdgeDuration{11}}, {1, ::EdgeWeight{20}, ::EdgeDuration{20}}}};
    view.write(std::string(METRIC_PREFIX) + "/isochrone/reverse_arcs", corrupt_reverse_arcs);
    BOOST_CHECK_THROW(osrm::storage::validateIsochroneIndex(view.get()), osrm::util::exception);
}

BOOST_AUTO_TEST_CASE(mmap_metric_view_rejects_an_isochrone_graph_with_a_nonpositive_weight)
{
    using Arc = osrm::engine::isochrone::DurationGraphArc;
    ViewIndex view(makeMetricLayout(4));
    writeQueryGraph(view);
    writeValidIsochroneGraph(view);

    const std::array<Arc, 2> invalid_forward_arcs = {
        {{1, ::EdgeWeight{0}, ::EdgeDuration{10}}, {2, ::EdgeWeight{20}, ::EdgeDuration{20}}}};
    view.write(std::string(METRIC_PREFIX) + "/isochrone/forward_arcs", invalid_forward_arcs);

    BOOST_CHECK_THROW(osrm::storage::validateIsochroneIndex(view.get()), osrm::util::exception);
}

BOOST_AUTO_TEST_SUITE_END()

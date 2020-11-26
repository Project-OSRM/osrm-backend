#ifndef OSRM_PARTITIONER_MULTI_LEVEL_GRAPH_HPP
#define OSRM_PARTITIONER_MULTI_LEVEL_GRAPH_HPP

#include "partitioner/edge_based_graph.hpp"
#include "partitioner/multi_level_partition.hpp"

#include "storage/shared_memory_ownership.hpp"
#include "storage/tar_fwd.hpp"

#include "util/static_graph.hpp"
#include "util/vector_view.hpp"

#include <tbb/parallel_sort.h>

#include <boost/iterator/permutation_iterator.hpp>
#include <boost/range/combine.hpp>

namespace osrm
{

namespace partitioner
{
template <typename EdgeDataT, storage::Ownership Ownership> class MultiLevelGraph;

namespace serialization
{
template <typename EdgeDataT, storage::Ownership Ownership>
void read(storage::tar::FileReader &reader,
          const std::string &name,
          MultiLevelGraph<EdgeDataT, Ownership> &graph);

template <typename EdgeDataT, storage::Ownership Ownership>
void write(storage::tar::FileWriter &writer,
           const std::string &name,
           const MultiLevelGraph<EdgeDataT, Ownership> &graph);
} // namespace serialization

template <typename EdgeDataT, storage::Ownership Ownership>
class MultiLevelGraph : public util::StaticGraph<EdgeDataT, Ownership>
{
  private:
    using SuperT = util::StaticGraph<EdgeDataT, Ownership>;
    template <typename T> using Vector = util::ViewOrVector<T, Ownership>;

  public:
    using SuperT::SuperT;

    // We limit each node to have 255 edges
    // this is very generous, we could probably pack this
    using EdgeOffset = std::uint8_t;

    MultiLevelGraph() = default;
    MultiLevelGraph(MultiLevelGraph &&) = default;
    MultiLevelGraph(const MultiLevelGraph &) = default;
    MultiLevelGraph &operator=(MultiLevelGraph &&) = default;
    MultiLevelGraph &operator=(const MultiLevelGraph &) = default;

    MultiLevelGraph(Vector<typename SuperT::NodeArrayEntry> node_array_,
                    Vector<typename SuperT::EdgeArrayEntry> edge_array_,
                    Vector<EdgeOffset> node_to_edge_offset_)
        : SuperT(std::move(node_array_), std::move(edge_array_)),
          node_to_edge_offset(std::move(node_to_edge_offset_))
    {
    }

    template <typename ContainerT>
    MultiLevelGraph(const MultiLevelPartition &mlp,
                    const std::uint32_t num_nodes,
                    const ContainerT &edges)
    {
        auto highest_border_level = GetHighestBorderLevel(mlp, edges);
        auto permutation = SortEdgesByHighestLevel(highest_border_level, edges);
        auto sorted_edges_begin =
            boost::make_permutation_iterator(edges.begin(), permutation.begin());
        auto sorted_edges_end = boost::make_permutation_iterator(edges.begin(), permutation.end());
        SuperT::InitializeFromSortedEdgeRange(num_nodes, sorted_edges_begin, sorted_edges_end);

        // if the node ordering is sorting the border nodes first,
        // the id of the maximum border node will be rather low
        // enabling us to save some memory here
        auto max_border_node_id = 0u;
        for (auto edge_index : util::irange<std::size_t>(0, edges.size()))
        {
            if (highest_border_level[edge_index] > 0)
            {
                max_border_node_id =
                    std::max(max_border_node_id,
                             std::max(edges[edge_index].source, edges[edge_index].target));
            }
        }
        BOOST_ASSERT(max_border_node_id < num_nodes);

        auto edge_and_level_range = boost::combine(edges, highest_border_level);
        auto sorted_edge_and_level_begin =
            boost::make_permutation_iterator(edge_and_level_range.begin(), permutation.begin());
        auto sorted_edge_and_level_end =
            boost::make_permutation_iterator(edge_and_level_range.begin(), permutation.end());
        InitializeOffsetsFromSortedEdges(
            mlp, max_border_node_id, sorted_edge_and_level_begin, sorted_edge_and_level_end);
    }

    // Fast scan over all relevant border edges
    // For level 0 this yield the same result as GetAdjacentEdgeRange
    auto GetBorderEdgeRange(const LevelID level, const NodeID node) const
    {
        auto begin = BeginBorderEdges(level, node);
        auto end = SuperT::EndEdges(node);
        return util::irange<EdgeID>(begin, end);
    }

    // Fast scan over all relevant internal edges, that is edges that will not
    // leave the cell of that node at the given level
    // For level 0 this returns an empty edge range
    auto GetInternalEdgeRange(const LevelID level, const NodeID node) const
    {
        auto begin = SuperT::BeginEdges(node);
        auto end = BeginBorderEdges(level, node);
        return util::irange<EdgeID>(begin, end);
    }

    EdgeID BeginBorderEdges(const LevelID level, const NodeID node) const
    {
        auto index = node * GetNumberOfLevels();
        // `node_to_edge_offset` has size `max_border_node_id + 1`
        // which can be smaller then the total number of nodes.
        // this will save memory in case we sort the border nodes first
        if (index >= node_to_edge_offset.size() - 1)
        {
            // On level 0 all edges are border edges
            if (level == 0)
                return SuperT::BeginEdges(node);
            else
                return SuperT::EndEdges(node);
        }
        else
        {
            return SuperT::BeginEdges(node) + node_to_edge_offset[index + level];
        }
    }

    // We save the level as sentinel at the end
    LevelID GetNumberOfLevels() const { return node_to_edge_offset.back(); }

    NodeID GetMaxBorderNodeID() const
    {
        auto num_levels = GetNumberOfLevels();
        BOOST_ASSERT((node_to_edge_offset.size() - 1) % num_levels == 0);
        auto max_border_node_id = (node_to_edge_offset.size() - 1) / num_levels - 1;
        return max_border_node_id;
    }

    auto data() && // rvalue ref-qualifier is a safety-belt
    {
        return std::make_tuple(std::move(SuperT::node_array),
                               std::move(SuperT::edge_array),
                               std::move(node_to_edge_offset),
                               connectivity_checksum);
    }

  private:
    template <typename ContainerT>
    auto GetHighestBorderLevel(const MultiLevelPartition &mlp, const ContainerT &edges) const
    {
        std::vector<LevelID> highest_border_level(edges.size());
        std::transform(
            edges.begin(), edges.end(), highest_border_level.begin(), [&mlp](const auto &edge) {
                return mlp.GetHighestDifferentLevel(edge.source, edge.target);
            });
        return highest_border_level;
    }

    template <typename ContainerT>
    auto SortEdgesByHighestLevel(const std::vector<LevelID> &highest_border_level,
                                 const ContainerT &edges) const
    {
        std::vector<std::uint32_t> permutation(edges.size());
        std::iota(permutation.begin(), permutation.end(), 0);
        tbb::parallel_sort(
            permutation.begin(),
            permutation.end(),
            [&edges, &highest_border_level](const auto &lhs, const auto &rhs) {
                // sort by source node and then by level in ascending order
                return std::tie(edges[lhs].source, highest_border_level[lhs], edges[lhs].target) <
                       std::tie(edges[rhs].source, highest_border_level[rhs], edges[rhs].target);
            });

        return permutation;
    }

    template <typename ZipIterT>
    auto InitializeOffsetsFromSortedEdges(const MultiLevelPartition &mlp,
                                          const NodeID max_border_node_id,
                                          ZipIterT edge_and_level_begin,
                                          ZipIterT edge_and_level_end)
    {

        auto num_levels = mlp.GetNumberOfLevels();
        // we save one sentinel element at the end
        node_to_edge_offset.reserve(num_levels * (max_border_node_id + 1) + 1);
        auto iter = edge_and_level_begin;
        for (auto node : util::irange<NodeID>(0, max_border_node_id + 1))
        {
            node_to_edge_offset.push_back(0);
            auto level_begin = iter;
            for (auto level : util::irange<LevelID>(0, mlp.GetNumberOfLevels()))
            {
                iter = std::find_if(
                    iter, edge_and_level_end, [node, level](const auto &edge_and_level) {
                        return boost::get<0>(edge_and_level).source != node ||
                               boost::get<1>(edge_and_level) != level;
                    });
                EdgeOffset offset = std::distance(level_begin, iter);
                node_to_edge_offset.push_back(offset);
            }
            node_to_edge_offset.pop_back();
        }
        BOOST_ASSERT(node_to_edge_offset.size() ==
                     mlp.GetNumberOfLevels() * (max_border_node_id + 1));
        // save number of levels as last element so we can reconstruct the stride
        node_to_edge_offset.push_back(mlp.GetNumberOfLevels());
    }

    friend void
    serialization::read<EdgeDataT, Ownership>(storage::tar::FileReader &reader,
                                              const std::string &name,
                                              MultiLevelGraph<EdgeDataT, Ownership> &graph);
    friend void
    serialization::write<EdgeDataT, Ownership>(storage::tar::FileWriter &writer,
                                               const std::string &name,
                                               const MultiLevelGraph<EdgeDataT, Ownership> &graph);

  protected:
    Vector<EdgeOffset> node_to_edge_offset;
    std::uint32_t connectivity_checksum;
};

using MultiLevelEdgeBasedGraph =
    MultiLevelGraph<EdgeBasedGraphEdgeData, storage::Ownership::Container>;
} // namespace partitioner
} // namespace osrm

#endif

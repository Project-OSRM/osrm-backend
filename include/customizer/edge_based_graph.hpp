#ifndef OSRM_CUSTOMIZE_EDGE_BASED_GRAPH_HPP
#define OSRM_CUSTOMIZE_EDGE_BASED_GRAPH_HPP

#include "extractor/edge_based_edge.hpp"
#include "partitioner/edge_based_graph.hpp"
#include "partitioner/multi_level_graph.hpp"
#include "util/static_graph.hpp"
#include "util/typedefs.hpp"

#include "storage/shared_memory_ownership.hpp"

#include <boost/filesystem/path.hpp>

namespace osrm
{
namespace customizer
{

struct EdgeBasedGraphEdgeData
{
    NodeID turn_id; // ID of the edge based node (node based edge)
};

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
}

template <typename EdgeDataT, storage::Ownership Ownership>
class MultiLevelGraph : public partitioner::MultiLevelGraph<EdgeDataT, Ownership>
{
  private:
    using SuperT = partitioner::MultiLevelGraph<EdgeDataT, Ownership>;
    using PartitionerGraphT = partitioner::MultiLevelGraph<partitioner::EdgeBasedGraphEdgeData,
                                                           storage::Ownership::Container>;
    template <typename T> using Vector = util::ViewOrVector<T, Ownership>;

  public:
    using NodeArrayEntry = typename SuperT::NodeArrayEntry;
    using EdgeArrayEntry = typename SuperT::EdgeArrayEntry;
    using EdgeOffset = typename SuperT::EdgeOffset;

    MultiLevelGraph() = default;
    MultiLevelGraph(MultiLevelGraph &&) = default;
    MultiLevelGraph(const MultiLevelGraph &) = default;
    MultiLevelGraph &operator=(MultiLevelGraph &&) = default;
    MultiLevelGraph &operator=(const MultiLevelGraph &) = default;

    MultiLevelGraph(PartitionerGraphT &&graph,
                    Vector<EdgeWeight> node_weights_,
                    Vector<EdgeDuration> node_durations_,
                    Vector<EdgeDistance> node_distances_)
        : node_weights(std::move(node_weights_)), node_durations(std::move(node_durations_)),
          node_distances(std::move(node_distances_))
    {
        util::ViewOrVector<PartitionerGraphT::EdgeArrayEntry, storage::Ownership::Container>
            original_edge_array;

        std::tie(SuperT::node_array,
                 original_edge_array,
                 SuperT::node_to_edge_offset,
                 SuperT::connectivity_checksum) = std::move(graph).data();

        SuperT::edge_array.reserve(original_edge_array.size());
        for (const auto &edge : original_edge_array)
        {
            SuperT::edge_array.push_back({edge.target, {edge.data.turn_id}});
            is_forward_edge.push_back(edge.data.forward);
            is_backward_edge.push_back(edge.data.backward);
        }
    }

    MultiLevelGraph(Vector<NodeArrayEntry> node_array_,
                    Vector<EdgeArrayEntry> edge_array_,
                    Vector<EdgeOffset> node_to_edge_offset_,
                    Vector<EdgeWeight> node_weights_,
                    Vector<EdgeDuration> node_durations_,
                    Vector<EdgeDistance> node_distances_,
                    Vector<bool> is_forward_edge_,
                    Vector<bool> is_backward_edge_)
        : SuperT(std::move(node_array_), std::move(edge_array_), std::move(node_to_edge_offset_)),
          node_weights(std::move(node_weights_)), node_durations(std::move(node_durations_)),
          node_distances(std::move(node_distances_)), is_forward_edge(is_forward_edge_),
          is_backward_edge(is_backward_edge_)
    {
    }

    EdgeWeight GetNodeWeight(NodeID node) const { return node_weights[node]; }

    EdgeWeight GetNodeDuration(NodeID node) const { return node_durations[node]; }

    EdgeDistance GetNodeDistance(NodeID node) const { return node_distances[node]; }

    bool IsForwardEdge(EdgeID edge) const { return is_forward_edge[edge]; }

    bool IsBackwardEdge(EdgeID edge) const { return is_backward_edge[edge]; }

    friend void
    serialization::read<EdgeDataT, Ownership>(storage::tar::FileReader &reader,
                                              const std::string &name,
                                              MultiLevelGraph<EdgeDataT, Ownership> &graph);
    friend void
    serialization::write<EdgeDataT, Ownership>(storage::tar::FileWriter &writer,
                                               const std::string &name,
                                               const MultiLevelGraph<EdgeDataT, Ownership> &graph);

  protected:
    Vector<EdgeWeight> node_weights;
    Vector<EdgeDuration> node_durations;
    Vector<EdgeDistance> node_distances;
    Vector<bool> is_forward_edge;
    Vector<bool> is_backward_edge;
};

using MultiLevelEdgeBasedGraph =
    MultiLevelGraph<EdgeBasedGraphEdgeData, storage::Ownership::Container>;
using MultiLevelEdgeBasedGraphView =
    MultiLevelGraph<EdgeBasedGraphEdgeData, storage::Ownership::View>;
}
}

#endif

#ifndef OSRM_PARTITIONER_RENUMBER_HPP
#define OSRM_PARTITIONER_RENUMBER_HPP

#include "extractor/edge_based_node_segment.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/nbg_to_ebg.hpp"
#include "extractor/node_data_container.hpp"

#include "partitioner/bisection_to_partition.hpp"
#include "partitioner/edge_based_graph.hpp"

#include "util/dynamic_graph.hpp"
#include "util/static_graph.hpp"

namespace osrm
{
namespace partitioner
{
std::vector<std::uint32_t> makePermutation(const DynamicEdgeBasedGraph &graph,
                                           const std::vector<Partition> &partitions);

template <typename EdgeDataT>
inline void renumber(util::DynamicGraph<EdgeDataT> &graph,
                     const std::vector<std::uint32_t> &permutation)
{
    // dynamic graph has own specilization
    graph.Renumber(permutation);
}

template <typename EdgeDataT>
inline void renumber(util::StaticGraph<EdgeDataT> &graph,
                     const std::vector<std::uint32_t> &permutation)
{
    // static graph has own specilization
    graph.Renumber(permutation);
}

inline void renumber(extractor::EdgeBasedNodeDataContainer &node_data_container,
                     const std::vector<std::uint32_t> &permutation)
{
    node_data_container.Renumber(permutation);
}

inline void renumber(std::vector<Partition> &partitions,
                     const std::vector<std::uint32_t> &permutation)
{
    for (auto &partition : partitions)
    {
        util::inplacePermutation(partition.begin(), partition.end(), permutation);
    }
}

inline void renumber(util::vector_view<extractor::EdgeBasedNodeSegment> &segments,
                     const std::vector<std::uint32_t> &permutation)
{
    for (auto &segment : segments)
    {
        BOOST_ASSERT(segment.forward_segment_id.enabled);
        segment.forward_segment_id.id = permutation[segment.forward_segment_id.id];
        if (segment.reverse_segment_id.enabled)
            segment.reverse_segment_id.id = permutation[segment.reverse_segment_id.id];
    }
}

inline void renumber(std::vector<extractor::NBGToEBG> &mapping,
                     const std::vector<std::uint32_t> &permutation)
{
    for (extractor::NBGToEBG &m : mapping)
    {
        if (m.backward_ebg_node != SPECIAL_NODEID)
            m.backward_ebg_node = permutation[m.backward_ebg_node];
        if (m.forward_ebg_node != SPECIAL_NODEID)
            m.forward_ebg_node = permutation[m.forward_ebg_node];
    }
}

inline void renumber(std::vector<NodeID> &node_ids, const std::vector<std::uint32_t> &permutation)
{
    for (auto &node_id : node_ids)
    {
        if (node_id != SPECIAL_NODEID)
            node_id = permutation[node_id];
    }
}

inline void renumber(std::vector<extractor::StorageManeuverOverride> &maneuver_overrides,
                     const std::vector<std::uint32_t> &permutation)
{
    for (auto &maneuver_override : maneuver_overrides)
    {
        if (maneuver_override.start_node != SPECIAL_NODEID)
            maneuver_override.start_node = permutation[maneuver_override.start_node];
    }
}

} // namespace partitioner
} // namespace osrm

#endif

#ifndef OSRM_ENGINE_ISOCHRONE_DURATION_GRAPH_HPP
#define OSRM_ENGINE_ISOCHRONE_DURATION_GRAPH_HPP

#include "storage/shared_memory_ownership.hpp"

#include "util/typedefs.hpp"
#include "util/vector_view.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <span>
#include <type_traits>

namespace osrm::engine::isochrone
{

struct DurationGraphArc
{
    NodeID node;
    EdgeDuration duration;

    bool operator==(const DurationGraphArc &) const = default;
};

static_assert(sizeof(DurationGraphArc) == sizeof(NodeID) + sizeof(EdgeDuration));
static_assert(std::is_trivially_copyable_v<DurationGraphArc>);

namespace detail
{
template <storage::Ownership Ownership> struct DurationGraph
{
    util::ViewOrVector<EdgeID, Ownership> forward_offsets;
    util::ViewOrVector<DurationGraphArc, Ownership> forward_arcs;
    util::ViewOrVector<EdgeID, Ownership> reverse_offsets;
    util::ViewOrVector<DurationGraphArc, Ownership> reverse_arcs;

    bool empty() const
    {
        return forward_offsets.empty() && forward_arcs.empty() && reverse_offsets.empty() &&
               reverse_arcs.empty();
    }

    std::size_t getNumberOfNodes() const
    { return forward_offsets.empty() ? 0 : forward_offsets.size() - 1; }

    std::size_t getNumberOfArcs() const { return forward_arcs.size(); }

    std::span<const DurationGraphArc> getForwardRange(const NodeID node) const
    {
        const auto begin = forward_offsets[node];
        const auto end = forward_offsets[node + 1];
        if (begin == end)
            return {};
        return {forward_arcs.data() + begin, static_cast<std::size_t>(end - begin)};
    }

    std::span<const DurationGraphArc> getReverseRange(const NodeID node) const
    {
        const auto begin = reverse_offsets[node];
        const auto end = reverse_offsets[node + 1];
        if (begin == end)
            return {};
        return {reverse_arcs.data() + begin, static_cast<std::size_t>(end - begin)};
    }
};
} // namespace detail

using DurationGraph = detail::DurationGraph<storage::Ownership::Container>;
using DurationGraphView = detail::DurationGraph<storage::Ownership::View>;

template <storage::Ownership Ownership>
bool isValidDurationGraph(const detail::DurationGraph<Ownership> &graph,
                          const std::size_t number_of_nodes)
{
    if (graph.empty())
        return true;

    const auto valid_offsets = [number_of_nodes](const auto &offsets, const auto &arcs)
    {
        if (offsets.size() != number_of_nodes + 1 || offsets.front() != EdgeID{0} ||
            offsets.back() != arcs.size())
        {
            return false;
        }

        for (std::size_t node = 0; node < number_of_nodes; ++node)
        {
            if (offsets[node] > offsets[node + 1] || offsets[node + 1] > arcs.size())
                return false;
        }
        return true;
    };

    if (!valid_offsets(graph.forward_offsets, graph.forward_arcs) ||
        !valid_offsets(graph.reverse_offsets, graph.reverse_arcs) ||
        graph.forward_arcs.size() != graph.reverse_arcs.size())
    {
        return false;
    }

    const auto valid_arcs = [number_of_nodes](const auto &offsets, const auto &arcs)
    {
        for (std::size_t source = 0; source < number_of_nodes; ++source)
        {
            NodeID previous = SPECIAL_NODEID;
            for (auto index = offsets[source]; index < offsets[source + 1]; ++index)
            {
                const auto &arc = arcs[index];
                if (arc.node >= number_of_nodes || arc.duration < EdgeDuration{0} ||
                    arc.duration == INVALID_EDGE_DURATION ||
                    (previous != SPECIAL_NODEID && arc.node <= previous))
                {
                    return false;
                }
                previous = arc.node;
            }
        }
        return true;
    };

    if (!valid_arcs(graph.forward_offsets, graph.forward_arcs) ||
        !valid_arcs(graph.reverse_offsets, graph.reverse_arcs))
    {
        return false;
    }

    for (NodeID source = 0; source < number_of_nodes; ++source)
    {
        for (const auto &forward : graph.getForwardRange(source))
        {
            const auto reverse = graph.getReverseRange(forward.node);
            const auto match = std::lower_bound(reverse.begin(),
                                                reverse.end(),
                                                source,
                                                [](const auto &arc, const NodeID node)
                                                { return arc.node < node; });
            if (match == reverse.end() || match->node != source ||
                match->duration != forward.duration)
            {
                return false;
            }
        }
    }

    return true;
}

} // namespace osrm::engine::isochrone

#endif // OSRM_ENGINE_ISOCHRONE_DURATION_GRAPH_HPP

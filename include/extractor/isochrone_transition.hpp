#ifndef OSRM_EXTRACTOR_ISOCHRONE_TRANSITION_HPP
#define OSRM_EXTRACTOR_ISOCHRONE_TRANSITION_HPP

#include "extractor/edge_based_edge.hpp"

#include "util/typedefs.hpp"

#include <algorithm>
#include <tuple>
#include <type_traits>
#include <vector>

namespace osrm::extractor
{

// A directed transition in the edge-based graph before partitioning coalesces parallel edges.
// `turn_id` identifies the corresponding raw EdgeBasedEdge.
struct IsochroneTransition
{
    NodeID source;
    NodeID target;
    NodeID turn_id;
};
static_assert(sizeof(IsochroneTransition) == 12,
              "IsochroneTransition must remain compact because it is stored per raw transition.");
static_assert(std::is_trivially_copyable<IsochroneTransition>::value,
              "IsochroneTransition must support binary serialization.");

inline bool operator<(const IsochroneTransition &lhs, const IsochroneTransition &rhs)
{
    return std::tie(lhs.source, lhs.target, lhs.turn_id) <
           std::tie(rhs.source, rhs.target, rhs.turn_id);
}

inline bool operator==(const IsochroneTransition &lhs, const IsochroneTransition &rhs)
{
    return std::tie(lhs.source, lhs.target, lhs.turn_id) ==
           std::tie(rhs.source, rhs.target, rhs.turn_id);
}

inline void sortAndUniqueIsochroneTransitions(std::vector<IsochroneTransition> &transitions)
{
    std::sort(transitions.begin(), transitions.end());
    transitions.erase(std::unique(transitions.begin(), transitions.end()), transitions.end());
}

inline bool
isSortedAndUniqueIsochroneTransitions(const std::vector<IsochroneTransition> &transitions)
{
    return std::is_sorted(transitions.begin(), transitions.end()) &&
           std::adjacent_find(transitions.begin(), transitions.end()) == transitions.end();
}

inline std::vector<IsochroneTransition>
makeIsochroneTransitions(const std::vector<EdgeBasedEdge> &edge_based_edges)
{
    std::vector<IsochroneTransition> transitions;
    transitions.reserve(edge_based_edges.size() * 2);

    for (const auto &edge : edge_based_edges)
    {
        if (edge.data.forward)
            transitions.push_back({edge.source, edge.target, edge.data.turn_id});
        if (edge.data.backward)
            transitions.push_back({edge.target, edge.source, edge.data.turn_id});
    }

    sortAndUniqueIsochroneTransitions(transitions);
    return transitions;
}

inline bool isValidIsochroneTransitions(const std::vector<IsochroneTransition> &transitions,
                                        const NodeID number_of_edge_based_nodes)
{
    return std::all_of(transitions.begin(),
                       transitions.end(),
                       [number_of_edge_based_nodes](const auto &transition)
                       {
                           return transition.source < number_of_edge_based_nodes &&
                                  transition.target < number_of_edge_based_nodes &&
                                  transition.turn_id != SPECIAL_NODEID;
                       });
}

} // namespace osrm::extractor

#endif // OSRM_EXTRACTOR_ISOCHRONE_TRANSITION_HPP

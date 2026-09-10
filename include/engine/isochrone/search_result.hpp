#ifndef OSRM_ENGINE_ISOCHRONE_SEARCH_RESULT_HPP
#define OSRM_ENGINE_ISOCHRONE_SEARCH_RESULT_HPP

#include "engine/phantom_node.hpp"

#include "util/typedefs.hpp"

#include <cstdint>
#include <vector>

namespace osrm::engine::isochrone
{

enum class SearchStatus : std::uint8_t
{
    Complete,
    SearchRecordLimitReached,
    ArithmeticOverflow
};

enum class NodeProvenance : std::uint8_t
{
    Network,
    SeedReentry
};

enum class PhantomTraversalDirection : std::uint8_t
{
    Forward,
    Reverse
};

enum class PhantomTraversalKind : std::uint8_t
{
    OutboundInitial,
    InboundTerminal
};

struct PhantomPartialTraversal
{
    NodeID node;
    PhantomNode phantom;
    PhantomTraversalDirection direction;
    PhantomTraversalKind kind;

    // The virtual search label at the first coordinate of `node`'s directed
    // geometry.  A materializer adds the geometry prefix for an outbound
    // partial and subtracts it for an inbound partial.
    EdgeDuration seed_duration;

    // False when the approach reaches the requested cutoff before the
    // network. The phantom is retained so callers can still materialize the
    // reachable input-to-phantom approach, but must not emit road geometry.
    bool reaches_network = false;
};

struct SearchResult
{
    struct Node
    {
        NodeID node;

        // The label at the first coordinate in this edge-based node's legal
        // GeometryID direction. An outbound geometry point has
        // `duration + prefix_duration`; an inbound point has
        // `duration - prefix_duration`.
        EdgeDuration duration;
        NodeProvenance provenance = NodeProvenance::Network;
    };

    struct InboundFrontier
    {
        // Entry label at the far side of the duration-graph arc that first
        // crosses the cutoff.
        Node entry;
    };

    // Geometry labels whose reachable portion intersects the requested cutoff.  An inbound
    // self-loop re-entry label at the first coordinate can exceed the cutoff while a suffix near
    // the last coordinate remains reachable. Initial phantom labels are deliberately absent:
    // `phantom_partials` describes their clipped geometry instead.
    std::vector<Node> nodes;

    // First reverse-search entries beyond the cutoff.  Their reachable
    // geometry suffix is still needed to materialize an exact inbound contour.
    std::vector<InboundFrontier> inbound_frontiers;

    // The direct, clipped traversal from a source phantom or into a target
    // phantom.  Re-entry after a nontrivial loop is represented by a Node with
    // SeedReentry provenance, never by this virtual seed.
    std::vector<PhantomPartialTraversal> phantom_partials;

    SearchStatus status = SearchStatus::Complete;

    bool isComplete() const { return status == SearchStatus::Complete; }
};

} // namespace osrm::engine::isochrone

#endif // OSRM_ENGINE_ISOCHRONE_SEARCH_RESULT_HPP

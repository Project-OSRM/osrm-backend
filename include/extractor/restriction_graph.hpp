#ifndef OSRM_EXTRACTOR_RESTRICTION_GRAPH_HPP_
#define OSRM_EXTRACTOR_RESTRICTION_GRAPH_HPP_

#include <boost/assert.hpp>

#include "util/node_based_graph.hpp"
#include "util/std_hash.hpp"
#include "util/typedefs.hpp"

#include <unordered_map>

namespace osrm::extractor
{

struct TurnRestriction;

namespace restriction_graph_details
{
struct transferBuilder;
struct pathBuilder;
} // namespace restriction_graph_details

struct RestrictionEdge
{
    NodeID node_based_to;
    RestrictionID target;
    bool is_transfer;
};

struct RestrictionNode
{
    size_t restrictions_begin_idx;
    size_t num_restrictions;
    size_t edges_begin_idx;
    size_t num_edges;
};

/**
 * The restriction graph is used to represent all possible restrictions within the routing graph.
 * The graph uses an edge-based node representation. Each node represents a compressed
 * node-based edge along a restriction path.
 *
 * Given a list of turn restrictions, the graph is created in multiple steps.
 *
 * INPUT
 * a -> b -> d: no_e
 * a -> b -> c: only_d
 * b -> c -> d: only_f
 * b -> c: no_g
 * e -> b -> d: no_g
 *
 * Step 1: create a disjoint union of prefix trees for all restriction paths.
 * The restriction instructions are added to the final node in its path.
 *
 * STEP 1
 * (a,b) -> (b,d,[no_e])
 *      \->(b,c,[only_d])
 *
 * (b,c,[no_g]) -> (c,d,[only_f])
 * (e,b) -> (b,d,[no_g])
 *
 * Step 2: add transfers between restriction paths that overlap.
 * We do this by traversing each restriction path, tracking where the suffix of our current path
 * matches the prefix of any other. If it does, there's opportunity to transfer to the suffix
 * restriction path *if* the transfer would not be restricted *and* that edge does not take us
 * further on our current path. Nested restrictions are also added from any of the suffix paths.
 *
 * STEP 2
 * (a,b) -> (b,d,[no_e])
 *      \->(b,c,[only_d,no_g])
 *                \
 * (b,c,[no_g]) -> \-> (c,d,[only_f])
 *
 * (e,b) -> (b,d,[no_g])
 * If a transfer applies to multiple suffix paths, we only add the edge to the largest suffix path.
 * This ensures we correctly track all overlapping paths.
 *
 *
 * Step 3: The nodes are split into
 * start nodes - compressed edges that are entry points into a restriction path.
 * via nodes - compressed edges that are intermediate steps along a restriction path.
 * Start nodes and via nodes are indexed by the compressed node-based edge for easy retrieval
 *
 * STEP 3
 * Start Node Index
 * (a,b) => (a,b)
 * (b,c) => (b,c,[no_g])
 * (e,b) => (e,b)
 *
 * Via Nodes Index
 * (b,c) => (b,c,[only_d,no_g])
 * (b,d) => (b,d,[no_e]) , (b,d,[no_g])
 * (c,d) => (c,d,[only_f])
 *
 * Duplicate Nodes:
 * There is a 1-1 mapping between restriction graph via nodes and edge-based-graph duplicate nodes.
 * This relationship is used in the creation of restrictions in the routing edge-based graph.
 * */
struct RestrictionGraph
{
    friend restriction_graph_details::pathBuilder;
    friend restriction_graph_details::transferBuilder;
    friend RestrictionGraph constructRestrictionGraph(const std::vector<TurnRestriction> &);

    using EdgeRange = boost::iterator_range<std::vector<RestrictionEdge>::const_iterator>;
    using RestrictionRange =
        boost::iterator_range<std::vector<const TurnRestriction *>::const_iterator>;
    using EdgeKey = std::pair<NodeID, NodeID>;

    // Helper functions for iterating over node restrictions and edges
    EdgeRange GetEdges(RestrictionID id) const;
    RestrictionRange GetRestrictions(RestrictionID id) const;

    // A compressed node-based edge can only have one start node in the restriction graph.
    std::unordered_map<EdgeKey, RestrictionID> start_edge_to_node{};
    // A compressed node-based edge can have multiple via nodes in the restriction graph
    // (as the compressed edge can appear in paths with different prefixes).
    std::unordered_multimap<EdgeKey, RestrictionID> via_edge_to_node{};
    std::vector<RestrictionNode> nodes;
    // TODO: Investigate reusing DynamicGraph. Currently it requires specific attributes
    // (e.g. reversed, weight) that would not make sense for restrictions.
    std::vector<RestrictionEdge> edges;
    std::vector<const TurnRestriction *> restrictions;
    size_t num_via_nodes{};

  private:
    RestrictionGraph() = default;
};

RestrictionGraph constructRestrictionGraph(const std::vector<TurnRestriction> &turn_restrictions);

} // namespace osrm::extractor

#endif // OSRM_EXTRACTOR_RESTRICTION_GRAPH_HPP_

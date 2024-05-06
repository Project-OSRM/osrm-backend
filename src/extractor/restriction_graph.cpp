#include "extractor/restriction_graph.hpp"
#include "extractor/restriction.hpp"
#include "extractor/turn_path.hpp"
#include "util/node_based_graph.hpp"
#include "util/timing_util.hpp"
#include <util/for_each_pair.hpp>

#include <boost/range/algorithm/copy.hpp>

namespace osrm::extractor
{

namespace restriction_graph_details
{

void insertEdge(RestrictionGraph &rg, const RestrictionID id, const RestrictionEdge &edge)
{
    const auto range = rg.GetEdges(id);
    auto &node = rg.nodes[id];
    if (node.edges_begin_idx + range.size() != rg.edges.size())
    {
        // Most nodes will only have one edge, so this copy will be infrequent
        node.edges_begin_idx = rg.edges.size();
        std::copy(range.begin(), range.end(), std::back_inserter(rg.edges));
    }
    rg.edges.push_back(edge);
    node.num_edges += 1;
}

void insertRestriction(RestrictionGraph &rg,
                       const RestrictionID id,
                       const TurnRestriction *restriction)
{
    const auto range = rg.GetRestrictions(id);
    auto &node = rg.nodes[id];
    if (node.restrictions_begin_idx + range.size() != rg.restrictions.size())
    {
        // Most nodes will only have zero or one restriction, so this copy will be infrequent
        node.restrictions_begin_idx = rg.restrictions.size();
        std::copy(range.begin(), range.end(), std::back_inserter(rg.restrictions));
    }
    rg.restrictions.push_back(restriction);
    node.num_restrictions += 1;
}

RestrictionID getOrInsertStartNode(RestrictionGraph &rg, NodeID from, NodeID to)
{
    auto start_edge = std::make_pair(from, to);
    auto start_node_idx_itr = rg.start_edge_to_node.find(start_edge);
    if (start_node_idx_itr == rg.start_edge_to_node.end())
    {
        // First time we have seen a restriction start from this edge.
        auto start_node_idx = rg.nodes.size();
        rg.nodes.push_back(RestrictionNode{rg.restrictions.size(), 0, rg.edges.size(), 0});
        start_node_idx_itr = rg.start_edge_to_node.insert({start_edge, start_node_idx}).first;
    }
    return start_node_idx_itr->second;
}

RestrictionID insertViaNode(RestrictionGraph &rg, NodeID from, NodeID to)
{
    auto new_via_node_idx = rg.nodes.size();
    rg.nodes.push_back(RestrictionNode{rg.restrictions.size(), 0, rg.edges.size(), 0});
    rg.via_edge_to_node.insert({{from, to}, new_via_node_idx});
    return new_via_node_idx;
}

// pathBuilder builds the prefix tree structure that is used to first insert turn restrictions
// into the graph.
struct pathBuilder
{
    RestrictionGraph &rg;
    RestrictionID cur_node;

    pathBuilder(RestrictionGraph &rg_) : rg(rg_) { cur_node = SPECIAL_RESTRICTIONID; }

    static std::string name() { return "path builder"; };

    void start(NodeID from, NodeID to) { cur_node = getOrInsertStartNode(rg, from, to); }

    void next(NodeID from, NodeID to)
    {
        const auto &edge_range = rg.GetEdges(cur_node);
        auto edge_to_itr = std::find_if(edge_range.begin(),
                                        edge_range.end(),
                                        [&](const auto &edge) { return edge.node_based_to == to; });
        if (edge_to_itr != edge_range.end())
        {
            cur_node = edge_to_itr->target;
            return;
        }

        // This is a new restriction path we have not seen before.
        // Add a new via node and edge to the tree.
        auto new_via_node_idx = insertViaNode(rg, from, to);
        insertEdge(rg, cur_node, RestrictionEdge{to, new_via_node_idx, false});

        cur_node = new_via_node_idx;
    }

    void end(const TurnRestriction &restriction) { insertRestriction(rg, cur_node, &restriction); }
};

// transferBuilder adds the transfer edges between overlapping restriction paths. It does this
// by tracking paths in the graph that can be equal to a suffix of a restriction path, and
// attempting to connect the them with a new edge.
struct transferBuilder
{
    RestrictionGraph &rg;
    std::vector<RestrictionID> suffix_nodes;
    RestrictionID cur_node;

    transferBuilder(RestrictionGraph &rg_) : rg(rg_) { cur_node = SPECIAL_RESTRICTIONID; }

    static std::string name() { return "transfer builder"; };

    void start(NodeID from, NodeID to) { cur_node = getOrInsertStartNode(rg, from, to); }

    void next_suffixes(NodeID from, NodeID to)
    {
        // Update the suffix paths to include those that also have this edge
        auto new_suffix_it = suffix_nodes.begin();
        for (const auto &suffix_node : suffix_nodes)
        {
            const auto &edges = rg.GetEdges(suffix_node);
            const auto edge_it = std::find_if(
                edges.begin(),
                edges.end(),
                [&to](const auto &edge) { return edge.node_based_to == to && !edge.is_transfer; });
            if (edge_it != edges.end())
            {
                *(new_suffix_it++) = edge_it->target;
            }
        }
        suffix_nodes.erase(new_suffix_it, suffix_nodes.end());

        // Add a start node if it is a trivial suffix
        auto start_way_it = rg.start_edge_to_node.find({from, to});
        if (start_way_it != rg.start_edge_to_node.end())
        {
            suffix_nodes.push_back({start_way_it->second});
        }
    }

    // For each edge on path, if there is an edge in the suffix path not on the current
    // path, add as a transfer.
    void add_suffix_transfer(const RestrictionID &suffix_node)
    {
        for (const auto &suffix_edge : rg.GetEdges(suffix_node))
        {
            if (suffix_edge.is_transfer)
                continue;

            // Check there are no unconditional restrictions at the current node that would prevent
            // the transfer
            const auto &restrictions = rg.GetRestrictions(cur_node);
            const auto is_restricted =
                std::any_of(restrictions.begin(),
                            restrictions.end(),
                            [&](const auto &restriction)
                            {
                                return restriction->IsTurnRestricted(suffix_edge.node_based_to) &&
                                       restriction->IsUnconditional();
                            });
            if (is_restricted)
                continue;

            const auto &edges = rg.GetEdges(cur_node);
            // Check that the suffix edge is not a next edge along the current path.
            const auto can_transfer = std::none_of(
                edges.begin(),
                edges.end(),
                [&](auto &edge) { return edge.node_based_to == suffix_edge.node_based_to; });
            if (can_transfer)
            {
                insertEdge(rg,
                           cur_node,
                           RestrictionEdge{suffix_edge.node_based_to, suffix_edge.target, true});
            }
        }
    };

    void next(NodeID from, NodeID to)
    {
        const auto &edges = rg.GetEdges(cur_node);
        auto next_edge_itr = std::find_if(
            edges.begin(), edges.end(), [&](const auto edge) { return edge.node_based_to == to; });

        // We know this edge exists
        cur_node = next_edge_itr->target;

        this->next_suffixes(from, to);

        std::for_each(suffix_nodes.begin(),
                      suffix_nodes.end(),
                      [&](const auto &suffix_node) { this->add_suffix_transfer(suffix_node); });

        for (const auto &suffix_node : suffix_nodes)
        {
            // Also add any restrictions from suffix paths to the current node in the
            // restriction graph.
            for (const auto &restriction : rg.GetRestrictions(suffix_node))
            {
                insertRestriction(rg, cur_node, restriction);
            }
        }
    }

    void end(const TurnRestriction & /*unused*/) {}
};

template <typename builder_type>
void buildGraph(RestrictionGraph &rg, const std::vector<TurnRestriction> &restrictions)
{
    const auto run_builder = [&](const auto &restriction)
    {
        builder_type builder(rg);

        builder.start(restriction.turn_path.From(), restriction.turn_path.FirstVia());
        if (restriction.turn_path.Type() == TurnPathType::VIA_WAY_TURN_PATH)
        {
            const auto &via_way_path = restriction.turn_path.AsViaWayPath();
            util::for_each_pair(via_way_path.via,
                                [&](NodeID from, NodeID to) { builder.next(from, to); });
        }
        builder.end(restriction);
    };

    std::for_each(restrictions.begin(), restrictions.end(), run_builder);
}
} // namespace restriction_graph_details

RestrictionGraph constructRestrictionGraph(const std::vector<TurnRestriction> &turn_restrictions)
{
    util::UnbufferedLog log;
    log << "Constructing restriction graph on " << turn_restrictions.size() << " restrictions...";
    TIMER_START(construct_restriction_graph);

    namespace rgd = restriction_graph_details;
    RestrictionGraph rg;
    rgd::buildGraph<rgd::pathBuilder>(rg, turn_restrictions);
    rgd::buildGraph<rgd::transferBuilder>(rg, turn_restrictions);

    // Reorder nodes so that via nodes are at the front. This makes it easier to represent the
    // bijection between restriction graph via nodes and edge-based graph duplicate nodes.
    std::vector<bool> is_via_node(rg.nodes.size(), false);
    for (const auto &entry : rg.via_edge_to_node)
    {
        is_via_node[entry.second] = true;
    }
    std::vector<RestrictionID> ordering(rg.nodes.size());
    std::iota(ordering.begin(), ordering.end(), 0);
    std::partition(ordering.begin(), ordering.end(), [&](const auto i) { return is_via_node[i]; });
    // Start renumbering
    const auto permutation = util::orderingToPermutation(ordering);
    util::inplacePermutation(rg.nodes.begin(), rg.nodes.end(), permutation);
    std::for_each(rg.edges.begin(),
                  rg.edges.end(),
                  [&](auto &edge) { edge.target = permutation[edge.target]; });
    rg.num_via_nodes = std::count(is_via_node.begin(), is_via_node.end(), true);
    for (auto &entry : rg.via_edge_to_node)
    {
        entry.second = permutation[entry.second];
    }
    for (auto &entry : rg.start_edge_to_node)
    {
        entry.second = permutation[entry.second];
    }

    TIMER_STOP(construct_restriction_graph);
    log << "ok, after " << TIMER_SEC(construct_restriction_graph) << "s";

    return rg;
}

RestrictionGraph::RestrictionRange RestrictionGraph::GetRestrictions(RestrictionID id) const
{
    const auto &node = nodes[id];
    return boost::make_iterator_range(restrictions.begin() + node.restrictions_begin_idx,
                                      restrictions.begin() + node.restrictions_begin_idx +
                                          node.num_restrictions);
}

RestrictionGraph::EdgeRange RestrictionGraph::GetEdges(RestrictionID id) const
{
    const auto &node = nodes[id];
    return boost::make_iterator_range(edges.begin() + node.edges_begin_idx,
                                      edges.begin() + node.edges_begin_idx + node.num_edges);
}

} // namespace osrm::extractor

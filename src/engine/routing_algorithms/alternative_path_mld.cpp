#include "engine/routing_algorithms/alternative_path.hpp"
#include "engine/routing_algorithms/routing_base_mld.hpp"

#include "util/static_assert.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <iterator>
#include <memory>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/iterator/function_output_iterator.hpp>

namespace osrm::engine::routing_algorithms
{

// Unqualified calls below are from the mld namespace.
// This alternative implementation works only for mld.
using namespace mld;

using Heap = SearchEngineData<Algorithm>::QueryHeap;
using Partition = partitioner::MultiLevelPartitionView;
using Facade = DataFacade<Algorithm>;

// Implementation details
namespace
{

struct Parameters
{
    // Alternative paths candidate via nodes are taken from overlapping search spaces.
    // Overlapping by a third guarantees us taking candidate nodes "from the middle".
    double kSearchSpaceOverlapFactor = 1.33;
    // Unpack n-times more candidate paths to run high-quality checks on.
    // Unpacking paths yields higher chance to find good alternatives but is also expensive.
    unsigned kAlternativesToUnpackFactor = 2;
    // Alternative paths length requirement (stretch).
    // At most 25% longer then the shortest path.
    double kAtMostLongerBy = 0.25;
    // Alternative paths similarity requirement (sharing).
    // At least 25% different than the shortest path.
    double kAtMostSameBy = 0.75;
    // Alternative paths are still reasonable around the via node candidate (local optimality).
    // At least optimal around 10% sub-paths around the via node candidate.
    double kAtLeastOptimalAroundViaBy = 0.1;
    // Alternative paths similarity requirement (sharing) based on cells.
    // At least 5% different than the shortest path.
    double kCellsAtMostSameBy = 0.95;
};

// Represents a via middle node where forward (from s) and backward (from t)
// search spaces overlap and the weight a path (made up of s,via and via,t) has.
struct WeightedViaNode
{
    NodeID node;
    EdgeWeight weight;
};

// Represents a complete packed path (made up of s,via and via,t)
// its total weight and the via node used to construct the path.
struct WeightedViaNodePackedPath
{
    WeightedViaNode via;
    PackedPath path;
};

// Represents a high-detail unpacked path (s, .., via, .., t)
// its total weight and the via node used to construct the path.
struct WeightedViaNodeUnpackedPath
{
    double sharing;
    WeightedViaNode via;
    UnpackedNodes nodes;
    UnpackedEdges edges;
};

// Scale the maximum allowed weight increase based on its magnitude:
//  - Shortest path 10 minutes, alternative 13 minutes => Factor of 0.30 ok
//  - Shortest path 10 hours, alternative 13 hours     => Factor of 0.30 unreasonable
double getLongerByFactorBasedOnDuration(const EdgeDuration duration)
{
    BOOST_ASSERT(duration != INVALID_EDGE_DURATION);

    // We only have generic weights here and no durations without unpacking.
    // We also have restricted way penalties which are huge and will screw scaling here.
    //
    // Users can pass us generic weights not based on durations; we can't do anything about
    // it here other than either generating too many or no alternatives in these cases.
    //
    // We scale the weights with a step function based on some rough guestimates, so that
    // they match tens of minutes, in the low hours, tens of hours, etc.

    // Computed using a least-squares curve fitiing on the following values:
    //
    // def func(xs, a, b, c, d):
    //       return a + b/(xs-d) + c/(xs-d)**3
    //
    // xs = np.array([5 * 60, 10 * 60, 30 * 60, 60 * 60, 3 * 60 * 60, 10 * 60 * 60])
    // ys = np.array([1.0,    0.75,    0.5,     0.4,     0.3,          0.2])
    //
    // xs_interp = np.arange(5*60, 10*60*60, 5*60)
    // ys_interp = np.interp(xs_interp, xs, ys)
    //
    // params, _ = scipy.optimize.curve_fit(func, xs_interp, ys_interp)
    //
    // The hyperbolic shape was chosen because it interpolated well between
    // the given datapoints and drops off for large durations.
    const constexpr auto a = 1.91578463e-01;
    const constexpr auto b = 1.35118442e+03;
    const constexpr auto c = 2.45437877e+09;
    const constexpr auto d = -2.07944571e+03;

    if (duration < EdgeDuration{5 * 60})
    {
        return 1.0;
    }
    else if (duration > EdgeDuration{10 * 60 * 60})
    {
        return 0.20;
    }

    // Bigger than 10 minutes but smaller than 10 hours
    BOOST_ASSERT(duration >= EdgeDuration{5 * 60} && duration <= EdgeDuration{10 * 60 * 60});

    return a + b / (from_alias<double>(duration) - d) +
           c / std::pow(from_alias<double>(duration) - d, 3);
}

Parameters parametersFromRequest(const PhantomEndpointCandidates &endpoint_candidates)
{
    Parameters parameters;

    const auto distance = util::coordinate_calculation::greatCircleDistance(
        candidatesSnappedLocation(endpoint_candidates.source_phantoms),
        candidatesSnappedLocation(endpoint_candidates.target_phantoms));

    // 10km
    if (distance < 10000.)
    {
        parameters.kAlternativesToUnpackFactor = 10.0;
        parameters.kCellsAtMostSameBy = 1.0;
        parameters.kAtLeastOptimalAroundViaBy = 0.2;
        parameters.kAtMostSameBy = 0.50;
    }
    // 20km
    else if (distance < 20000.)
    {
        parameters.kAlternativesToUnpackFactor = 8.0;
        parameters.kCellsAtMostSameBy = 1.0;
        parameters.kAtLeastOptimalAroundViaBy = 0.2;
        parameters.kAtMostSameBy = 0.60;
    }
    // 50km
    else if (distance < 50000.)
    {
        parameters.kAlternativesToUnpackFactor = 6.0;
        parameters.kCellsAtMostSameBy = 0.95;
        parameters.kAtMostSameBy = 0.65;
    }
    // 100km
    else if (distance < 100000.)
    {
        parameters.kAlternativesToUnpackFactor = 4.0;
        parameters.kCellsAtMostSameBy = 0.95;
        parameters.kAtMostSameBy = 0.70;
    }

    return parameters;
}

// Filters candidates which are on not unique.
// Returns an iterator to the uniquified range's new end.
// Note: mutates the range in-place invalidating iterators.
template <typename RandIt> RandIt filterViaCandidatesByUniqueNodeIds(RandIt first, RandIt last)
{
    util::static_assert_iter_category<RandIt, std::random_access_iterator_tag>();
    util::static_assert_iter_value<RandIt, WeightedViaNode>();

    std::sort(first, last, [](auto lhs, auto rhs) { return lhs.node < rhs.node; });
    return std::unique(first, last, [](auto lhs, auto rhs) { return lhs.node == rhs.node; });
}

// Filters candidates which are on un-important roads.
// Returns an iterator to the filtered range's new end.
template <typename RandIt>
RandIt filterViaCandidatesByRoadImportance(RandIt first, RandIt last, const Facade &facade)
{
    util::static_assert_iter_category<RandIt, std::random_access_iterator_tag>();
    util::static_assert_iter_value<RandIt, WeightedViaNode>();

    // Todo: the idea here is to filter out alternatives where the via candidate is not on a
    // high-priority road. We should experiment if this is really needed or if the boundary
    // nodes the mld search space gives us already provides us with reasonable via candidates.
    //
    // Implementation: we already have `RoadClassification` from guidance. We need to serialize
    // it to disk and then in the facades read it in again providing `IsImportantRoad(NodeID)`.
    // Note: serialize out bit vector keyed by node id with 0/1 <=> unimportant/important.
    (void)first;
    (void)last;
    (void)facade;

    return last;
}

// Filters candidates with much higher weight than the primary route. Mutates range in-place.
// Returns an iterator to the filtered range's new end.
template <typename RandIt>
RandIt filterViaCandidatesByStretch(RandIt first,
                                    RandIt last,
                                    const EdgeWeight weight,
                                    const Parameters &parameters)
{
    util::static_assert_iter_category<RandIt, std::random_access_iterator_tag>();
    util::static_assert_iter_value<RandIt, WeightedViaNode>();

    // Assumes weight roughly corresponds to duration-ish. If this is not the case e.g.
    // because users are setting weight to be distance in the profiles, then we might
    // either generate more candidates than we have to or not enough. But is okay.
    const auto stretch_weight_limit =
        (1. + parameters.kAtMostLongerBy) * from_alias<double>(weight);

    const auto over_weight_limit = [=](const auto via)
    { return from_alias<double>(via.weight) > stretch_weight_limit; };

    return std::remove_if(first, last, over_weight_limit);
}

// Filters candidates that are on the path. Mutates range in-place.
// Returns an iterator to the filtered range's new end.
template <typename RandIt>
RandIt
filterViaCandidatesByViaNotOnPath(const WeightedViaNodePackedPath &path, RandIt first, RandIt last)
{
    util::static_assert_iter_category<RandIt, std::random_access_iterator_tag>();
    util::static_assert_iter_value<RandIt, WeightedViaNode>();

    if (path.path.empty())
        return last;

    std::unordered_set<NodeID> nodes;
    nodes.reserve(path.path.size() + 1);

    nodes.insert(std::get<0>(path.path.front()));
    for (const auto &edge : path.path)
        nodes.insert(std::get<1>(edge));

    const auto via_on_path = [&](const auto via) { return nodes.contains(via.node); };

    return std::remove_if(first, last, via_on_path);
}

// Filters packed paths with similar cells between each other. Mutates range in-place.
// Returns an iterator to the filtered range's new end.
template <typename RandIt>
RandIt filterPackedPathsByCellSharing(RandIt first,
                                      RandIt last,
                                      const Partition &partition,
                                      const Parameters &parameters)
{
    // In this case we don't need to calculate sharing, because it would not filter anything
    if (parameters.kCellsAtMostSameBy >= 1.0)
        return last;

    util::static_assert_iter_category<RandIt, std::random_access_iterator_tag>();
    util::static_assert_iter_value<RandIt, WeightedViaNodePackedPath>();

    // Todo: we could scale cell sharing with edge weights. Also basing sharing on
    // cells could be problematic e.g. think of parallel ways in grid cities.

    const auto size = static_cast<std::size_t>(last - first);

    if (size < 2)
        return last;

    const auto number_of_levels = partition.GetNumberOfLevels();
    (void)number_of_levels;
    BOOST_ASSERT(number_of_levels >= 1);

    // Todo: sharing could be a linear combination based on level and sharing on each level.
    // Experimental evaluation shows using the lowest level works surprisingly well already.
    const auto level = 1;
    const auto get_cell = [&](auto node) { return partition.GetCell(level, node); };

    const auto shortest_path = *first;

    if (shortest_path.path.empty())
        return last;

    std::unordered_set<CellID> cells;
    cells.reserve(size * (shortest_path.path.size() + 1) * (1 + parameters.kAtMostLongerBy));

    cells.insert(get_cell(std::get<0>(shortest_path.path.front())));
    for (const auto &edge : shortest_path.path)
        cells.insert(get_cell(std::get<1>(edge)));

    const auto over_sharing_limit = [&](const auto &packed)
    {
        if (packed.path.empty())
        { // don't remove routes with single-node (empty) path
            return false;
        }

        const auto not_seen = [&](const PackedEdge edge)
        {
            const auto source_cell = get_cell(std::get<0>(edge));
            const auto target_cell = get_cell(std::get<1>(edge));
            return !cells.contains(source_cell) && !cells.contains(target_cell);
        };

        const auto different = std::count_if(begin(packed.path), end(packed.path), not_seen);

        const auto difference = different / static_cast<double>(packed.path.size() + 1);
        BOOST_ASSERT(difference >= 0.);
        BOOST_ASSERT(difference <= 1.);

        const auto sharing = 1. - difference;

        if (sharing > parameters.kCellsAtMostSameBy)
        {
            return true;
        }
        else
        {
            cells.insert(get_cell(std::get<0>(packed.path.front())));
            for (const auto &edge : packed.path)
                cells.insert(get_cell(std::get<1>(edge)));

            return false;
        }
    };

    return std::remove_if(first + 1, last, over_sharing_limit);
}

// Filters packed paths based on local optimality. Mutates range in-place.
// Returns an iterator to the filtered range's new end.
template <typename RandIt>
RandIt filterPackedPathsByLocalOptimality(const WeightedViaNodePackedPath &path,
                                          const Heap &forward_heap,
                                          const Heap &reverse_heap,
                                          RandIt first,
                                          RandIt last,
                                          const Parameters &parameters)
{
    util::static_assert_iter_category<RandIt, std::random_access_iterator_tag>();
    util::static_assert_iter_value<RandIt, WeightedViaNodePackedPath>();

    if (path.path.empty())
        return last;

    // Check sub-path optimality on alternative path crossing the via node candidate.
    //
    //   s - - - v - - - t  our packed path made up of (from, to) edges
    //        |--|--|       sub-paths "left" and "right" of v based on threshold
    //        f  v  l       nodes in [v,f] and in [v, l] have to match predecessor in heaps
    //
    // Todo: this approach is efficient but works on packed paths only. Do we need to do a
    // thorough check on the unpacked paths instead? Or do we even need to introduce two
    // new thread-local heaps for the mld SearchEngineData and do proper s-t routing here?

    BOOST_ASSERT(path.via.weight != INVALID_EDGE_WEIGHT);

    // node == parent_in_main_heap(parent_in_side_heap(v)) -> plateaux at `node`
    const auto has_plateaux_at_node = [&](const NodeID node, const Heap &fst, const Heap &snd)
    {
        BOOST_ASSERT(fst.WasInserted(node));
        auto const parent = fst.GetData(node).parent;
        return snd.WasInserted(parent) && snd.GetData(parent).parent == node;
    };

    // A plateaux is defined as a segment in which the search tree from s and the search
    // tree from t overlap. An edge is part of such a plateaux around `v` if:
    // v == parent_in_reverse_search(parent_in_forward_search(v)).
    // Here we calculate the last node on the plateaux in either direction.
    const auto plateaux_end = [&](NodeID node, const Heap &fst, const Heap &snd)
    {
        BOOST_ASSERT(node != SPECIAL_NODEID);
        BOOST_ASSERT(fst.WasInserted(node));
        BOOST_ASSERT(snd.WasInserted(node));

        // Check plateaux edges towards the target. Terminates at the source / target
        // at the latest, since parent(target)==target for the reverse heap and
        // parent(target) != target in the forward heap (and vice versa).
        while (node != fst.GetData(node).parent && has_plateaux_at_node(node, fst, snd))
            node = fst.GetData(node).parent;

        return node;
    };

    const auto is_not_locally_optimal = [&](const auto &packed)
    {
        BOOST_ASSERT(packed.via.node != path.via.node);
        BOOST_ASSERT(packed.via.weight != INVALID_EDGE_WEIGHT);
        BOOST_ASSERT(packed.via.node != SPECIAL_NODEID);

        if (packed.path.empty())
        { // the edge case when packed.via.node is both source and target node
            return false;
        }

        const NodeID via = packed.via.node;

        // Plateaux iff via == parent_in_reverse_search(parent_in_forward_search(via))
        //
        // Forward search starts from s, reverse search starts from t:
        //  - parent_in_forward_search(via) = a
        //  - parent_in_reverse_search(a) = b  != via and therefore not local optimal
        //
        //        via
        //      .'   '.
        // s - a - - - b - t
        //
        // Care needs to be taken for the edge case where the via node is on the border
        // of the search spaces and therefore parent pointers may not be valid in heaps.
        // In these cases we know we can't have local optimality around the via already.

        const auto first_on_plateaux = plateaux_end(via, forward_heap, reverse_heap);
        const auto last_on_plateaux = plateaux_end(via, reverse_heap, forward_heap);

        //        fop - - via - - lop
        //      .'                    '.
        // s - a - - - - - - - - - - - - b - t
        //
        // Lenth of plateaux is given by the weight vetween fop and via as well as via and lop.
        const auto plateaux_length =
            forward_heap.GetKey(last_on_plateaux) - forward_heap.GetKey(first_on_plateaux);

        // Find a/b as the first location where packed and path differ
        const auto a = std::get<0>(*std::mismatch(packed.path.begin(), //
                                                  packed.path.end(),
                                                  path.path.begin(),
                                                  path.path.end())
                                        .first);
        const auto b = std::get<1>(*std::mismatch(packed.path.rbegin(), //
                                                  packed.path.rend(),
                                                  path.path.rbegin(),
                                                  path.path.rend())
                                        .first);

        BOOST_ASSERT(forward_heap.WasInserted(a));
        BOOST_ASSERT(reverse_heap.WasInserted(b));
        const auto detour_length = forward_heap.GetKey(via) - forward_heap.GetKey(a) +
                                   reverse_heap.GetKey(via) - reverse_heap.GetKey(b);

        return from_alias<double>(plateaux_length) <
               parameters.kAtLeastOptimalAroundViaBy * from_alias<double>(detour_length);
    };

    return std::remove_if(first, last, is_not_locally_optimal);
}

// Filters unpacked paths compared to all other paths. Mutates range in-place.
// Returns an iterator to the filtered range's new end.
template <typename RandIt>
RandIt filterUnpackedPathsBySharing(RandIt first,
                                    RandIt last,
                                    const Facade &facade,
                                    const Parameters &parameters)
{
    util::static_assert_iter_category<RandIt, std::random_access_iterator_tag>();
    util::static_assert_iter_value<RandIt, WeightedViaNodeUnpackedPath>();

    const auto size = static_cast<std::size_t>(last - first);

    if (size < 2)
        return last;

    const auto &shortest_path = *first;

    if (shortest_path.edges.empty())
        return last;

    std::unordered_set<NodeID> nodes;
    nodes.reserve(size * shortest_path.nodes.size() * (1.25));

    nodes.insert(begin(shortest_path.nodes), end(shortest_path.nodes));

    const auto over_sharing_limit = [&](auto &unpacked)
    {
        if (unpacked.edges.empty())
        { // don't remove routes with single-node (empty) path
            return false;
        }

        EdgeDuration total_duration = {0};
        const auto add_if_seen = [&](const EdgeDuration duration, const NodeID node)
        {
            auto node_duration = facade.GetNodeDuration(node);
            total_duration += node_duration;
            if (nodes.contains(node))
            {
                return duration + node_duration;
            }
            return duration;
        };

        const auto shared_duration = std::accumulate(
            begin(unpacked.nodes), end(unpacked.nodes), EdgeDuration{0}, add_if_seen);

        unpacked.sharing = from_alias<double>(shared_duration) / from_alias<double>(total_duration);
        BOOST_ASSERT(unpacked.sharing >= 0.);
        BOOST_ASSERT(unpacked.sharing <= 1.);

        if (unpacked.sharing > parameters.kAtMostSameBy)
        {
            return true;
        }
        else
        {
            nodes.insert(begin(unpacked.nodes), end(unpacked.nodes));
            return false;
        }
    };

    auto end = std::remove_if(first + 1, last, over_sharing_limit);
    std::sort(
        first + 1, end, [](const auto &lhs, const auto &rhs) { return lhs.sharing < rhs.sharing; });
    return end;
}

// Filters annotated routes by stretch based on duration. Mutates range in-place.
// Returns an iterator to the filtered range's new end.
template <typename RandIt>
RandIt filterAnnotatedRoutesByStretch(RandIt first,
                                      RandIt last,
                                      const InternalRouteResult &shortest_route,
                                      const Parameters &parameters)
{
    util::static_assert_iter_category<RandIt, std::random_access_iterator_tag>();
    util::static_assert_iter_value<RandIt, InternalRouteResult>();

    BOOST_ASSERT(shortest_route.is_valid());

    const auto shortest_route_duration = shortest_route.duration();
    const auto stretch_duration_limit =
        (1. + parameters.kAtMostLongerBy) * from_alias<double>(shortest_route_duration);

    const auto over_duration_limit = [=](const auto &route)
    { return from_alias<double>(route.duration()) > stretch_duration_limit; };

    return std::remove_if(first, last, over_duration_limit);
}

// Unpacks a range of WeightedViaNodePackedPaths into a range of WeightedViaNodeUnpackedPaths.
// Note: destroys search engine heaps for recursive unpacking. Extract heap data you need before.
template <typename InputIt, typename OutIt>
void unpackPackedPaths(InputIt first,
                       InputIt last,
                       OutIt out,
                       SearchEngineData<Algorithm> &search_engine_data,
                       const Facade &facade,
                       const PhantomEndpointCandidates &endpoint_candidates)
{
    util::static_assert_iter_category<InputIt, std::input_iterator_tag>();
    util::static_assert_iter_category<OutIt, std::output_iterator_tag>();
    util::static_assert_iter_value<InputIt, WeightedViaNodePackedPath>();

    const Partition &partition = facade.GetMultiLevelPartition();

    Heap &forward_heap = *search_engine_data.forward_heap_1;
    Heap &reverse_heap = *search_engine_data.reverse_heap_1;

    for (auto it = first; it != last; ++it, ++out)
    {
        const auto packed_path_weight = it->via.weight;
        const auto packed_path_via = it->via.node;

        const auto &packed_path = it->path;

        //
        // Todo: dup. code with mld::search except for level entry: we run a slight mld::search
        //       adaption here and then dispatch to mld::search for recursively descending down.
        //

        std::vector<NodeID> unpacked_nodes;
        std::vector<EdgeID> unpacked_edges;
        unpacked_nodes.reserve(packed_path.size());
        unpacked_edges.reserve(packed_path.size());

        // Beware the edge case when start, via, end are all the same.
        // In this case we return a single node, no edges. We also don't unpack.
        if (packed_path.empty())
        {
            const auto source_node = packed_path_via;
            unpacked_nodes.push_back(source_node);
        }
        else
        {
            const auto source_node = std::get<0>(packed_path.front());
            unpacked_nodes.push_back(source_node);
        }

        for (auto const &packed_edge : packed_path)
        {
            NodeID source, target;
            bool overlay_edge;
            std::tie(source, target, overlay_edge) = packed_edge;
            if (!overlay_edge)
            { // a base graph edge
                unpacked_nodes.push_back(target);
                unpacked_edges.push_back(facade.FindEdge(source, target));
            }
            else
            { // an overlay graph edge
                LevelID level = getNodeQueryLevel(partition, source, endpoint_candidates); // XXX
                CellID parent_cell_id = partition.GetCell(level, source);
                BOOST_ASSERT(parent_cell_id == partition.GetCell(level, target));

                LevelID sublevel = level - 1;

                // Here heaps can be reused, let's go deeper!
                forward_heap.Clear();
                reverse_heap.Clear();
                forward_heap.Insert(source, {0}, {source});
                reverse_heap.Insert(target, {0}, {target});

                BOOST_ASSERT(!facade.ExcludeNode(source));
                BOOST_ASSERT(!facade.ExcludeNode(target));

                auto unpacked_subpath = search(search_engine_data,
                                               facade,
                                               forward_heap,
                                               reverse_heap,
                                               {},
                                               INVALID_EDGE_WEIGHT,
                                               sublevel,
                                               parent_cell_id);
                BOOST_ASSERT(!unpacked_subpath.edges.empty());
                BOOST_ASSERT(unpacked_subpath.nodes.size() > 1);
                BOOST_ASSERT(unpacked_subpath.nodes.front() == source);
                BOOST_ASSERT(unpacked_subpath.nodes.back() == target);
                unpacked_nodes.insert(unpacked_nodes.end(),
                                      std::next(unpacked_subpath.nodes.begin()),
                                      unpacked_subpath.nodes.end());
                unpacked_edges.insert(unpacked_edges.end(),
                                      unpacked_subpath.edges.begin(),
                                      unpacked_subpath.edges.end());
            }
        }

        WeightedViaNodeUnpackedPath unpacked_path{
            0.0,
            WeightedViaNode{packed_path_via, packed_path_weight},
            std::move(unpacked_nodes),
            std::move(unpacked_edges)};

        out = std::move(unpacked_path);
    }
}

// Generates via candidate nodes from the overlap of the two search spaces from s and t.
// Returns via node candidates in no particular order; they're not guaranteed to be unique.
// Note: heaps are modified in-place, after the function returns they're valid and can be used.
inline std::vector<WeightedViaNode>
makeCandidateVias(SearchEngineData<Algorithm> &search_engine_data,
                  const Facade &facade,
                  const PhantomEndpointCandidates &endpoint_candidates,
                  const Parameters &parameters)
{
    Heap &forward_heap = *search_engine_data.forward_heap_1;
    Heap &reverse_heap = *search_engine_data.reverse_heap_1;

    insertNodesInHeaps(forward_heap, reverse_heap, endpoint_candidates);
    if (forward_heap.Empty() || reverse_heap.Empty())
    {
        return {};
    }

    // The single via node in the shortest paths s,via and via,t sub-paths and
    // the weight for the shortest path s,t we return and compare alternatives to.
    EdgeWeight shortest_path_weight = INVALID_EDGE_WEIGHT;

    // The current via node during search spaces overlap stepping and an artificial
    // weight (overlap factor * shortest path weight) we use as a termination criteria.
    NodeID overlap_via = SPECIAL_NODEID;
    EdgeWeight overlap_weight = INVALID_EDGE_WEIGHT;

    // All via nodes in the overlapping search space (except the shortest path via node).
    // Will be filtered and ranked and then used for s,via and via,t alternative paths.
    std::vector<WeightedViaNode> candidate_vias;

    // The logic below is a bit weird - here's why: we want to re-use the MLD routingStep for
    // stepping our search space from s and from t. We don't know how far to overlap until we have
    // the shortest path. Once we have the shortest path we can use its weight to terminate when
    // we're over factor * weight. We have to set the weight for routingStep to INVALID_EDGE_WEIGHT
    // so that stepping will continue even after we reached the shortest path upper bound.

    EdgeWeight forward_heap_min = forward_heap.MinKey();
    EdgeWeight reverse_heap_min = reverse_heap.MinKey();

    while (forward_heap.Size() + reverse_heap.Size() > 0)
    {
        if (shortest_path_weight != INVALID_EDGE_WEIGHT)
            overlap_weight = to_alias<EdgeWeight>(from_alias<double>(shortest_path_weight) *
                                                  parameters.kSearchSpaceOverlapFactor);

        // Termination criteria - when we have a shortest path this will guarantee for our overlap.
        const bool keep_going = forward_heap_min + reverse_heap_min < overlap_weight;

        if (!keep_going)
            break;

        // Force forward step to not break early when we reached the middle, continue for overlap.
        // Note: only invalidate the via node, the weight upper bound is still correct!
        overlap_via = SPECIAL_NODEID;

        if (!forward_heap.Empty())
        {
            routingStep<FORWARD_DIRECTION>(facade,
                                           forward_heap,
                                           reverse_heap,
                                           overlap_via,
                                           overlap_weight,
                                           {},
                                           endpoint_candidates);

            if (!forward_heap.Empty())
                forward_heap_min = forward_heap.MinKey();

            if (overlap_weight != INVALID_EDGE_WEIGHT && overlap_via != SPECIAL_NODEID)
            {
                candidate_vias.push_back(WeightedViaNode{overlap_via, overlap_weight});
            }
        }

        // Adjusting upper bound for forward search
        shortest_path_weight = std::min(shortest_path_weight, overlap_weight);
        // Force reverse step to not break early when we reached the middle, continue for overlap.
        // Note: only invalidate the via node, the weight upper bound is still correct!
        overlap_via = SPECIAL_NODEID;

        if (!reverse_heap.Empty())
        {
            routingStep<REVERSE_DIRECTION>(facade,
                                           reverse_heap,
                                           forward_heap,
                                           overlap_via,
                                           overlap_weight,
                                           {},
                                           endpoint_candidates);

            if (!reverse_heap.Empty())
                reverse_heap_min = reverse_heap.MinKey();

            if (overlap_weight != INVALID_EDGE_WEIGHT && overlap_via != SPECIAL_NODEID)
            {
                candidate_vias.push_back(WeightedViaNode{overlap_via, overlap_weight});
            }
        }

        // Adjusting upper bound for reverse search
        shortest_path_weight = std::min(shortest_path_weight, overlap_weight);
    }

    return candidate_vias;
}

} // namespace

// Alternative Routes for MLD.
//
// Start search from s and continue "for a while" when middle was found. Save vertices.
// Start search from t and continue "for a while" when middle was found. Save vertices.
// Intersect both vertex sets: these are the candidate vertices.
// For all candidate vertices c a (potentially arbitrarily bad) alternative route is (s, c, t).
// Apply heuristic to evaluate alternative route based on stretch, overlap, how reasonable it is.
//
// For MLD specifically we can pull off some tricks to make evaluating alternatives fast:
//   Only consider (s, c, t) with c border vertex: re-use MLD search steps.
//   Add meta data to border vertices: consider (s, c, t) only when c is e.g. on a highway.
//   Prune based on vertex cell id
//
// https://github.com/Project-OSRM/osrm-backend/issues/3905
InternalManyRoutesResult alternativePathSearch(SearchEngineData<Algorithm> &search_engine_data,
                                               const Facade &facade,
                                               const PhantomEndpointCandidates &endpoint_candidates,
                                               unsigned number_of_alternatives)
{
    Parameters parameters = parametersFromRequest(endpoint_candidates);

    const auto max_number_of_alternatives = number_of_alternatives;
    const auto max_number_of_alternatives_to_unpack =
        parameters.kAlternativesToUnpackFactor * max_number_of_alternatives;
    BOOST_ASSERT(max_number_of_alternatives > 0);
    BOOST_ASSERT(max_number_of_alternatives_to_unpack >= max_number_of_alternatives);

    const Partition &partition = facade.GetMultiLevelPartition();

    // Prepare heaps for usage below. The searches will modify them in-place.
    search_engine_data.InitializeOrClearFirstThreadLocalStorage(facade.GetNumberOfNodes(),
                                                                facade.GetMaxBorderNodeID() + 1);

    Heap &forward_heap = *search_engine_data.forward_heap_1;
    Heap &reverse_heap = *search_engine_data.reverse_heap_1;

    // Do forward and backward search, save search space overlap as via candidates.
    auto candidate_vias =
        makeCandidateVias(search_engine_data, facade, endpoint_candidates, parameters);

    const auto by_weight = [](const auto &lhs, const auto &rhs) { return lhs.weight < rhs.weight; };
    auto shortest_path_via_it =
        std::min_element(begin(candidate_vias), end(candidate_vias), by_weight);

    const auto has_shortest_path = shortest_path_via_it != end(candidate_vias) &&
                                   shortest_path_via_it->node != SPECIAL_NODEID &&
                                   shortest_path_via_it->weight != INVALID_EDGE_WEIGHT;

    // Care needs to be taken to meet the call sites post condition.
    // We must return at least one route, even if it's an invalid one.
    if (!has_shortest_path)
    {
        InternalRouteResult invalid;
        return invalid;
    }

    NodeID shortest_path_via = shortest_path_via_it->node;
    EdgeWeight shortest_path_weight = shortest_path_via_it->weight;

    const double duration_estimation =
        from_alias<double>(shortest_path_weight) / facade.GetWeightMultiplier();
    parameters.kAtMostLongerBy =
        getLongerByFactorBasedOnDuration(to_alias<EdgeDuration>(duration_estimation));

    // Filters via candidate nodes with heuristics

    // Note: filter pipeline below only makes range smaller; no need to erase items
    // from the vector when we can mutate in-place and for filtering adjust iterators.
    auto it = end(candidate_vias);

    it = filterViaCandidatesByUniqueNodeIds(begin(candidate_vias), it);
    it = filterViaCandidatesByRoadImportance(begin(candidate_vias), it, facade);
    it = filterViaCandidatesByStretch(begin(candidate_vias), it, shortest_path_weight, parameters);

    // Pre-rank by weight; sharing filtering below then discards by similarity.
    std::sort(begin(candidate_vias),
              it,
              [](const auto lhs, const auto rhs) { return lhs.weight < rhs.weight; });

    // Filtered and ranked candidate range
    const auto candidate_vias_first = begin(candidate_vias);
    const auto candidate_vias_last = it;
    const auto number_of_candidate_vias = candidate_vias_last - candidate_vias_first;

    // Reconstruct packed paths from the heaps.
    // The recursive path unpacking below destructs heaps.
    // We need to save all packed paths from the heaps upfront.

    const auto extract_packed_path_from_heaps = [&](WeightedViaNode via)
    {
        auto packed_path = retrievePackedPathFromHeap(forward_heap, reverse_heap, via.node);

        return WeightedViaNodePackedPath{via, std::move(packed_path)};
    };

    std::vector<WeightedViaNodePackedPath> weighted_packed_paths;
    weighted_packed_paths.reserve(1 + number_of_candidate_vias);

    // Store shortest path
    WeightedViaNode shortest_path_weighted_via{shortest_path_via, shortest_path_weight};
    weighted_packed_paths.push_back(extract_packed_path_from_heaps(shortest_path_weighted_via));

    const auto last_filtered = filterViaCandidatesByViaNotOnPath(
        weighted_packed_paths[0], candidate_vias_first + 1, candidate_vias_last);

    // Store all alternative packed paths (if there are any).
    auto into = std::back_inserter(weighted_packed_paths);
    std::transform(begin(candidate_vias) + 1, last_filtered, into, extract_packed_path_from_heaps);

    // Filter packed paths with heuristics

    auto alternative_paths_last = end(weighted_packed_paths);

    alternative_paths_last = filterPackedPathsByLocalOptimality(weighted_packed_paths[0],
                                                                forward_heap, // paths for s, via
                                                                reverse_heap, // paths for via, t
                                                                begin(weighted_packed_paths) + 1,
                                                                alternative_paths_last,
                                                                parameters);
    alternative_paths_last = filterPackedPathsByCellSharing(
        begin(weighted_packed_paths), alternative_paths_last, partition, parameters);

    BOOST_ASSERT(weighted_packed_paths.size() >= 1);

    const auto number_of_filtered_alternative_paths = std::min(
        static_cast<std::size_t>(max_number_of_alternatives_to_unpack),
        static_cast<std::size_t>(alternative_paths_last - (begin(weighted_packed_paths) + 1)));

    const auto paths_first = begin(weighted_packed_paths);
    const auto paths_last = begin(weighted_packed_paths) + 1 + number_of_filtered_alternative_paths;
    const auto number_of_packed_paths = paths_last - paths_first;

    std::vector<WeightedViaNodeUnpackedPath> unpacked_paths;
    unpacked_paths.reserve(number_of_packed_paths);

    // Note: re-uses (read: destroys) heaps; we don't need them from here on anyway.
    unpackPackedPaths(paths_first,
                      paths_last,
                      std::back_inserter(unpacked_paths),
                      search_engine_data,
                      facade,
                      endpoint_candidates);

    //
    // Filter and rank a second time. This time instead of being fast and doing
    // heuristics on the packed path only we now have the detailed unpacked path.
    //

    auto unpacked_paths_last = end(unpacked_paths);

    unpacked_paths_last = filterUnpackedPathsBySharing(
        begin(unpacked_paths), end(unpacked_paths), facade, parameters);

    const auto unpacked_paths_first = begin(unpacked_paths);
    const auto number_of_unpacked_paths =
        std::min(static_cast<std::size_t>(max_number_of_alternatives) + 1,
                 static_cast<std::size_t>(unpacked_paths_last - unpacked_paths_first));
    BOOST_ASSERT(number_of_unpacked_paths >= 1);
    unpacked_paths_last = unpacked_paths_first + number_of_unpacked_paths;

    //
    // Annotate the unpacked path and transform to proper internal route result.
    //

    std::vector<InternalRouteResult> routes;
    routes.reserve(number_of_unpacked_paths);

    const auto unpacked_path_to_route = [&](const WeightedViaNodeUnpackedPath &path)
    { return extractRoute(facade, path.via.weight, endpoint_candidates, path.nodes, path.edges); };

    std::transform(unpacked_paths_first,
                   unpacked_paths_last,
                   std::back_inserter(routes),
                   unpacked_path_to_route);

    BOOST_ASSERT(routes.size() >= 1);

    // Only now that we annotated the routes do we have their actual duration.

    const auto routes_first = begin(routes);
    auto routes_last = end(routes);

    if (routes.size() > 1)
    {
        parameters.kAtMostLongerBy = getLongerByFactorBasedOnDuration(routes_first->duration());
        routes_last = filterAnnotatedRoutesByStretch(
            routes_first + 1, routes_last, *routes_first, parameters);
        routes.erase(routes_last, end(routes));
    }

    BOOST_ASSERT(routes.size() >= 1);
    return InternalManyRoutesResult{std::move(routes)};
}

} // namespace osrm::engine::routing_algorithms

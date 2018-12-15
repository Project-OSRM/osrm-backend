#include "engine/routing_algorithms/alternative_path.hpp"
#include "engine/routing_algorithms/routing_base_ch.hpp"

#include "util/integer_range.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <iterator>
#include <memory>
#include <unordered_set>
#include <vector>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

// Unqualified calls below are from the ch namespace.
// This alternative implementation works only for ch.
using namespace ch;

namespace
{
const double constexpr VIAPATH_ALPHA = 0.25;   // alternative is local optimum on 25% sub-paths
const double constexpr VIAPATH_EPSILON = 0.15; // alternative at most 15% longer
const double constexpr VIAPATH_GAMMA = 0.75;   // alternative shares at most 75% with the shortest.

using QueryHeap = SearchEngineData<Algorithm>::QueryHeap;
using SearchSpaceEdge = std::pair<NodeID, NodeID>;

struct RankedCandidateNode
{
    RankedCandidateNode(const NodeID node, const EdgeWeight weight, const EdgeWeight sharing)
        : node(node), weight(weight), sharing(sharing)
    {
    }

    NodeID node;
    EdgeWeight weight;
    EdgeWeight sharing;

    bool operator<(const RankedCandidateNode &other) const
    {
        return (2 * weight + sharing) < (2 * other.weight + other.sharing);
    }
};

// todo: reorder parameters
template <bool DIRECTION>
void alternativeRoutingStep(const DataFacade<Algorithm> &facade,
                            QueryHeap &heap1,
                            QueryHeap &heap2,
                            NodeID *middle_node,
                            EdgeWeight *upper_bound_to_shortest_path_weight,
                            std::vector<NodeID> &search_space_intersection,
                            std::vector<SearchSpaceEdge> &search_space,
                            const EdgeWeight min_edge_offset)
{
    QueryHeap &forward_heap = DIRECTION == FORWARD_DIRECTION ? heap1 : heap2;
    QueryHeap &reverse_heap = DIRECTION == FORWARD_DIRECTION ? heap2 : heap1;

    const NodeID node = forward_heap.DeleteMin();
    const EdgeWeight weight = forward_heap.GetKey(node);

    const auto scaled_weight =
        static_cast<EdgeWeight>((weight + min_edge_offset) / (1. + VIAPATH_EPSILON));
    if ((INVALID_EDGE_WEIGHT != *upper_bound_to_shortest_path_weight) &&
        (scaled_weight > *upper_bound_to_shortest_path_weight))
    {
        forward_heap.DeleteAll();
        return;
    }

    search_space.emplace_back(forward_heap.GetData(node).parent, node);

    if (reverse_heap.WasInserted(node))
    {
        search_space_intersection.emplace_back(node);
        const EdgeWeight new_weight = reverse_heap.GetKey(node) + weight;
        if (new_weight < *upper_bound_to_shortest_path_weight)
        {
            if (new_weight >= 0)
            {
                *middle_node = node;
                *upper_bound_to_shortest_path_weight = new_weight;
            }
            else
            {
                // check whether there is a loop present at the node
                const auto loop_weight = std::get<0>(getLoopWeight<false>(facade, node));
                const EdgeWeight new_weight_with_loop = new_weight + loop_weight;
                if (loop_weight != INVALID_EDGE_WEIGHT &&
                    new_weight_with_loop <= *upper_bound_to_shortest_path_weight)
                {
                    *middle_node = node;
                    *upper_bound_to_shortest_path_weight = loop_weight;
                }
            }
        }
    }

    for (auto edge : facade.GetAdjacentEdgeRange(node))
    {
        const auto &data = facade.GetEdgeData(edge);
        if (DIRECTION == FORWARD_DIRECTION ? data.forward : data.backward)
        {
            const NodeID to = facade.GetTarget(edge);
            const EdgeWeight edge_weight = data.weight;

            BOOST_ASSERT(edge_weight > 0);
            const EdgeWeight to_weight = weight + edge_weight;

            // New Node discovered -> Add to Heap + Node Info Storage
            if (!forward_heap.WasInserted(to))
            {
                forward_heap.Insert(to, to_weight, node);
            }
            // Found a shorter Path -> Update weight
            else if (to_weight < forward_heap.GetKey(to))
            {
                // new parent
                forward_heap.GetData(to).parent = node;
                // decreased weight
                forward_heap.DecreaseKey(to, to_weight);
            }
        }
    }
}

void retrievePackedAlternatePath(const QueryHeap &forward_heap1,
                                 const QueryHeap &reverse_heap1,
                                 const QueryHeap &forward_heap2,
                                 const QueryHeap &reverse_heap2,
                                 const NodeID s_v_middle,
                                 const NodeID v_t_middle,
                                 std::vector<NodeID> &packed_path)
{
    // fetch packed path [s,v)
    std::vector<NodeID> packed_v_t_path;
    retrievePackedPathFromHeap(forward_heap1, reverse_heap2, s_v_middle, packed_path);
    packed_path.pop_back(); // remove middle node. It's in both half-paths

    // fetch patched path [v,t]
    retrievePackedPathFromHeap(forward_heap2, reverse_heap1, v_t_middle, packed_v_t_path);

    packed_path.insert(packed_path.end(), packed_v_t_path.begin(), packed_v_t_path.end());
}

// TODO: reorder parameters
// compute and unpack <s,..,v> and <v,..,t> by exploring search spaces
// from v and intersecting against queues. only half-searches have to be
// done at this stage
void computeWeightAndSharingOfViaPath(SearchEngineData<Algorithm> &engine_working_data,
                                      const DataFacade<Algorithm> &facade,
                                      const NodeID via_node,
                                      EdgeWeight *real_weight_of_via_path,
                                      EdgeWeight *sharing_of_via_path,
                                      const std::vector<NodeID> &packed_shortest_path,
                                      const EdgeWeight min_edge_offset)
{
    engine_working_data.InitializeOrClearSecondThreadLocalStorage(facade.GetNumberOfNodes());

    auto &existing_forward_heap = *engine_working_data.forward_heap_1;
    auto &existing_reverse_heap = *engine_working_data.reverse_heap_1;
    auto &new_forward_heap = *engine_working_data.forward_heap_2;
    auto &new_reverse_heap = *engine_working_data.reverse_heap_2;

    std::vector<NodeID> packed_s_v_path;
    std::vector<NodeID> packed_v_t_path;

    std::vector<NodeID> partially_unpacked_shortest_path;
    std::vector<NodeID> partially_unpacked_via_path;

    NodeID s_v_middle = SPECIAL_NODEID;
    EdgeWeight upper_bound_s_v_path_weight = INVALID_EDGE_WEIGHT;
    new_reverse_heap.Insert(via_node, 0, via_node);
    // compute path <s,..,v> by reusing forward search from s
    while (!new_reverse_heap.Empty())
    {
        routingStep<REVERSE_DIRECTION>(facade,
                                       new_reverse_heap,
                                       existing_forward_heap,
                                       s_v_middle,
                                       upper_bound_s_v_path_weight,
                                       min_edge_offset,
                                       DO_NOT_FORCE_LOOPS,
                                       DO_NOT_FORCE_LOOPS);
    }
    // compute path <v,..,t> by reusing backward search from node t
    NodeID v_t_middle = SPECIAL_NODEID;
    EdgeWeight upper_bound_of_v_t_path_weight = INVALID_EDGE_WEIGHT;
    new_forward_heap.Insert(via_node, 0, via_node);
    while (!new_forward_heap.Empty())
    {
        routingStep<FORWARD_DIRECTION>(facade,
                                       new_forward_heap,
                                       existing_reverse_heap,
                                       v_t_middle,
                                       upper_bound_of_v_t_path_weight,
                                       min_edge_offset,
                                       DO_NOT_FORCE_LOOPS,
                                       DO_NOT_FORCE_LOOPS);
    }
    *real_weight_of_via_path = upper_bound_s_v_path_weight + upper_bound_of_v_t_path_weight;

    if (SPECIAL_NODEID == s_v_middle || SPECIAL_NODEID == v_t_middle)
    {
        return;
    }

    // retrieve packed paths
    retrievePackedPathFromHeap(
        existing_forward_heap, new_reverse_heap, s_v_middle, packed_s_v_path);
    retrievePackedPathFromHeap(
        new_forward_heap, existing_reverse_heap, v_t_middle, packed_v_t_path);

    // partial unpacking, compute sharing
    // First partially unpack s-->v until paths deviate, note length of common path.
    const auto s_v_min_path_size =
        std::min(packed_s_v_path.size(), packed_shortest_path.size()) - 1;
    for (const auto current_node : util::irange<std::size_t>(0UL, s_v_min_path_size))
    {
        if (packed_s_v_path[current_node] == packed_shortest_path[current_node] &&
            packed_s_v_path[current_node + 1] == packed_shortest_path[current_node + 1])
        {
            EdgeID edgeID = facade.FindEdgeInEitherDirection(packed_s_v_path[current_node],
                                                             packed_s_v_path[current_node + 1]);
            *sharing_of_via_path += facade.GetEdgeData(edgeID).weight;
        }
        else
        {
            if (packed_s_v_path[current_node] == packed_shortest_path[current_node])
            {
                unpackEdge(facade,
                           packed_s_v_path[current_node],
                           packed_s_v_path[current_node + 1],
                           partially_unpacked_via_path);
                unpackEdge(facade,
                           packed_shortest_path[current_node],
                           packed_shortest_path[current_node + 1],
                           partially_unpacked_shortest_path);
                break;
            }
        }
    }
    // traverse partially unpacked edge and note common prefix
    const int64_t packed_path_length =
        static_cast<int64_t>(
            std::min(partially_unpacked_via_path.size(), partially_unpacked_shortest_path.size())) -
        1;
    for (int64_t current_node = 0; (current_node < packed_path_length) &&
                                   (partially_unpacked_via_path[current_node] ==
                                        partially_unpacked_shortest_path[current_node] &&
                                    partially_unpacked_via_path[current_node + 1] ==
                                        partially_unpacked_shortest_path[current_node + 1]);
         ++current_node)
    {
        EdgeID selected_edge =
            facade.FindEdgeInEitherDirection(partially_unpacked_via_path[current_node],
                                             partially_unpacked_via_path[current_node + 1]);
        *sharing_of_via_path += facade.GetEdgeData(selected_edge).weight;
    }

    // Second, partially unpack v-->t in reverse order until paths deviate and note lengths
    int64_t via_path_index = static_cast<int64_t>(packed_v_t_path.size()) - 1;
    int64_t shortest_path_index = static_cast<int64_t>(packed_shortest_path.size()) - 1;
    for (; via_path_index > 0 && shortest_path_index > 0; --via_path_index, --shortest_path_index)
    {
        if (packed_v_t_path[via_path_index - 1] == packed_shortest_path[shortest_path_index - 1] &&
            packed_v_t_path[via_path_index] == packed_shortest_path[shortest_path_index])
        {
            EdgeID edgeID = facade.FindEdgeInEitherDirection(packed_v_t_path[via_path_index - 1],
                                                             packed_v_t_path[via_path_index]);
            *sharing_of_via_path += facade.GetEdgeData(edgeID).weight;
        }
        else
        {
            if (packed_v_t_path[via_path_index] == packed_shortest_path[shortest_path_index])
            {
                unpackEdge(facade,
                           packed_v_t_path[via_path_index - 1],
                           packed_v_t_path[via_path_index],
                           partially_unpacked_via_path);
                unpackEdge(facade,
                           packed_shortest_path[shortest_path_index - 1],
                           packed_shortest_path[shortest_path_index],
                           partially_unpacked_shortest_path);
                break;
            }
        }
    }

    via_path_index = static_cast<int64_t>(partially_unpacked_via_path.size()) - 1;
    shortest_path_index = static_cast<int64_t>(partially_unpacked_shortest_path.size()) - 1;
    for (; via_path_index > 0 && shortest_path_index > 0; --via_path_index, --shortest_path_index)
    {
        if (partially_unpacked_via_path[via_path_index - 1] ==
                partially_unpacked_shortest_path[shortest_path_index - 1] &&
            partially_unpacked_via_path[via_path_index] ==
                partially_unpacked_shortest_path[shortest_path_index])
        {
            EdgeID edgeID =
                facade.FindEdgeInEitherDirection(partially_unpacked_via_path[via_path_index - 1],
                                                 partially_unpacked_via_path[via_path_index]);
            *sharing_of_via_path += facade.GetEdgeData(edgeID).weight;
        }
        else
        {
            break;
        }
    }
    // finished partial unpacking spree! Amount of sharing is stored to appropriate pointer
    // variable
}

// conduct T-Test
bool viaNodeCandidatePassesTTest(SearchEngineData<Algorithm> &engine_working_data,
                                 const DataFacade<Algorithm> &facade,
                                 QueryHeap &existing_forward_heap,
                                 QueryHeap &existing_reverse_heap,
                                 QueryHeap &new_forward_heap,
                                 QueryHeap &new_reverse_heap,
                                 const RankedCandidateNode &candidate,
                                 const EdgeWeight weight_of_shortest_path,
                                 EdgeWeight *weight_of_via_path,
                                 NodeID *s_v_middle,
                                 NodeID *v_t_middle,
                                 const EdgeWeight min_edge_offset)
{
    new_forward_heap.Clear();
    new_reverse_heap.Clear();
    std::vector<NodeID> packed_s_v_path;
    std::vector<NodeID> packed_v_t_path;

    *s_v_middle = SPECIAL_NODEID;
    EdgeWeight upper_bound_s_v_path_weight = INVALID_EDGE_WEIGHT;
    // compute path <s,..,v> by reusing forward search from s
    new_reverse_heap.Insert(candidate.node, 0, candidate.node);
    while (new_reverse_heap.Size() > 0)
    {
        routingStep<REVERSE_DIRECTION>(facade,
                                       new_reverse_heap,
                                       existing_forward_heap,
                                       *s_v_middle,
                                       upper_bound_s_v_path_weight,
                                       min_edge_offset,
                                       DO_NOT_FORCE_LOOPS,
                                       DO_NOT_FORCE_LOOPS);
    }

    if (INVALID_EDGE_WEIGHT == upper_bound_s_v_path_weight)
    {
        return false;
    }

    // compute path <v,..,t> by reusing backward search from t
    *v_t_middle = SPECIAL_NODEID;
    EdgeWeight upper_bound_of_v_t_path_weight = INVALID_EDGE_WEIGHT;
    new_forward_heap.Insert(candidate.node, 0, candidate.node);
    while (new_forward_heap.Size() > 0)
    {
        routingStep<FORWARD_DIRECTION>(facade,
                                       new_forward_heap,
                                       existing_reverse_heap,
                                       *v_t_middle,
                                       upper_bound_of_v_t_path_weight,
                                       min_edge_offset,
                                       DO_NOT_FORCE_LOOPS,
                                       DO_NOT_FORCE_LOOPS);
    }

    if (INVALID_EDGE_WEIGHT == upper_bound_of_v_t_path_weight)
    {
        return false;
    }

    *weight_of_via_path = upper_bound_s_v_path_weight + upper_bound_of_v_t_path_weight;

    // retrieve packed paths
    retrievePackedPathFromHeap(
        existing_forward_heap, new_reverse_heap, *s_v_middle, packed_s_v_path);

    retrievePackedPathFromHeap(
        new_forward_heap, existing_reverse_heap, *v_t_middle, packed_v_t_path);

    NodeID s_P = *s_v_middle, t_P = *v_t_middle;
    if (SPECIAL_NODEID == s_P)
    {
        return false;
    }

    if (SPECIAL_NODEID == t_P)
    {
        return false;
    }
    const EdgeWeight T_threshold = static_cast<EdgeWeight>(VIAPATH_ALPHA * weight_of_shortest_path);
    EdgeWeight unpacked_until_weight = 0;

    std::stack<SearchSpaceEdge> unpack_stack;
    // Traverse path s-->v
    for (std::size_t i = packed_s_v_path.size() - 1; (i > 0) && unpack_stack.empty(); --i)
    {
        const EdgeID current_edge_id =
            facade.FindEdgeInEitherDirection(packed_s_v_path[i - 1], packed_s_v_path[i]);
        const EdgeWeight weight_of_current_edge = facade.GetEdgeData(current_edge_id).weight;
        if ((weight_of_current_edge + unpacked_until_weight) >= T_threshold)
        {
            unpack_stack.emplace(packed_s_v_path[i - 1], packed_s_v_path[i]);
        }
        else
        {
            unpacked_until_weight += weight_of_current_edge;
            s_P = packed_s_v_path[i - 1];
        }
    }

    while (!unpack_stack.empty())
    {
        const SearchSpaceEdge via_path_edge = unpack_stack.top();
        unpack_stack.pop();
        EdgeID edge_in_via_path_id =
            facade.FindEdgeInEitherDirection(via_path_edge.first, via_path_edge.second);

        if (SPECIAL_EDGEID == edge_in_via_path_id)
        {
            return false;
        }

        const auto &current_edge_data = facade.GetEdgeData(edge_in_via_path_id);
        const bool current_edge_is_shortcut = current_edge_data.shortcut;
        if (current_edge_is_shortcut)
        {
            const NodeID via_path_middle_node_id = current_edge_data.turn_id;
            const EdgeID second_segment_edge_id =
                facade.FindEdgeInEitherDirection(via_path_middle_node_id, via_path_edge.second);
            const auto second_segment_weight = facade.GetEdgeData(second_segment_edge_id).weight;
            // attention: !unpacking in reverse!
            // Check if second segment is the one to go over treshold? if yes add second segment
            // to stack, else push first segment to stack and add weight of second one.
            if (unpacked_until_weight + second_segment_weight >= T_threshold)
            {
                unpack_stack.emplace(via_path_middle_node_id, via_path_edge.second);
            }
            else
            {
                unpacked_until_weight += second_segment_weight;
                unpack_stack.emplace(via_path_edge.first, via_path_middle_node_id);
            }
        }
        else
        {
            // edge is not a shortcut, set the start node for T-Test to end of edge.
            unpacked_until_weight += current_edge_data.weight;
            s_P = via_path_edge.first;
        }
    }

    EdgeWeight t_test_path_weight = unpacked_until_weight;
    unpacked_until_weight = 0;
    // Traverse path s-->v
    BOOST_ASSERT(!packed_v_t_path.empty());
    for (unsigned i = 0, packed_path_length = static_cast<unsigned>(packed_v_t_path.size() - 1);
         (i < packed_path_length) && unpack_stack.empty();
         ++i)
    {
        const EdgeID edgeID =
            facade.FindEdgeInEitherDirection(packed_v_t_path[i], packed_v_t_path[i + 1]);
        auto weight_of_current_edge = facade.GetEdgeData(edgeID).weight;
        if (weight_of_current_edge + unpacked_until_weight >= T_threshold)
        {
            unpack_stack.emplace(packed_v_t_path[i], packed_v_t_path[i + 1]);
        }
        else
        {
            unpacked_until_weight += weight_of_current_edge;
            t_P = packed_v_t_path[i + 1];
        }
    }

    while (!unpack_stack.empty())
    {
        const SearchSpaceEdge via_path_edge = unpack_stack.top();
        unpack_stack.pop();
        EdgeID edge_in_via_path_id =
            facade.FindEdgeInEitherDirection(via_path_edge.first, via_path_edge.second);
        if (SPECIAL_EDGEID == edge_in_via_path_id)
        {
            return false;
        }

        const auto &current_edge_data = facade.GetEdgeData(edge_in_via_path_id);
        const bool IsViaEdgeShortCut = current_edge_data.shortcut;
        if (IsViaEdgeShortCut)
        {
            const NodeID middleOfViaPath = current_edge_data.turn_id;
            EdgeID edgeIDOfFirstSegment =
                facade.FindEdgeInEitherDirection(via_path_edge.first, middleOfViaPath);
            auto weightOfFirstSegment = facade.GetEdgeData(edgeIDOfFirstSegment).weight;
            // Check if first segment is the one to go over treshold? if yes first segment to
            // stack, else push second segment to stack and add weight of first one.
            if (unpacked_until_weight + weightOfFirstSegment >= T_threshold)
            {
                unpack_stack.emplace(via_path_edge.first, middleOfViaPath);
            }
            else
            {
                unpacked_until_weight += weightOfFirstSegment;
                unpack_stack.emplace(middleOfViaPath, via_path_edge.second);
            }
        }
        else
        {
            // edge is not a shortcut, set the start node for T-Test to end of edge.
            unpacked_until_weight += current_edge_data.weight;
            t_P = via_path_edge.second;
        }
    }

    t_test_path_weight += unpacked_until_weight;
    // Run actual T-Test query and compare if weight equal.
    engine_working_data.InitializeOrClearThirdThreadLocalStorage(facade.GetNumberOfNodes());

    QueryHeap &forward_heap3 = *engine_working_data.forward_heap_3;
    QueryHeap &reverse_heap3 = *engine_working_data.reverse_heap_3;
    EdgeWeight upper_bound = INVALID_EDGE_WEIGHT;
    NodeID middle = SPECIAL_NODEID;

    forward_heap3.Insert(s_P, 0, s_P);
    reverse_heap3.Insert(t_P, 0, t_P);
    // exploration from s and t until deletemin/(1+epsilon) > _lengt_oO_sShortest_path
    while ((forward_heap3.Size() + reverse_heap3.Size()) > 0)
    {
        if (!forward_heap3.Empty())
        {
            routingStep<FORWARD_DIRECTION>(facade,
                                           forward_heap3,
                                           reverse_heap3,
                                           middle,
                                           upper_bound,
                                           min_edge_offset,
                                           DO_NOT_FORCE_LOOPS,
                                           DO_NOT_FORCE_LOOPS);
        }
        if (!reverse_heap3.Empty())
        {
            routingStep<REVERSE_DIRECTION>(facade,
                                           reverse_heap3,
                                           forward_heap3,
                                           middle,
                                           upper_bound,
                                           min_edge_offset,
                                           DO_NOT_FORCE_LOOPS,
                                           DO_NOT_FORCE_LOOPS);
        }
    }
    return (upper_bound <= t_test_path_weight);
}
} // anon. namespace

InternalManyRoutesResult alternativePathSearch(SearchEngineData<Algorithm> &engine_working_data,
                                               const DataFacade<Algorithm> &facade,
                                               const PhantomNodes &phantom_node_pair,
                                               unsigned /*number_of_alternatives*/)
{
    InternalRouteResult primary_route;
    InternalRouteResult secondary_route;

    primary_route.segment_end_coordinates = {phantom_node_pair};
    secondary_route.segment_end_coordinates = {phantom_node_pair};

    std::vector<NodeID> alternative_path;
    std::vector<NodeID> via_node_candidate_list;
    std::vector<SearchSpaceEdge> forward_search_space;
    std::vector<SearchSpaceEdge> reverse_search_space;

    // Init queues, semi-expensive because access to TSS invokes a sys-call
    engine_working_data.InitializeOrClearFirstThreadLocalStorage(facade.GetNumberOfNodes());
    engine_working_data.InitializeOrClearSecondThreadLocalStorage(facade.GetNumberOfNodes());
    engine_working_data.InitializeOrClearThirdThreadLocalStorage(facade.GetNumberOfNodes());

    auto &forward_heap1 = *engine_working_data.forward_heap_1;
    auto &reverse_heap1 = *engine_working_data.reverse_heap_1;
    auto &forward_heap2 = *engine_working_data.forward_heap_2;
    auto &reverse_heap2 = *engine_working_data.reverse_heap_2;

    EdgeWeight upper_bound_to_shortest_path_weight = INVALID_EDGE_WEIGHT;
    NodeID middle_node = SPECIAL_NODEID;
    const EdgeWeight min_edge_offset =
        std::min(phantom_node_pair.source_phantom.forward_segment_id.enabled
                     ? -phantom_node_pair.source_phantom.GetForwardWeightPlusOffset()
                     : 0,
                 phantom_node_pair.source_phantom.reverse_segment_id.enabled
                     ? -phantom_node_pair.source_phantom.GetReverseWeightPlusOffset()
                     : 0);

    insertNodesInHeaps(forward_heap1, reverse_heap1, phantom_node_pair);

    // search from s and t till new_min/(1+epsilon) > weight_of_shortest_path
    while (0 < (forward_heap1.Size() + reverse_heap1.Size()))
    {
        if (0 < forward_heap1.Size())
        {
            alternativeRoutingStep<FORWARD_DIRECTION>(facade,
                                                      forward_heap1,
                                                      reverse_heap1,
                                                      &middle_node,
                                                      &upper_bound_to_shortest_path_weight,
                                                      via_node_candidate_list,
                                                      forward_search_space,
                                                      min_edge_offset);
        }
        if (0 < reverse_heap1.Size())
        {
            alternativeRoutingStep<REVERSE_DIRECTION>(facade,
                                                      forward_heap1,
                                                      reverse_heap1,
                                                      &middle_node,
                                                      &upper_bound_to_shortest_path_weight,
                                                      via_node_candidate_list,
                                                      reverse_search_space,
                                                      min_edge_offset);
        }
    }

    if (INVALID_EDGE_WEIGHT == upper_bound_to_shortest_path_weight)
    {
        return InternalManyRoutesResult{std::move(primary_route)};
    }

    std::sort(begin(via_node_candidate_list), end(via_node_candidate_list));
    auto unique_end = std::unique(begin(via_node_candidate_list), end(via_node_candidate_list));
    via_node_candidate_list.resize(unique_end - begin(via_node_candidate_list));

    std::vector<NodeID> packed_forward_path;
    std::vector<NodeID> packed_reverse_path;

    const bool path_is_a_loop =
        upper_bound_to_shortest_path_weight !=
        forward_heap1.GetKey(middle_node) + reverse_heap1.GetKey(middle_node);
    if (path_is_a_loop)
    {
        // Self Loop
        packed_forward_path.push_back(middle_node);
        packed_forward_path.push_back(middle_node);
    }
    else
    {

        retrievePackedPathFromSingleHeap(forward_heap1, middle_node, packed_forward_path);
        retrievePackedPathFromSingleHeap(reverse_heap1, middle_node, packed_reverse_path);
    }

    // this set is is used as an indicator if a node is on the shortest path
    std::unordered_set<NodeID> nodes_in_path(packed_forward_path.size() +
                                             packed_reverse_path.size());
    nodes_in_path.insert(packed_forward_path.begin(), packed_forward_path.end());
    nodes_in_path.insert(middle_node);
    nodes_in_path.insert(packed_reverse_path.begin(), packed_reverse_path.end());

    std::unordered_map<NodeID, EdgeWeight> approximated_forward_sharing;
    std::unordered_map<NodeID, EdgeWeight> approximated_reverse_sharing;

    // sweep over search space, compute forward sharing for each current edge (u,v)
    for (const SearchSpaceEdge &current_edge : forward_search_space)
    {
        const NodeID u = current_edge.first;
        const NodeID v = current_edge.second;

        if (nodes_in_path.find(v) != nodes_in_path.end())
        {
            // current_edge is on shortest path => sharing(v):=queue.GetKey(v);
            approximated_forward_sharing.emplace(v, forward_heap1.GetKey(v));
        }
        else
        {
            // current edge is not on shortest path. Check if we know a value for the other
            // endpoint
            const auto sharing_of_u_iterator = approximated_forward_sharing.find(u);
            if (sharing_of_u_iterator != approximated_forward_sharing.end())
            {
                approximated_forward_sharing.emplace(v, sharing_of_u_iterator->second);
            }
        }
    }

    // sweep over search space, compute backward sharing
    for (const SearchSpaceEdge &current_edge : reverse_search_space)
    {
        const NodeID u = current_edge.first;
        const NodeID v = current_edge.second;
        if (nodes_in_path.find(v) != nodes_in_path.end())
        {
            // current_edge is on shortest path => sharing(u):=queue.GetKey(u);
            approximated_reverse_sharing.emplace(v, reverse_heap1.GetKey(v));
        }
        else
        {
            // current edge is not on shortest path. Check if we know a value for the other
            // endpoint
            const auto sharing_of_u_iterator = approximated_reverse_sharing.find(u);
            if (sharing_of_u_iterator != approximated_reverse_sharing.end())
            {
                approximated_reverse_sharing.emplace(v, sharing_of_u_iterator->second);
            }
        }
    }

    std::vector<NodeID> preselected_node_list;
    for (const NodeID node : via_node_candidate_list)
    {
        if (node == middle_node)
            continue;
        const auto fwd_iterator = approximated_forward_sharing.find(node);
        const EdgeWeight fwd_sharing =
            (fwd_iterator != approximated_forward_sharing.end()) ? fwd_iterator->second : 0;
        const auto rev_iterator = approximated_reverse_sharing.find(node);
        const EdgeWeight rev_sharing =
            (rev_iterator != approximated_reverse_sharing.end()) ? rev_iterator->second : 0;

        const EdgeWeight approximated_sharing = fwd_sharing + rev_sharing;
        const EdgeWeight approximated_weight =
            forward_heap1.GetKey(node) + reverse_heap1.GetKey(node);
        const bool weight_passes =
            (approximated_weight < upper_bound_to_shortest_path_weight * (1 + VIAPATH_EPSILON));
        const bool sharing_passes =
            (approximated_sharing <= upper_bound_to_shortest_path_weight * VIAPATH_GAMMA);
        const bool stretch_passes =
            (approximated_weight - approximated_sharing) <
            ((1. + VIAPATH_EPSILON) * (upper_bound_to_shortest_path_weight - approximated_sharing));

        if (weight_passes && sharing_passes && stretch_passes)
        {
            preselected_node_list.emplace_back(node);
        }
    }

    std::vector<NodeID> &packed_shortest_path = packed_forward_path;
    if (!path_is_a_loop)
    {
        std::reverse(packed_shortest_path.begin(), packed_shortest_path.end());
        packed_shortest_path.emplace_back(middle_node);
        packed_shortest_path.insert(
            packed_shortest_path.end(), packed_reverse_path.begin(), packed_reverse_path.end());
    }
    std::vector<RankedCandidateNode> ranked_candidates_list;

    // prioritizing via nodes for deep inspection
    for (const NodeID node : preselected_node_list)
    {
        EdgeWeight weight_of_via_path = 0, sharing_of_via_path = 0;
        computeWeightAndSharingOfViaPath(engine_working_data,
                                         facade,
                                         node,
                                         &weight_of_via_path,
                                         &sharing_of_via_path,
                                         packed_shortest_path,
                                         min_edge_offset);
        const EdgeWeight maximum_allowed_sharing =
            static_cast<EdgeWeight>(upper_bound_to_shortest_path_weight * VIAPATH_GAMMA);
        if (sharing_of_via_path <= maximum_allowed_sharing &&
            weight_of_via_path <= upper_bound_to_shortest_path_weight * (1 + VIAPATH_EPSILON))
        {
            ranked_candidates_list.emplace_back(node, weight_of_via_path, sharing_of_via_path);
        }
    }
    std::sort(ranked_candidates_list.begin(), ranked_candidates_list.end());

    NodeID selected_via_node = SPECIAL_NODEID;
    EdgeWeight weight_of_via_path = INVALID_EDGE_WEIGHT;
    NodeID s_v_middle = SPECIAL_NODEID, v_t_middle = SPECIAL_NODEID;
    for (const RankedCandidateNode &candidate : ranked_candidates_list)
    {
        if (viaNodeCandidatePassesTTest(engine_working_data,
                                        facade,
                                        forward_heap1,
                                        reverse_heap1,
                                        forward_heap2,
                                        reverse_heap2,
                                        candidate,
                                        upper_bound_to_shortest_path_weight,
                                        &weight_of_via_path,
                                        &s_v_middle,
                                        &v_t_middle,
                                        min_edge_offset))
        {
            // select first admissable
            selected_via_node = candidate.node;
            break;
        }
    }

    // Unpack shortest path and alternative, if they exist
    if (INVALID_EDGE_WEIGHT != upper_bound_to_shortest_path_weight)
    {
        BOOST_ASSERT(!packed_shortest_path.empty());
        primary_route.unpacked_path_segments.resize(1);
        primary_route.source_traversed_in_reverse.push_back(
            (packed_shortest_path.front() !=
             phantom_node_pair.source_phantom.forward_segment_id.id));
        primary_route.target_traversed_in_reverse.push_back((
            packed_shortest_path.back() != phantom_node_pair.target_phantom.forward_segment_id.id));

        unpackPath(facade,
                   // -- packed input
                   packed_shortest_path.begin(),
                   packed_shortest_path.end(),
                   // -- start of route
                   phantom_node_pair,
                   // -- unpacked output
                   primary_route.unpacked_path_segments.front());
        primary_route.shortest_path_weight = upper_bound_to_shortest_path_weight;
    }

    if (SPECIAL_NODEID != selected_via_node)
    {
        std::vector<NodeID> packed_alternate_path;
        // retrieve alternate path
        retrievePackedAlternatePath(forward_heap1,
                                    reverse_heap1,
                                    forward_heap2,
                                    reverse_heap2,
                                    s_v_middle,
                                    v_t_middle,
                                    packed_alternate_path);

        secondary_route.unpacked_path_segments.resize(1);
        secondary_route.source_traversed_in_reverse.push_back(
            (packed_alternate_path.front() !=
             phantom_node_pair.source_phantom.forward_segment_id.id));
        secondary_route.target_traversed_in_reverse.push_back(
            (packed_alternate_path.back() !=
             phantom_node_pair.target_phantom.forward_segment_id.id));

        // unpack the alternate path
        unpackPath(facade,
                   packed_alternate_path.begin(),
                   packed_alternate_path.end(),
                   phantom_node_pair,
                   secondary_route.unpacked_path_segments.front());

        secondary_route.shortest_path_weight = weight_of_via_path;
    }
    else
    {
        BOOST_ASSERT(secondary_route.shortest_path_weight == INVALID_EDGE_WEIGHT);
    }

    return InternalManyRoutesResult{{std::move(primary_route), std::move(secondary_route)}};
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm}

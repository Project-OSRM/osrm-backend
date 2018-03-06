#include "engine/routing_algorithms/routing_base_ch.hpp"

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{
namespace ch
{

/**
 * Unpacks a single edge (NodeID->NodeID) from the CH graph down to it's original non-shortcut
 * route.
 * @param from the node the CH edge starts at
 * @param to the node the CH edge finishes at
 * @param unpacked_path the sequence of original NodeIDs that make up the expanded CH edge
 */
void unpackEdge(const DataFacade<Algorithm> &facade,
                const NodeID from,
                const NodeID to,
                std::vector<NodeID> &unpacked_path)
{
    std::array<NodeID, 2> path{{from, to}};
    unpackPath(facade,
               path.begin(),
               path.end(),
               [&unpacked_path](const std::pair<NodeID, NodeID> &edge, const auto & /* data */) {
                   unpacked_path.emplace_back(edge.first);
               });
    unpacked_path.emplace_back(to);
}

void retrievePackedPathFromHeap(const SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                                const SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
                                const NodeID middle_node_id,
                                std::vector<NodeID> &packed_path)
{
    retrievePackedPathFromSingleHeap(forward_heap, middle_node_id, packed_path);
    std::reverse(packed_path.begin(), packed_path.end());
    packed_path.emplace_back(middle_node_id);
    retrievePackedPathFromSingleHeap(reverse_heap, middle_node_id, packed_path);
}

void retrievePackedPathFromSingleHeap(const SearchEngineData<Algorithm>::QueryHeap &search_heap,
                                      const NodeID middle_node_id,
                                      std::vector<NodeID> &packed_path)
{
    NodeID current_node_id = middle_node_id;
    // all initial nodes will have itself as parent, or a node not in the heap
    // in case of a core search heap. We need a distinction between core entry nodes
    // and start nodes since otherwise start node specific code that assumes
    // node == node.parent (e.g. the loop code) might get actived.
    while (current_node_id != search_heap.GetData(current_node_id).parent &&
           search_heap.WasInserted(search_heap.GetData(current_node_id).parent))
    {
        current_node_id = search_heap.GetData(current_node_id).parent;
        packed_path.emplace_back(current_node_id);
    }
}

void retrievePackedPathFromSingleManyToManyHeap(
    const SearchEngineData<Algorithm>::ManyToManyQueryHeap &search_heap,
    const NodeID middle_node_id,
    std::vector<NodeID> &packed_path)
{
    NodeID current_node_id = middle_node_id;
    // all initial nodes will have itself as parent, or a node not in the heap
    // in case of a core search heap. We need a distinction between core entry nodes
    // and start nodes since otherwise start node specific code that assumes
    // node == node.parent (e.g. the loop code) might get actived.
    while (current_node_id != search_heap.GetData(current_node_id).parent &&
           search_heap.WasInserted(search_heap.GetData(current_node_id).parent))
    {
        current_node_id = search_heap.GetData(current_node_id).parent;
        packed_path.emplace_back(current_node_id);
    }
}

// assumes that heaps are already setup correctly.
// ATTENTION: This only works if no additional offset is supplied next to the Phantom Node
// Offsets.
// In case additional offsets are supplied, you might have to force a loop first.
// A forced loop might be necessary, if source and target are on the same segment.
// If this is the case and the offsets of the respective direction are larger for the source
// than the target
// then a force loop is required (e.g. source_phantom.forward_segment_id ==
// target_phantom.forward_segment_id
// && source_phantom.GetForwardWeightPlusOffset() > target_phantom.GetForwardWeightPlusOffset())
// requires
// a force loop, if the heaps have been initialized with positive offsets.
void search(SearchEngineData<Algorithm> & /*engine_working_data*/,
            const DataFacade<Algorithm> &facade,
            SearchEngineData<Algorithm>::QueryHeap &forward_heap,
            SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
            EdgeWeight &weight,
            std::vector<NodeID> &packed_leg,
            const bool force_loop_forward,
            const bool force_loop_reverse,
            const PhantomNodes & /*phantom_nodes*/,
            const EdgeWeight weight_upper_bound)
{
    if (forward_heap.Empty() || reverse_heap.Empty())
    {
        weight = INVALID_EDGE_WEIGHT;
        return;
    }

    NodeID middle = SPECIAL_NODEID;
    weight = weight_upper_bound;

    // get offset to account for offsets on phantom nodes on compressed edges
    const auto min_edge_offset = std::min(0, forward_heap.MinKey());
    BOOST_ASSERT(min_edge_offset <= 0);
    // we only every insert negative offsets for nodes in the forward heap
    BOOST_ASSERT(reverse_heap.MinKey() >= 0);

    // run two-Target Dijkstra routing step.
    while (0 < (forward_heap.Size() + reverse_heap.Size()))
    {
        if (!forward_heap.Empty())
        {
            routingStep<FORWARD_DIRECTION>(facade,
                                           forward_heap,
                                           reverse_heap,
                                           middle,
                                           weight,
                                           min_edge_offset,
                                           force_loop_forward,
                                           force_loop_reverse);
        }
        if (!reverse_heap.Empty())
        {
            routingStep<REVERSE_DIRECTION>(facade,
                                           reverse_heap,
                                           forward_heap,
                                           middle,
                                           weight,
                                           min_edge_offset,
                                           force_loop_reverse,
                                           force_loop_forward);
        }
    }

    // No path found for both target nodes?
    if (weight_upper_bound <= weight || SPECIAL_NODEID == middle)
    {
        weight = INVALID_EDGE_WEIGHT;
        return;
    }

    // Was a paths over one of the forward/reverse nodes not found?
    BOOST_ASSERT_MSG((SPECIAL_NODEID != middle && INVALID_EDGE_WEIGHT != weight), "no path found");

    // make sure to correctly unpack loops
    if (weight != forward_heap.GetKey(middle) + reverse_heap.GetKey(middle))
    {
        // self loop makes up the full path
        packed_leg.push_back(middle);
        packed_leg.push_back(middle);
    }
    else
    {
        retrievePackedPathFromHeap(forward_heap, reverse_heap, middle, packed_leg);
    }
}

// Requires the heaps for be empty
// If heaps should be adjusted to be initialized outside of this function,
// the addition of force_loop parameters might be required
double getNetworkDistance(SearchEngineData<Algorithm> &engine_working_data,
                          const DataFacade<Algorithm> &facade,
                          SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                          SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
                          const PhantomNode &source_phantom,
                          const PhantomNode &target_phantom,
                          EdgeWeight weight_upper_bound)
{
    forward_heap.Clear();
    reverse_heap.Clear();

    insertNodesInHeaps(forward_heap, reverse_heap, {source_phantom, target_phantom});

    EdgeWeight weight = INVALID_EDGE_WEIGHT;
    std::vector<NodeID> packed_path;
    search(engine_working_data,
           facade,
           forward_heap,
           reverse_heap,
           weight,
           packed_path,
           DO_NOT_FORCE_LOOPS,
           DO_NOT_FORCE_LOOPS,
           {source_phantom, target_phantom},
           weight_upper_bound);

    if (weight == INVALID_EDGE_WEIGHT)
    {
        return std::numeric_limits<double>::max();
    }

    std::vector<PathData> unpacked_path;
    unpackPath(facade,
               packed_path.begin(),
               packed_path.end(),
               {source_phantom, target_phantom},
               unpacked_path);

    return getPathDistance(facade, unpacked_path, source_phantom, target_phantom);
}
} // namespace ch

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#include "engine/routing_algorithms/many_to_many.hpp"
#include "engine/routing_algorithms/routing_base_ch.hpp"

#include <boost/assert.hpp>
#include <boost/range/iterator_range_core.hpp>

#include <limits>
#include <memory>
#include <vector>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

namespace ch
{

inline bool addLoopWeight(const DataFacade<ch::Algorithm> &facade,
                          const NodeID node,
                          EdgeWeight &weight,
                          EdgeDuration &duration)
{ // Special case for CH when contractor creates a loop edge node->node
    BOOST_ASSERT(weight < 0);

    const auto loop_weight = ch::getLoopWeight<false>(facade, node);
    if (loop_weight != INVALID_EDGE_WEIGHT)
    {
        const auto new_weight_with_loop = weight + loop_weight;
        if (new_weight_with_loop >= 0)
        {
            weight = new_weight_with_loop;
            duration += ch::getLoopWeight<true>(facade, node);
            return true;
        }
    }

    // No loop found or adjusted weight is negative
    return false;
}

template <bool DIRECTION>
void relaxOutgoingEdges(const DataFacade<Algorithm> &facade,
                        const NodeID node,
                        const EdgeWeight weight,
                        const EdgeDuration duration,
                        typename SearchEngineData<Algorithm>::ManyToManyQueryHeap &query_heap,
                        const PhantomNode &)
{
    if (stallAtNode<DIRECTION>(facade, node, weight, query_heap))
    {
        return;
    }

    for (auto edge : facade.GetAdjacentEdgeRange(node))
    {
        const auto &data = facade.GetEdgeData(edge);
        if (DIRECTION == FORWARD_DIRECTION ? data.forward : data.backward)
        {
            const NodeID to = facade.GetTarget(edge);

            const auto edge_weight = data.weight;
            const auto edge_duration = data.duration; // data.duration will be gone. so we can get
                                                      // this info from  node -> to

            BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
            const auto to_weight = weight + edge_weight;
            const auto to_duration = duration + edge_duration;

            // New Node discovered -> Add to Heap + Node Info Storage
            if (!query_heap.WasInserted(to))
            {
                query_heap.Insert(to, to_weight, {node, to_duration});
            }
            // Found a shorter Path -> Update weight and set new parent
            else if (std::tie(to_weight, to_duration) <
                     std::tie(query_heap.GetKey(to), query_heap.GetData(to).duration))
            {
                query_heap.GetData(to) = {node, to_duration};
                query_heap.DecreaseKey(to, to_weight);
            }
        }
    }
}

void forwardRoutingStep(const DataFacade<Algorithm> &facade,
                        const unsigned row_idx,
                        const unsigned number_of_targets,
                        typename SearchEngineData<Algorithm>::ManyToManyQueryHeap &query_heap,
                        const std::vector<NodeBucket> &search_space_with_buckets,
                        std::vector<EdgeWeight> &weights_table,
                        std::vector<EdgeDuration> &durations_table,
                        std::vector<NodeID> &middle_nodes_table,
                        const PhantomNode &phantom_node)
{
    const auto node = query_heap.DeleteMin();
    const auto source_weight = query_heap.GetKey(node);
    const auto source_duration = query_heap.GetData(node).duration;

    // Check if each encountered node has an entry
    const auto &bucket_list = std::equal_range(search_space_with_buckets.begin(),
                                               search_space_with_buckets.end(),
                                               node,
                                               NodeBucket::Compare());
    for (const auto &current_bucket : boost::make_iterator_range(bucket_list))
    {
        // Get target id from bucket entry
        const auto column_idx = current_bucket.column_index;
        const auto target_weight = current_bucket.weight;
        const auto target_duration = current_bucket.duration;

        auto &current_weight = weights_table[row_idx * number_of_targets + column_idx];
        auto &current_duration = durations_table[row_idx * number_of_targets + column_idx];

        // Check if new weight is better
        auto new_weight = source_weight + target_weight;
        auto new_duration = source_duration + target_duration;

        if (new_weight < 0)
        {
            if (addLoopWeight(facade, node, new_weight, new_duration))
            {
                current_weight = std::min(current_weight, new_weight);
                current_duration = std::min(current_duration, new_duration);
                // // std::cout << "new NodeID weight<0: " << node;
                middle_nodes_table[row_idx * number_of_targets + column_idx] = node;
            }
        }
        else if (std::tie(new_weight, new_duration) < std::tie(current_weight, current_duration))
        {
            current_weight = new_weight;
            current_duration = new_duration;
            // // std::cout << "new NodeID weight>0: " << node;
            middle_nodes_table[row_idx * number_of_targets + column_idx] = node;
        }
    }

    relaxOutgoingEdges<FORWARD_DIRECTION>(
        facade, node, source_weight, source_duration, query_heap, phantom_node);
}

void backwardRoutingStep(const DataFacade<Algorithm> &facade,
                         const unsigned column_idx,
                         typename SearchEngineData<Algorithm>::ManyToManyQueryHeap &query_heap,
                         std::vector<NodeBucket> &search_space_with_buckets,
                         const PhantomNode &phantom_node)
{
    const auto node = query_heap.DeleteMin();
    const auto target_weight = query_heap.GetKey(node);
    const auto target_duration = query_heap.GetData(node).duration;
    const auto parent = query_heap.GetData(node).parent;

    // Store settled nodes in search space bucket
    search_space_with_buckets.emplace_back(node, parent, column_idx, target_weight, target_duration);

    relaxOutgoingEdges<REVERSE_DIRECTION>(
        facade, node, target_weight, target_duration, query_heap, phantom_node);
}

} // namespace ch

std::vector<NodeID> retrievePackedPathFromSearchSpace(std::vector<NodeID> &middle_nodes_table, // middle_nodes_table
    const unsigned row_idx, // in order to figure out which target is being accessed and also to access the middle_nodes_table correctly
    std::vector<NodeBucket> &search_space_with_buckets) {

// We need to write an own function retrievePackedPathFromSearchSpace that take the middle node, the target node and the search space. It iteratively needs to lookup the parent of the middle node, the parent of the parent, and so forth but not using the ManyToManySearchHeap but the search_space_with_buckets
// For a given middle node we can find the parent node over running this code to extract a list of NodeBuckets and find the parent with the smallest weight to middle.
//         const auto &bucket_list = std::equal_range(search_space_with_buckets.begin(),
//                                                search_space_with_buckets.end(),
//                                                node,
//                                                NodeBucket::Compare());
// For start to middle we run retrievePackedPathFromSingleHeap on the forward heap
// With the packed path we continue as discussed by unpacking it with the cache

//     [  0           1          2         3    ]
//     [ [m0,p=m3],[m1,p=m2],[m2,p=m1], [m3,p=2]]

 //           targets (columns) target_id = column_idx
 //              a   b   c
 //          a  [0,  1,  2],
 // sources  b  [3,  4,  5],
 //  (rows)  c  [6,  7,  8],
 //          d  [9, 10, 11]
 // row_idx * number_of_targets + column_idx
 // a -> c 0 * 3 + 2 = 2
 // c -> c 2 * 3 + 2 = 8
 // d -> c 3 * 3 + 2 = 11

    NodeID middle_node = middle_nodes_table[row_idx];
    const auto &bucket_list = std::equal_range(search_space_with_buckets.begin(),
                                               search_space_with_buckets.end(),
                                               middle_node,
                                               NodeBucket::Compare());

    for (const auto &current_bucket : boost::make_iterator_range(bucket_list))
    {
        // Get target id from bucket entry
        const auto column_idx = current_bucket.column_index;
        const auto target_weight = current_bucket.weight;
        const auto target_duration = current_bucket.duration;

        auto &current_weight = weights_table[row_idx * number_of_targets + column_idx];
        auto &current_duration = durations_table[row_idx * number_of_targets + column_idx];

        // Check if new weight is better
        auto new_weight = source_weight + target_weight;
        auto new_duration = source_duration + target_duration;

        if (new_weight < 0)
        {
            if (addLoopWeight(facade, node, new_weight, new_duration))
            {
                current_weight = std::min(current_weight, new_weight);
                current_duration = std::min(current_duration, new_duration);
            }
        }
        else if (std::tie(new_weight, new_duration) < std::tie(current_weight, current_duration))
        {
            current_weight = new_weight;
            current_duration = new_duration;
        }
    }

    std::vector<NodeID> packed_path = {1,2,3};

    packed_path.emplace_back(target_node);

    std::cout << search_space_with_buckets.size();
    return packed_path;
}

template <>
std::vector<EdgeDuration> manyToManySearch(SearchEngineData<ch::Algorithm> &engine_working_data,
                                           const DataFacade<ch::Algorithm> &facade,
                                           const std::vector<PhantomNode> &phantom_nodes,
                                           const std::vector<std::size_t> &source_indices,
                                           const std::vector<std::size_t> &target_indices)
{
    const auto number_of_sources = source_indices.size();
    const auto number_of_targets = target_indices.size();
    const auto number_of_entries = number_of_sources * number_of_targets;

    std::cout << "Target NodeIDs: ";
    for (std::uint32_t idx = 0; idx < number_of_targets; ++idx)
    {
        std::cout << target_indices[idx] << ", ";
        // std::cout << "duration: " << durations_table[idx] << ", ";
    }
    std::cout << std::endl;

    std::cout << "Source NodeIDs: ";
    for (std::uint32_t idx = 0; idx < number_of_sources; ++idx)
    {
        std::cout << source_indices[idx] << ", ";
        // std::cout << "duration: " << durations_table[idx] << ", ";
    }
    std::cout << std::endl;

    std::vector<EdgeWeight> weights_table(number_of_entries, INVALID_EDGE_WEIGHT);
    std::vector<EdgeDuration> durations_table(number_of_entries, MAXIMAL_EDGE_DURATION);
    std::vector<NodeID> middle_nodes_table(number_of_entries, SPECIAL_NODEID);

    std::vector<NodeBucket> search_space_with_buckets;

    engine_working_data.InitializeOrClearUnpackingCacheThreadLocalStorage();

    // Populate buckets with paths from all accessible nodes to destinations via backward searches
    for (std::uint32_t column_idx = 0; column_idx < target_indices.size(); ++column_idx)
    {
        const auto index = target_indices[column_idx];
        const auto &phantom = phantom_nodes[index];

        engine_working_data.InitializeOrClearManyToManyThreadLocalStorage(
            facade.GetNumberOfNodes());
        auto &query_heap = *(engine_working_data.many_to_many_heap);
        insertTargetInHeap(query_heap, phantom);

        // Explore search space
        while (!query_heap.Empty())
        {
            backwardRoutingStep(facade, column_idx, query_heap, search_space_with_buckets, phantom);
        }
    }

    // Order lookup buckets
    std::sort(search_space_with_buckets.begin(), search_space_with_buckets.end());

    // Find shortest paths from sources to all accessible nodes
    for (std::uint32_t row_idx = 0; row_idx < source_indices.size(); ++row_idx)
    {
        const auto index = source_indices[row_idx];
        const auto &phantom = phantom_nodes[index];

        // Clear heap and insert source nodes
        engine_working_data.InitializeOrClearManyToManyThreadLocalStorage(
            facade.GetNumberOfNodes());
        auto &query_heap = *(engine_working_data.many_to_many_heap);
        insertSourceInHeap(query_heap, phantom);

        // Explore search space
        while (!query_heap.Empty())
        {
            forwardRoutingStep(facade,
                               row_idx,
                               number_of_targets,
                               query_heap,
                               search_space_with_buckets,
                               weights_table,
                               durations_table,
                               middle_nodes_table,
                               phantom);
        }

        std::vector<NodeID> packed_path = retrievePackedPathFromSearchSpace(middle_nodes_table, row_idx, search_space_with_buckets)

 //           targets (columns) target_id = column_idx
 //              a   b   c
 //          a  [0,  1,  2],
 // sources  b  [3,  4,  5],
 //  (rows)  c  [6,  7,  8],
 //          d  [9, 10, 11]
 // row_idx * number_of_targets + column_idx
 // a -> c 0 * 3 + 2 = 2
 // c -> c 2 * 3 + 2 = 8
 // d -> c 3 * 3 + 2 = 11

        // to calculate the durations table: 
        // 1) get the packed path from the the above loops
        // std::vector<NodeID> retrievePackedPathFromSearchSpace(middle_node, target_node, search_space_with_buckets)
        // 2) call unpackPath inside this file, in here
        // void unpackPath(const DataFacade<Algorithm> &facade,
                // BidirectionalIterator packed_path_begin,
                // BidirectionalIterator packed_path_end,
                // Callback &&callback)
        // 3) calculate the duration using the new method that you will write:
        // extractDurations(facade, weight, phantom_nodes, unpacked_nodes, unpacked_edges)

        // std::vector<NodeID> packed_path = retrievePackedPathFromSearchSpace(1, 0, search_space_with_buckets);

        // std::cout << "Packed Path: ";
        // for (std::uint32_t idx = 0; idx < packed_path.size(); ++idx)
        // {
        //     std::cout  << packed_path[idx] << ", ";
        //     std::cout << "duration: " << durations_table[idx] << ", ";
        // }
        // std::cout << std::endl;

        // ch::unpackPath(facade,
        //                packed_leg.begin(),
        //                packed_leg.end(),
        //                *engine_working_data.unpacking_cache.get(),
        //                [&unpacked_nodes, &unpacked_edges](std::pair<NodeID, NodeID> &edge,
        //                                                   const auto &edge_id) {
        //                    BOOST_ASSERT(edge.first == unpacked_nodes.back());
        //                    unpacked_nodes.push_back(edge.second);
        //                    unpacked_edges.push_back(edge_id);
        //                });
        // engine_working_data.unpacking_cache.get()->PrintStats();
    }

    return durations_table;
}



} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

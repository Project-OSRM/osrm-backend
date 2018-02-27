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
            const auto edge_duration = data.duration; 

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
                middle_nodes_table[row_idx * number_of_targets + column_idx] = node;
            }
        }
        else if (std::tie(new_weight, new_duration) < std::tie(current_weight, current_duration))
        {
            current_weight = new_weight;
            current_duration = new_duration;
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

std::vector<NodeID> retrievePackedPathFromSearchSpace(
    const DataFacade<ch::Algorithm> &facade,
    NodeID middle_node, // middle_node,
    const unsigned number_of_targets,
    const unsigned row_idx, // in order to figure out which target is being accessed
    std::vector<NodeBucket> &search_space_with_buckets,
    std::vector<EdgeWeight> &weights_table,
    std::vector<EdgeDuration> &durations_table) {

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

 //   middle_nodes_table = [0 , 1, 2, .........]

    std::cout << "number_of_targets: "<< number_of_targets << " row_idx: "<< row_idx << std::endl;
    std::cout << "weights_table: ";
    for ( std::vector<EdgeWeight>::iterator weight = weights_table.begin(); weight != weights_table.end(); weight++) std::cout << *weight << " ";
    std::cout << std::endl;
    std::cout << "durationz_table: ";
    for ( std::vector<EdgeDuration>::iterator duration = durations_table.begin(); duration != durations_table.end(); duration++) std::cout << *duration << " ";
    std::cout << std::endl;

    const auto &bucket_list = std::equal_range(search_space_with_buckets.begin(),
                                               search_space_with_buckets.end(),
                                               middle_node,
                                               NodeBucket::Compare());

    EdgeWeight shortest_path_weight = INVALID_EDGE_WEIGHT;
    EdgeDuration shortest_path_duration = MAXIMAL_EDGE_DURATION;
    std::vector<NodeID> shortest_packed_path;
    //inside the loop, do a check to see if the current path is better
    // for (const auto &cb : boost::make_iterator_range(bucket_list))
    for (std::vector<NodeBucket>::iterator cb = bucket_list.first; cb <= bucket_list.second; cb++)
    {
        EdgeWeight current_path_weight = 0;
        EdgeDuration current_path_duration = 0;
        std::vector<NodeID> current_packed_path = { cb->middle_node };
        auto pb = cb;
        std::cout << "--------------MIDDLE NODE " << pb->middle_node <<" INDEX " << pb - bucket_list.first << "--------------" << std::endl;
        do {
            std::cout << "pb->middle_node (" << pb->middle_node << ") != pb->parent_node (" << pb->parent_node <<") \n";
            // Get target id from bucket entry
            const auto column_idx = pb->column_index;
            const auto target_weight = pb->weight;
            const auto target_duration = pb->duration;
            // std::cout << "target_weight = " << target_weight << ", target_duration = " << target_duration <<" \n";

            auto &current_weight = weights_table[row_idx * number_of_targets + column_idx];
            auto &current_duration = durations_table[row_idx * number_of_targets + column_idx];
            // Check if new weight is better
            auto new_weight = current_path_weight + target_weight;  // in the original this is the source_weight + target_weight that is taken off from the heap. But, here we are using current_path_weight and it is not serving well because it is causing the test cases to fail. the total weight isn't adding up correctly but. Why not? What am I missing here?
            auto new_duration = current_path_duration + target_duration;

            if (new_weight < 0)
            {
                if (addLoopWeight(facade, pb->middle_node, new_weight, new_duration))
                {
                    current_weight = std::min(current_weight, new_weight);
                    current_path_duration = std::min(current_path_duration, new_duration);
                }
            }
            else if (std::tie(new_weight, new_duration) < std::tie(current_weight, current_duration))
            {
                current_weight = new_weight;
                current_duration = new_duration;
            }
            current_path_weight = current_weight;
            current_path_duration = current_duration;
            current_packed_path.emplace_back(pb->parent_node);
            pb = std::find_if(search_space_with_buckets.begin(), search_space_with_buckets.end(), [pb](const auto &b){return pb->parent_node == b.middle_node;});
        } while (pb->parent_node != pb->middle_node && pb != search_space_with_buckets.end());

        // std::cout << " Packed Path for NodeBucket { middle_node: " << cb->middle_node << " "
        //     << " parent_node: " << cb->parent_node << " "
        //     << " column_index: " << cb->column_index << " "
        //     << " weight: " << cb->weight << " " 
        //     << " duration: " << cb->duration << " }: ";
        // for (unsigned idx = 0; idx < current_packed_path.size(); ++idx) std::cout << current_packed_path[idx] << ", ";
        // std::cout << std::endl;
        std::cout << "current_path_weight: " << current_path_weight << " current_path_duration: " << current_path_duration<< std::endl;

        if (shortest_path_weight > current_path_weight) {
            shortest_path_weight = current_path_weight;
            shortest_path_duration = current_path_duration;
            shortest_packed_path.clear();
            for (unsigned idx = 0; idx < current_packed_path.size(); ++idx) shortest_packed_path.emplace_back(current_packed_path[idx]);
        }
        std::cout << "Shortest Packed Path: ";
        for (unsigned idx = 0; idx < shortest_packed_path.size(); ++idx) std::cout << shortest_packed_path[idx] << ", ";
        std::cout << std::endl;
        std::cout << "shortest_path_weight: " << shortest_path_weight << " shortest_path_duration: " << shortest_path_duration << std::endl;
        std::cout << "--------------END MIDDLE NODE " << cb->middle_node <<" INDEX " << cb - bucket_list.first << "--------------" <<std::endl;
    }

    
    // std::cout << " Packed Path: ";
    // for (unsigned idx = 0; idx < shortest_packed_path.size(); ++idx) std::cout << shortest_packed_path[idx] << ", ";
    // std::cout << std::endl;

    return shortest_packed_path;
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

    std::cout << "search_space_with_buckets:" << std::endl; 
    for (std::vector<NodeBucket>::iterator bucket = search_space_with_buckets.begin(); bucket != search_space_with_buckets.end(); bucket++) {
         std::cout << "NodeBucket { middle_node: " << bucket->middle_node << " "
            << " parent_node: " << bucket->parent_node << " "
            << " column_index: " << bucket->column_index << " "
            << " weight: " << bucket->weight << " " 
            << " duration: " << bucket->duration << " }\n";
        }

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
        // row_idx == one source
        // target == search_space_with_buckets.column_idx

        for (unsigned column_idx = 0; column_idx < number_of_targets; ++column_idx){
            NodeID middle_node = middle_nodes_table[row_idx * number_of_targets + column_idx];
            std::cout << "Passing in " << middle_node << " as middle_node to retrievePackedPathFromSearchSpace: " << std::endl;
            std::vector<NodeID> packed_path_from_source_to_middle = retrievePackedPathFromSearchSpace(
                facade, middle_node, number_of_targets, row_idx, search_space_with_buckets, weights_table, durations_table);

            std::cout << "------------------------------------------------------------------------------------" << std::endl;
            // store packed_path_from_source_to_middle
        }

        // find packed_path_from_middle_to_target (using retrievePackedPathFromSingleHeap using the many_to_many_heap that we have up there for backwardRoutingStep )

        // join packed_path_from_source_to_middle and packed_path_from_middle_to_target to make packed_path

        // unpack packed_path (ch::unpackPath())

        // calculate the duration and fill in the durations table

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

    }

    return durations_table;
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

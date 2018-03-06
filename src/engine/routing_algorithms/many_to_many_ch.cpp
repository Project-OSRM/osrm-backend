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
    search_space_with_buckets.emplace_back(
        node, parent, column_idx, target_weight, target_duration);

    relaxOutgoingEdges<REVERSE_DIRECTION>(
        facade, node, target_weight, target_duration, query_heap, phantom_node);
}

} // namespace ch

std::vector<NodeID>
retrievePackedPathFromSearchSpace(NodeID middle_node,
                                  const unsigned column_idx,
                                  std::vector<NodeBucket> &search_space_with_buckets)
{

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

    auto bucket_list = std::equal_range(search_space_with_buckets.begin(),
                                        search_space_with_buckets.end(),
                                        middle_node,
                                        NodeBucket::ColumnCompare(column_idx));

    std::vector<NodeID> packed_leg = {bucket_list.first->middle_node};
    while (bucket_list.first->parent_node != bucket_list.first->middle_node &&
           bucket_list.first != search_space_with_buckets.end())
    {

        packed_leg.emplace_back(bucket_list.first->parent_node);
        bucket_list = std::equal_range(search_space_with_buckets.begin(),
                                       search_space_with_buckets.end(),
                                       bucket_list.first->parent_node,
                                       NodeBucket::ColumnCompare(column_idx));
    }

    return packed_leg;
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
    for (std::vector<NodeBucket>::iterator bucket = search_space_with_buckets.begin();
         bucket != search_space_with_buckets.end();
         bucket++)
    {
        std::cout << "NodeBucket { middle_node: " << bucket->middle_node << " "
                  << " parent_node: " << bucket->parent_node << " "
                  << " column_index: " << bucket->column_index << " "
                  << " weight: " << bucket->weight << " "
                  << " duration: " << bucket->duration << " }\n";
    }

    // Find shortest paths from sources to all accessible nodes
    for (std::uint32_t row_idx = 0; row_idx < source_indices.size(); ++row_idx)
    {
        const auto source_index = source_indices[row_idx];
        const auto &source_phantom = phantom_nodes[source_index];

        // Clear heap and insert source nodes
        engine_working_data.InitializeOrClearManyToManyThreadLocalStorage(
            facade.GetNumberOfNodes());
        auto &query_heap = *(engine_working_data.many_to_many_heap);
        insertSourceInHeap(query_heap, source_phantom);

        // Explore search spacekewl thanks
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
                               source_phantom);
        }
        // row_idx == one source
        // target == search_space_with_buckets.column_idx

        for (unsigned column_idx = 0; column_idx < number_of_targets; ++column_idx)
        {
            auto target_index = target_indices[column_idx];

            if (source_index == target_index)
            {
                durations_table[row_idx * number_of_targets + column_idx] = 0;
                continue;
            }

            const auto &target_phantom = phantom_nodes[target_indices[column_idx]];
            std::cout << "source -- f: " << source_phantom.forward_segment_id.id
                      << " b: " << source_phantom.reverse_segment_id.id << std::endl;
            std::cout << "target -- f: " << target_phantom.forward_segment_id.id
                      << " b: " << target_phantom.reverse_segment_id.id << std::endl;
            NodeID middle_node_id = middle_nodes_table[row_idx * number_of_targets + column_idx];

            // Step 1: Find path from source to middle node
            std::vector<NodeID> packed_leg_from_source_to_middle;
            ch::retrievePackedPathFromSingleManyToManyHeap(
                query_heap,
                middle_node_id,
                packed_leg_from_source_to_middle); // packed_leg_from_source_to_middle
            std::cout << "packed_leg_from_source_to_middle: ";
            for (unsigned idx = 0; idx < packed_leg_from_source_to_middle.size(); ++idx)
                std::cout << packed_leg_from_source_to_middle[idx] << ", ";
            std::cout << std::endl;

            // Step 2: Find path from middle to target node
            std::vector<NodeID> packed_leg_from_middle_to_target =
                retrievePackedPathFromSearchSpace(
                    middle_node_id,
                    column_idx,
                    search_space_with_buckets); // packed_leg_from_middle_to_target
            std::cout << "packed_leg_from_middle_to_target: ";
            for (unsigned idx = 0; idx < packed_leg_from_middle_to_target.size(); ++idx)
                std::cout << packed_leg_from_middle_to_target[idx] << ", ";
            std::cout << std::endl;

            // Step 3: Join them together
            std::vector<NodeID> packed_leg;
            packed_leg.reserve(packed_leg_from_source_to_middle.size() +
                               packed_leg_from_middle_to_target.size());
            packed_leg.insert(packed_leg.end(),
                              packed_leg_from_source_to_middle.begin(),
                              packed_leg_from_source_to_middle.end());
            packed_leg.insert(packed_leg.end(),
                              packed_leg_from_middle_to_target.begin(),
                              packed_leg_from_middle_to_target.end());
            std::cout << "packed_leg: ";
            for (unsigned idx = 0; idx < packed_leg.size(); ++idx)
                std::cout << packed_leg[idx] << ", ";
            std::cout << std::endl;

            // Step 4: Unpack the pack_path. Modify the unpackPath method to also calculate duration
            // as it unpacks the path.

            // if (!packed_leg.empty())
            // {
            //     durations_table[row_idx * number_of_targets + column_idx] =
            //     ch::calculateEBGNodeAnnotations(facade,
            //                    packed_leg.begin(),
            //                    packed_leg.end(),
            //                    *engine_working_data.unpacking_cache.get());
            // }

            // Step 4: Unpack the pack_path
            std::vector<NodeID> unpacked_nodes;
            std::vector<EdgeID> unpacked_edges;
            if (!packed_leg.empty())
            {
                unpacked_nodes.reserve(packed_leg.size());
                unpacked_edges.reserve(packed_leg.size());
                unpacked_nodes.push_back(packed_leg.front());

                ch::unpackPath(facade,
                               packed_leg.begin(),
                               packed_leg.end(),
                               [&unpacked_nodes, &unpacked_edges](std::pair<NodeID, NodeID> &edge,
                                                                  const auto &edge_id) {
                                   BOOST_ASSERT(edge.first == unpacked_nodes.back());
                                   unpacked_nodes.push_back(edge.second);
                                   unpacked_edges.push_back(edge_id);
                               });
            }
            std::cout << "unpacked_nodes: ";
            for (unsigned idx = 0; idx < unpacked_nodes.size(); ++idx)
                std::cout << unpacked_nodes[idx] << ", ";
            std::cout << std::endl;

            std::cout << "unpacked_edges: ";
            for (unsigned idx = 0; idx < unpacked_edges.size(); ++idx)
                std::cout << unpacked_edges[idx] << ", ";
            std::cout << std::endl;
            std::cout << std::endl;

            PhantomNodes phantom_nodes_pair{source_phantom, target_phantom};

            // Step 5: Use modified function to calculate durations of path (modelled after
            // extractRoute)
            InternalDurationsRouteResult result =
                extractDurations(facade,
                                 weights_table[row_idx * number_of_targets + column_idx],
                                 phantom_nodes_pair,
                                 unpacked_nodes,
                                 unpacked_edges);

            durations_table[row_idx * number_of_targets + column_idx] = result.duration();

            std::cout << "path duration: " << result.duration() << std::endl;
            // TAKE CARE OF THE START AND TARGET OFFSET FOR WHEN source_index == target_index
        }

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

#include "engine/routing_algorithms/many_to_many.hpp"
#include "engine/routing_algorithms/routing_base_ch.hpp"

#include <boost/assert.hpp>

#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

namespace
{
struct NodeBucket
{
    unsigned target_id; // essentially a row in the weight matrix
    EdgeWeight weight;
    EdgeWeight duration;
    NodeBucket(const unsigned target_id, const EdgeWeight weight, const EdgeWeight duration)
        : target_id(target_id), weight(weight), duration(duration)
    {
    }
};

// FIXME This should be replaced by an std::unordered_multimap, though this needs benchmarking
using SearchSpaceWithBuckets = std::unordered_map<NodeID, std::vector<NodeBucket>>;

inline bool
addLoopWeight(const datafacade::ContiguousInternalMemoryDataFacade<ch::Algorithm> &facade,
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
void relaxOutgoingEdges(const datafacade::ContiguousInternalMemoryDataFacade<ch::Algorithm> &facade,
                        const NodeID node,
                        const EdgeWeight weight,
                        const EdgeDuration duration,
                        typename SearchEngineData<ch::Algorithm>::ManyToManyQueryHeap &query_heap,
                        const PhantomNode &)
{
    if (ch::stallAtNode<DIRECTION>(facade, node, weight, query_heap))
    {
        return;
    }

    for (auto edge : facade.GetAdjacentEdgeRange(node))
    {
        const auto &data = facade.GetEdgeData(edge);
        if (DIRECTION == FORWARD_DIRECTION ? data.forward : data.backward)
        {
            const NodeID to = facade.GetTarget(edge);
            const EdgeWeight edge_weight = data.weight;
            const EdgeWeight edge_duration = data.duration;

            BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
            const EdgeWeight to_weight = weight + edge_weight;
            const EdgeWeight to_duration = duration + edge_duration;

            // New Node discovered -> Add to Heap + Node Info Storage
            if (!query_heap.WasInserted(to))
            {
                query_heap.Insert(to, to_weight, {node, to_duration});
            }
            // Found a shorter Path -> Update weight
            else if (to_weight < query_heap.GetKey(to))
            {
                // new parent
                query_heap.GetData(to) = {node, to_duration};
                query_heap.DecreaseKey(to, to_weight);
            }
        }
    }
}

inline bool addLoopWeight(const datafacade::ContiguousInternalMemoryDataFacade<mld::Algorithm> &,
                          const NodeID,
                          EdgeWeight &,
                          EdgeDuration &)
{ // MLD overlay does not introduce loop edges
    return false;
}

template <bool DIRECTION>
void relaxOutgoingEdges(
    const datafacade::ContiguousInternalMemoryDataFacade<mld::Algorithm> &facade,
    const NodeID node,
    const EdgeWeight weight,
    const EdgeDuration duration,
    typename SearchEngineData<mld::Algorithm>::ManyToManyQueryHeap &query_heap,
    const PhantomNode &phantom_node)
{
    const auto &partition = facade.GetMultiLevelPartition();
    const auto &cells = facade.GetCellStorage();

    auto highest_diffrent_level = [&partition, node](const SegmentID &phantom_node) {
        if (phantom_node.enabled)
            return partition.GetHighestDifferentLevel(phantom_node.id, node);
        return INVALID_LEVEL_ID;
    };
    const auto level = std::min(highest_diffrent_level(phantom_node.forward_segment_id),
                                highest_diffrent_level(phantom_node.reverse_segment_id));

    const auto &node_data = query_heap.GetData(node);

    if (level >= 1 && !node_data.from_clique_arc)
    {
        const auto &cell = cells.GetCell(level, partition.GetCell(level, node));
        if (DIRECTION == FORWARD_DIRECTION)
        { // Shortcuts in forward direction
            auto destination = cell.GetDestinationNodes().begin();
            auto shortcut_durations = cell.GetOutDuration(node);
            for (auto shortcut_weight : cell.GetOutWeight(node))
            {
                BOOST_ASSERT(destination != cell.GetDestinationNodes().end());
                BOOST_ASSERT(!shortcut_durations.empty());
                const NodeID to = *destination;
                if (shortcut_weight != INVALID_EDGE_WEIGHT && node != to)
                {
                    const auto to_weight = weight + shortcut_weight;
                    const auto to_duration = duration + shortcut_durations.front();
                    if (!query_heap.WasInserted(to))
                    {
                        query_heap.Insert(to, to_weight, {node, true, to_duration});
                    }
                    else if (to_weight < query_heap.GetKey(to))
                    {
                        query_heap.GetData(to) = {node, true, to_duration};
                        query_heap.DecreaseKey(to, to_weight);
                    }
                }
                ++destination;
                shortcut_durations.advance_begin(1);
            }
            BOOST_ASSERT(shortcut_durations.empty());
        }
        else
        { // Shortcuts in backward direction
            auto source = cell.GetSourceNodes().begin();
            auto shortcut_durations = cell.GetInDuration(node);
            for (auto shortcut_weight : cell.GetInWeight(node))
            {
                BOOST_ASSERT(source != cell.GetSourceNodes().end());
                BOOST_ASSERT(!shortcut_durations.empty());
                const NodeID to = *source;
                if (shortcut_weight != INVALID_EDGE_WEIGHT && node != to)
                {
                    const auto to_weight = weight + shortcut_weight;
                    const auto to_duration = duration + shortcut_durations.front();
                    if (!query_heap.WasInserted(to))
                    {
                        query_heap.Insert(to, to_weight, {node, true, to_duration});
                    }
                    else if (to_weight < query_heap.GetKey(to))
                    {
                        query_heap.GetData(to) = {node, true, to_duration};
                        query_heap.DecreaseKey(to, to_weight);
                    }
                }
                ++source;
                shortcut_durations.advance_begin(1);
            }
            BOOST_ASSERT(shortcut_durations.empty());
        }
    }

    for (const auto edge : facade.GetBorderEdgeRange(level, node))
    {
        const auto &data = facade.GetEdgeData(edge);
        if (DIRECTION == FORWARD_DIRECTION ? data.forward : data.backward)
        {
            const NodeID to = facade.GetTarget(edge);
            const EdgeWeight edge_weight = data.weight;
            const EdgeWeight edge_duration = data.duration;

            BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
            const EdgeWeight to_weight = weight + edge_weight;
            const EdgeWeight to_duration = duration + edge_duration;

            // New Node discovered -> Add to Heap + Node Info Storage
            if (!query_heap.WasInserted(to))
            {
                query_heap.Insert(to, to_weight, {node, false, to_duration});
            }
            // Found a shorter Path -> Update weight
            else if (to_weight < query_heap.GetKey(to))
            {
                // new parent
                query_heap.GetData(to) = {node, false, to_duration};
                query_heap.DecreaseKey(to, to_weight);
            }
        }
    }
}

template <typename Algorithm>
void forwardRoutingStep(const datafacade::ContiguousInternalMemoryDataFacade<Algorithm> &facade,
                        const unsigned row_idx,
                        const unsigned number_of_targets,
                        typename SearchEngineData<Algorithm>::ManyToManyQueryHeap &query_heap,
                        const SearchSpaceWithBuckets &search_space_with_buckets,
                        std::vector<EdgeWeight> &weights_table,
                        std::vector<EdgeWeight> &durations_table,
                        const PhantomNode &phantom_node)
{
    const NodeID node = query_heap.DeleteMin();
    const EdgeWeight source_weight = query_heap.GetKey(node);
    const EdgeWeight source_duration = query_heap.GetData(node).duration;

    // check if each encountered node has an entry
    const auto bucket_iterator = search_space_with_buckets.find(node);
    // iterate bucket if there exists one
    if (bucket_iterator != search_space_with_buckets.end())
    {
        const std::vector<NodeBucket> &bucket_list = bucket_iterator->second;
        for (const NodeBucket &current_bucket : bucket_list)
        {
            // get target id from bucket entry
            const unsigned column_idx = current_bucket.target_id;
            const EdgeWeight target_weight = current_bucket.weight;
            const EdgeWeight target_duration = current_bucket.duration;

            auto &current_weight = weights_table[row_idx * number_of_targets + column_idx];
            auto &current_duration = durations_table[row_idx * number_of_targets + column_idx];

            // check if new weight is better
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
            else if (new_weight < current_weight)
            {
                current_weight = new_weight;
                current_duration = new_duration;
            }
        }
    }

    relaxOutgoingEdges<FORWARD_DIRECTION>(
        facade, node, source_weight, source_duration, query_heap, phantom_node);
}

template <typename Algorithm>
void backwardRoutingStep(const datafacade::ContiguousInternalMemoryDataFacade<Algorithm> &facade,
                         const unsigned column_idx,
                         typename SearchEngineData<Algorithm>::ManyToManyQueryHeap &query_heap,
                         SearchSpaceWithBuckets &search_space_with_buckets,
                         const PhantomNode &phantom_node)
{
    const NodeID node = query_heap.DeleteMin();
    const EdgeWeight target_weight = query_heap.GetKey(node);
    const EdgeWeight target_duration = query_heap.GetData(node).duration;

    // store settled nodes in search space bucket
    search_space_with_buckets[node].emplace_back(column_idx, target_weight, target_duration);

    relaxOutgoingEdges<REVERSE_DIRECTION>(
        facade, node, target_weight, target_duration, query_heap, phantom_node);
}
}

template <typename Algorithm>
std::vector<EdgeWeight>
manyToManySearch(SearchEngineData<Algorithm> &engine_working_data,
                 const datafacade::ContiguousInternalMemoryDataFacade<Algorithm> &facade,
                 const std::vector<PhantomNode> &phantom_nodes,
                 const std::vector<std::size_t> &source_indices,
                 const std::vector<std::size_t> &target_indices)
{
    const auto number_of_sources =
        source_indices.empty() ? phantom_nodes.size() : source_indices.size();
    const auto number_of_targets =
        target_indices.empty() ? phantom_nodes.size() : target_indices.size();
    const auto number_of_entries = number_of_sources * number_of_targets;

    std::vector<EdgeWeight> weights_table(number_of_entries, INVALID_EDGE_WEIGHT);
    std::vector<EdgeWeight> durations_table(number_of_entries, MAXIMAL_EDGE_DURATION);

    engine_working_data.InitializeOrClearManyToManyThreadLocalStorage(facade.GetNumberOfNodes());

    auto &query_heap = *(engine_working_data.many_to_many_heap);

    SearchSpaceWithBuckets search_space_with_buckets;

    unsigned column_idx = 0;
    const auto search_target_phantom = [&](const PhantomNode &phantom) {
        // clear heap and insert target nodes
        query_heap.Clear();
        insertTargetInHeap(query_heap, phantom);

        // explore search space
        while (!query_heap.Empty())
        {
            backwardRoutingStep(facade, column_idx, query_heap, search_space_with_buckets, phantom);
        }
        ++column_idx;
    };

    // for each source do forward search
    unsigned row_idx = 0;
    const auto search_source_phantom = [&](const PhantomNode &phantom) {
        // clear heap and insert source nodes
        query_heap.Clear();
        insertSourceInHeap(query_heap, phantom);

        // explore search space
        while (!query_heap.Empty())
        {
            forwardRoutingStep(facade,
                               row_idx,
                               number_of_targets,
                               query_heap,
                               search_space_with_buckets,
                               weights_table,
                               durations_table,
                               phantom);
        }
        ++row_idx;
    };

    if (target_indices.empty())
    {
        for (const auto &phantom : phantom_nodes)
        {
            search_target_phantom(phantom);
        }
    }
    else
    {
        for (const auto index : target_indices)
        {
            const auto &phantom = phantom_nodes[index];
            search_target_phantom(phantom);
        }
    }

    if (source_indices.empty())
    {
        for (const auto &phantom : phantom_nodes)
        {
            search_source_phantom(phantom);
        }
    }
    else
    {
        for (const auto index : source_indices)
        {
            const auto &phantom = phantom_nodes[index];
            search_source_phantom(phantom);
        }
    }

    return durations_table;
}

template <typename QueryHeap>
void extractRow(const std::size_t offset,
                QueryHeap &query_heap,
                const std::vector<std::size_t> &target_indices,
                const std::vector<PhantomNode> &phantom_nodes,
                std::vector<EdgeWeight> &durations_table,
                std::vector<EdgeWeight> &weights_table)
{
    std::size_t column_idx = 0;
    const auto extract_phantom = [&](const auto &phantom) {
        auto &weight_entry = weights_table[offset + column_idx];
        if (phantom.forward_segment_id.enabled && phantom.reverse_segment_id.enabled)
        {
            weight_entry = std::min(query_heap.GetKey(phantom.forward_segment_id.id),
                                    query_heap.GetKey(phantom.reverse_segment_id.id));
        }
        else if (phantom.forward_segment_id.enabled)
        {
            weight_entry = query_heap.GetKey(phantom.forward_segment_id.id);
        }
        else if (phantom.reverse_segment_id.enabled)
        {
            weight_entry = query_heap.GetKey(phantom.reverse_segment_id.id);
        }
    };

    if (target_indices.empty())
    {
        for (const auto &phantom : phantom_nodes)
        {
            extract_phantom(phantom);
        }
    }
    else
    {
        for (const auto index : target_indices)
        {
            const auto &phantom = phantom_nodes[index];
            extract_phantom(phantom);
        }
    }
}
}

std::vector<EdgeWeight>
manyToManySearch(SearchEngineData<mld::Algorithm> &engine_working_data,
                 const datafacade::ContiguousInternalMemoryDataFacade<mld::Algorithm> &facade,
                 const std::vector<PhantomNode> &phantom_nodes,
                 const std::vector<std::size_t> &source_indices,
                 const std::vector<std::size_t> &target_indices)
{
    const auto number_of_sources =
        source_indices.empty() ? phantom_nodes.size() : source_indices.size();
    const auto number_of_targets =
        target_indices.empty() ? phantom_nodes.size() : target_indices.size();
    const auto number_of_entries = number_of_sources * number_of_targets;

    const auto &grasp_storage = facade.GetGraspStorage();

    std::vector<EdgeWeight> weights_table(number_of_entries, INVALID_EDGE_WEIGHT);
    std::vector<EdgeWeight> durations_table(number_of_entries, MAXIMAL_EDGE_DURATION);
    std::vector<bool> is_target(facade.GetNumberOfNodes(), false);

    const auto mark_target = [&](const PhantomNode& phantom) {
        if (phantom.forward_segment_id.enabled)
            is_target[phantom.forward_segment_id.id] = true;
        if (phantom.reverse_segment_id.enabled)
            is_target[phantom.reverse_segment_id.id] = true;
    };

    if (target_indices.empty())
    {
        for (const auto &phantom : phantom_nodes)
        {
            mark_target(phantom);
        }
    }
    else
    {
        for (const auto index : target_indices)
        {
            const auto &phantom = phantom_nodes[index];
            mark_target(phantom);
        }
    }

    for (const auto node : util::irange<NodeID>(0, facade.GetNumberOfNodes()))
    {
        if (is_target[node])
        {
            for (const auto edge : grasp_storage.GetDownwardEdgeRange(node))
            {
                const auto source = grasp_storage.GetSource(edge);
                // we can insure this because of our ID sorting
                // that sortes border vertices of smaller cells later
                BOOST_ASSERT(source < node);
                is_target[source] = true;
            }
        }
    }

    engine_working_data.InitializeOrClearFirstThreadLocalStorage(facade.GetNumberOfNodes());
    auto &query_heap = *(engine_working_data.forward_heap_1);

    const auto& partition = facade.GetMultiLevelPartition();
    const auto& cell_storage = facade.GetCellStorage();

    // for each source do forward search
    unsigned row_idx = 0;
    const auto search_source_phantom = [&](const PhantomNode &phantom) {
        // clear heap and insert source nodes
        query_heap.Clear();
        insertSourceInHeap(query_heap, phantom);

        // explore search space
        while (!query_heap.Empty())
        {
            forwardRoutingStep(facade, partition, cell_storage, phantom, query_heap);
        }

        for (const auto level : boost::range::reverse(util::irange<LevelID>(0, partition.GetNumberOfLevel())))
        {
            for (const auto cell_id : util::irange<CellID>(0, partition.GetNumberOfCells(level)))
            {
                const auto &cell = cell_storage.GetCell(cell_id);

                for (const auto node : cell.GetSourceNodes())
                {
                    const auto &data = query_heap.GetData(node);

                    for (const auto edge : grasp_storage.GetDownwardEdgeRange(node))
                    {
                        auto source = graph_storage.GetSource(edge);
                        const auto &edge_data = grasp_storage.GetEdgeData(edge);
                        auto new_weight = data.weight + edge_data.weight;
                        auto new_duration = data.duration + edge_data.duration;

                        // FIXME update data in heap
                    }
                }
            }
        }

        const auto offset = row_idx * number_of_targets;
        extractRow(
            offset, query_heap, target_indices, phantom_nodes, durations_table, weights_table);
        ++row_idx;
    };

    if (source_indices.empty())
    {
        for (const auto &phantom : phantom_nodes)
        {
            search_source_phantom(phantom);
        }
    }
    else
    {
        for (const auto index : source_indices)
        {
            const auto &phantom = phantom_nodes[index];
            search_source_phantom(phantom);
        }
    }

    return durations_table;
}

template std::vector<EdgeWeight>
manyToManySearch(SearchEngineData<ch::Algorithm> &engine_working_data,
                 const datafacade::ContiguousInternalMemoryDataFacade<ch::Algorithm> &facade,
                 const std::vector<PhantomNode> &phantom_nodes,
                 const std::vector<std::size_t> &source_indices,
                 const std::vector<std::size_t> &target_indices);

//template std::vector<EdgeWeight>
//manyToManySearch(SearchEngineData<mld::Algorithm> &engine_working_data,
//                 const datafacade::ContiguousInternalMemoryDataFacade<mld::Algorithm> &facade,
//                 const std::vector<PhantomNode> &phantom_nodes,
//                 const std::vector<std::size_t> &source_indices,
//                 const std::vector<std::size_t> &target_indices);

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

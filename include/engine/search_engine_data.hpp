#ifndef SEARCH_ENGINE_DATA_HPP
#define SEARCH_ENGINE_DATA_HPP

#include "engine/algorithm.hpp"
#include "engine/concepts.hpp"
#include "util/browse_resistant_cache.hpp"
#include "util/query_heap.hpp"
#include "util/typedefs.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace osrm::engine
{

// Algorithm-dependent heaps
// - CH algorithms use CH heaps
// - MLD algorithms use MLD heaps

template <routing_algorithms::RoutingAlgorithm Algorithm> struct SearchEngineData
{
};

struct HeapData
{
    NodeID parent;
    /* explicit */ HeapData(NodeID p) : parent(p) {}
};

struct ManyToManyHeapData : HeapData
{
    EdgeDuration duration;
    EdgeDistance distance;
    // How much of the weight is the walk into an open area rather than travel through the
    // graph (PhantomNode::approach_weight). Set by the seed and carried unchanged as the
    // search relaxes, because a walk to where the graph starts is not made up by
    // travelling along it.
    //
    // It has to be separable. A source and a target on one edge-based node with the target
    // the earlier of the two need a loop, and the graph part of the weight going negative
    // is what says so. A walk charged at both ends lifts the sum back above zero and hides
    // it, and the cell then comes out short by the stretch in between.
    EdgeWeight approach;
    ManyToManyHeapData(NodeID p,
                       EdgeDuration duration,
                       EdgeDistance distance,
                       EdgeWeight approach = EdgeWeight{0})
        : HeapData(p), duration(duration), distance(distance), approach(approach)
    {
    }
};

template <> struct SearchEngineData<routing_algorithms::ch::Algorithm>
{
    using QueryHeap = util::
        QueryHeap<NodeID, NodeID, EdgeWeight, HeapData, util::UnorderedMapStorage<NodeID, int>>;

    using ManyToManyQueryHeap = util::QueryHeap<NodeID,
                                                NodeID,
                                                EdgeWeight,
                                                ManyToManyHeapData,
                                                util::UnorderedMapStorage<NodeID, int>>;

    using SearchEngineHeapPtr = std::unique_ptr<QueryHeap>;

    using ManyToManyHeapPtr = std::unique_ptr<ManyToManyQueryHeap>;

    static thread_local SearchEngineHeapPtr forward_heap_1;
    static thread_local SearchEngineHeapPtr reverse_heap_1;
    static thread_local SearchEngineHeapPtr forward_heap_2;
    static thread_local SearchEngineHeapPtr reverse_heap_2;
    static thread_local SearchEngineHeapPtr forward_heap_3;
    static thread_local SearchEngineHeapPtr reverse_heap_3;
    static thread_local ManyToManyHeapPtr many_to_many_heap;
    static thread_local SearchEngineHeapPtr map_matching_forward_heap_1;
    static thread_local SearchEngineHeapPtr map_matching_reverse_heap_1;

    void InitializeOrClearMapMatchingThreadLocalStorage(unsigned number_of_nodes);

    void InitializeOrClearFirstThreadLocalStorage(unsigned number_of_nodes);

    void InitializeOrClearSecondThreadLocalStorage(unsigned number_of_nodes);

    void InitializeOrClearThirdThreadLocalStorage(unsigned number_of_nodes);

    void InitializeOrClearManyToManyThreadLocalStorage(unsigned number_of_nodes);
};

struct MultiLayerDijkstraHeapData
{
    NodeID parent;
    bool from_clique_arc;
    MultiLayerDijkstraHeapData(NodeID p) : parent(p), from_clique_arc(false) {}
    MultiLayerDijkstraHeapData(NodeID p, bool from) : parent(p), from_clique_arc(from) {}
};

struct MapMatchingMultiLayerDijkstraHeapData
{
    NodeID parent;
    bool from_clique_arc;
    EdgeDistance distance = {0};
    MapMatchingMultiLayerDijkstraHeapData(NodeID p) : parent(p), from_clique_arc(false) {}
    MapMatchingMultiLayerDijkstraHeapData(NodeID p, bool from) : parent(p), from_clique_arc(from) {}
    MapMatchingMultiLayerDijkstraHeapData(NodeID p, bool from, EdgeDistance d)
        : parent(p), from_clique_arc(from), distance(d)
    {
    }
};

struct ManyToManyMultiLayerDijkstraHeapData : MultiLayerDijkstraHeapData
{
    EdgeDuration duration;
    EdgeDistance distance;
    //! See ManyToManyHeapData::approach.
    EdgeWeight approach;
    ManyToManyMultiLayerDijkstraHeapData(NodeID p,
                                         EdgeDuration duration,
                                         EdgeDistance distance,
                                         EdgeWeight approach = EdgeWeight{0})
        : MultiLayerDijkstraHeapData(p), duration(duration), distance(distance), approach(approach)
    {
    }
    ManyToManyMultiLayerDijkstraHeapData(NodeID p,
                                         bool from,
                                         EdgeDuration duration,
                                         EdgeDistance distance,
                                         EdgeWeight approach = EdgeWeight{0})
        : MultiLayerDijkstraHeapData(p, from), duration(duration), distance(distance),
          approach(approach)
    {
    }
};

struct MLDUnpackingCacheKey
{
    NodeID source;
    NodeID target;
    LevelID level;
    CellID cell_id;

    bool operator==(const MLDUnpackingCacheKey &) const = default;
};

struct MLDUnpackingCacheKeyHash
{
    size_t operator()(const MLDUnpackingCacheKey &k) const
    {
        auto h = std::hash<NodeID>{}(k.source);
        h ^= std::hash<NodeID>{}(k.target) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<LevelID>{}(k.level) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<CellID>{}(k.cell_id) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

struct MLDUnpackingCacheValue
{
    std::vector<NodeID> nodes;
    std::vector<EdgeID> edges;
};

struct MLDUnpackingCacheCostFn
{
    size_t operator()(const MLDUnpackingCacheValue &v) const
    {
        return sizeof(MLDUnpackingCacheKey) + sizeof(MLDUnpackingCacheValue) +
               v.nodes.capacity() * sizeof(NodeID) + v.edges.capacity() * sizeof(EdgeID) +
               kPerEntryOverhead;
    }

    static constexpr size_t kPerEntryOverhead = 96;
};

using MLDUnpackingCache = util::BrowseResistantCache<MLDUnpackingCacheKey,
                                                     MLDUnpackingCacheValue,
                                                     MLDUnpackingCacheCostFn,
                                                     MLDUnpackingCacheKeyHash>;

template <> struct SearchEngineData<routing_algorithms::mld::Algorithm>
{
    using QueryHeap = util::QueryHeap<NodeID,
                                      NodeID,
                                      EdgeWeight,
                                      MultiLayerDijkstraHeapData,
                                      util::TwoLevelStorage<NodeID, int>>;

    using ManyToManyQueryHeap = util::QueryHeap<NodeID,
                                                NodeID,
                                                EdgeWeight,
                                                ManyToManyMultiLayerDijkstraHeapData,
                                                util::TwoLevelStorage<NodeID, int>>;
    using MapMatchingQueryHeap = util::QueryHeap<NodeID,
                                                 NodeID,
                                                 EdgeWeight,
                                                 MapMatchingMultiLayerDijkstraHeapData,
                                                 util::TwoLevelStorage<NodeID, int>>;

    using SearchEngineHeapPtr = std::unique_ptr<QueryHeap>;
    using ManyToManyHeapPtr = std::unique_ptr<ManyToManyQueryHeap>;
    using MapMatchingHeapPtr = std::unique_ptr<MapMatchingQueryHeap>;
    using UnpackingCachePtr = std::unique_ptr<MLDUnpackingCache>;

    static thread_local SearchEngineHeapPtr forward_heap_1;
    static thread_local SearchEngineHeapPtr reverse_heap_1;
    static thread_local MapMatchingHeapPtr map_matching_forward_heap_1;
    static thread_local MapMatchingHeapPtr map_matching_reverse_heap_1;

    static thread_local ManyToManyHeapPtr many_to_many_heap;
    static thread_local UnpackingCachePtr unpacking_cache;
    static thread_local unsigned unpacking_cache_node_count;
    static thread_local unsigned unpacking_cache_edge_count;

    void InitializeOrClearFirstThreadLocalStorage(unsigned number_of_nodes,
                                                  unsigned number_of_boundary_nodes);
    void InitializeOrClearMapMatchingThreadLocalStorage(unsigned number_of_nodes,
                                                        unsigned number_of_boundary_nodes);

    void InitializeOrClearManyToManyThreadLocalStorage(unsigned number_of_nodes,
                                                       unsigned number_of_boundary_nodes);

    void InitializeUnpackingCache(unsigned number_of_nodes, unsigned number_of_edges);
};
} // namespace osrm::engine

#endif // SEARCH_ENGINE_DATA_HPP
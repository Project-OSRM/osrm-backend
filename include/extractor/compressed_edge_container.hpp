#ifndef GEOMETRY_COMPRESSOR_HPP_
#define GEOMETRY_COMPRESSOR_HPP_

#include "extractor/geometry_memory.hpp"
#include "extractor/record_spool.hpp"
#include "extractor/segment_data_container.hpp"

#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include <boost/container/options.hpp>
#include <boost/container/vector.hpp>

#include <deque>
#include <filesystem>
#include <limits>
#include <memory>
#include <ranges>
#include <span>
#include <utility>

#include <vector>

namespace osrm::extractor
{

class CompressedEdgeContainer
{
  public:
    struct OnewayCompressedEdge
    {
      public:
        NodeID node_id;           // refers to an internal node-based-node
        SegmentWeight weight;     // the weight of the edge leading to this node
        SegmentDuration duration; // the duration of the edge leading to this node
    };

    using BucketOptions = boost::container::vector_options<
        boost::container::stored_size<unsigned>,
        boost::container::growth_factor<boost::container::growth_factor_50>>::type;
    using OnewayEdgeBucket =
        boost::container::vector<OnewayCompressedEdge,
                                 std::pmr::polymorphic_allocator<OnewayCompressedEdge>,
                                 BucketOptions>;

    explicit CompressedEdgeContainer(std::filesystem::path spool_parent = {});
    void initializeEdgeIndex(std::size_t edge_capacity);
    void moveEdgeMappings(EdgeID source, EdgeID destination, unsigned count);
    void resizeEdgeIndex(std::size_t edge_capacity);
    void CompressEdge(const EdgeID surviving_edge_id,
                      const EdgeID removed_edge_id,
                      const NodeID via_node_id,
                      const NodeID target_node,
                      const EdgeWeight weight1,
                      const EdgeWeight weight2,
                      const EdgeDuration duration1,
                      const EdgeDuration duration2,
                      // node-penalties can be added before/or after the traversal of an edge which
                      // depends on whether we traverse the link forwards or backwards.
                      const EdgeWeight node_weight_penalty = INVALID_EDGE_WEIGHT,
                      const EdgeDuration node_duration_penalty = MAXIMAL_EDGE_DURATION);

    unsigned ZipEdges(const util::NodeBasedDynamicGraph &graph, EdgeID forward, EdgeID reverse);
    void releaseEquivalentEdgeGeometry(EdgeID forward, EdgeID reverse);
    bool finalizeGeometry(const util::NodeBasedDynamicGraph &graph);

    auto getGeometryNodes(EdgeID edge, GeometryID geometry, NodeID target = SPECIAL_NODEID) const
    {
        std::span<const OnewayCompressedEdge> bucket;
        std::span<const NodeID> packed;
        const auto bucket_index = findBucket(edge);
        if ((!m_use_packed_geometry || m_has_fallback_geometry) && bucket_index != PackedGeometry)
        {
            if (m_geometry_buckets && bucket_index < m_geometry_buckets->buckets.size())
            {
                const auto &stored = m_geometry_buckets->buckets[bucket_index];
                bucket = {stored.data(), stored.size()};
            }
        }
        else if (m_use_packed_geometry)
        {
            if (geometry.id != SPECIAL_GEOMETRYID)
                packed = std::as_const(*segment_data).GetForwardGeometry(geometry.id);
        }

        const auto uncompressed =
            !m_use_packed_geometry && bucket.empty() && target != SPECIAL_NODEID;
        const auto size =
            packed.empty() ? bucket.size() + unsigned(uncompressed) : packed.size() - 1;
        return std::views::iota(std::size_t{0}, size) |
               std::views::transform(
                   [bucket, packed, target, forward = bool(geometry.forward)](auto i)
                   {
                       if (packed.empty())
                           return bucket.empty() ? target : bucket[i].node_id;
                       return packed[forward ? i + 1 : packed.size() - 2 - i];
                   });
    }

    bool HasEntryForID(const EdgeID edge_id) const;
    void PrintStatistics(unsigned live_edges) const;
    unsigned GetPositionForID(const EdgeID edge_id) const;
    const OnewayEdgeBucket &GetBucketReference(const EdgeID edge_id) const;
    bool IsTrivial(const EdgeID edge_id) const;
    NodeID GetFirstEdgeTargetID(const EdgeID edge_id) const;
    NodeID GetLastEdgeTargetID(const EdgeID edge_id) const;
    NodeID GetLastEdgeSourceID(const EdgeID edge_id) const;

    void writeGeometryAndReleaseCosts(const std::filesystem::path &path);
    void releaseGeometry();

  private:
    SegmentWeight ClipWeight(const EdgeWeight weight);
    SegmentDuration ClipDuration(const EdgeDuration duration);

    static constexpr unsigned NoBucket = std::numeric_limits<unsigned>::max();
    static constexpr unsigned PackedGeometry = NoBucket - 1;
    unsigned findBucket(EdgeID edge) const
    { return edge < m_edge_id_to_list_index.size() ? m_edge_id_to_list_index[edge] : NoBucket; }
    void assignBucket(EdgeID edge);

    int free_list_maximum = 0;
    std::atomic_size_t clipped_weights{0};
    std::atomic_size_t clipped_durations{0};

    struct GeometryRecord
    {
        NodeID node;
        SegmentWeight forward_weight;
        SegmentWeight reverse_weight;
        SegmentDuration forward_duration;
        SegmentDuration reverse_duration;
    };

    void IncreaseFreeList();
    void releaseGeometryBuckets();
    void loadPackedGeometry();

    struct GeometryBuckets
    {
        GeometryMemory memory;
        std::pmr::deque<OnewayEdgeBucket> buckets{&memory};
    };

    RecordSpool<GeometryRecord> m_geometry_records;
    std::unique_ptr<GeometryBuckets> m_geometry_buckets;
    std::vector<unsigned> m_free_list;
    util::DeallocatingVector<unsigned> m_edge_id_to_list_index;
    std::unique_ptr<SegmentDataContainer> segment_data;
    bool m_use_packed_geometry = false;
    bool m_has_fallback_geometry = false;
};
} // namespace osrm::extractor

#endif // GEOMETRY_COMPRESSOR_HPP_

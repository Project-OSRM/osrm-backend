#ifndef GEOMETRY_COMPRESSOR_HPP_
#define GEOMETRY_COMPRESSOR_HPP_

#include "extractor/segment_data_container.hpp"

#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include <memory>
#include <ranges>
#include <span>
#include <unordered_map>
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

    using OnewayEdgeBucket = std::vector<OnewayCompressedEdge>;

    CompressedEdgeContainer();
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

    void AddUncompressedEdge(const EdgeID edge_id,
                             const NodeID target_node,
                             const EdgeWeight weight,
                             const EdgeDuration duration);

    void InitializeBothwayVector();
    unsigned ZipEdges(const unsigned f_edge_pos, const unsigned r_edge_pos);
    void finalizeGeometry(const util::NodeBasedDynamicGraph &graph);

    auto getGeometryNodes(EdgeID edge, GeometryID geometry) const
    {
        std::span<const OnewayCompressedEdge> bucket;
        std::span<const NodeID> packed;
        if (m_use_packed_geometry)
        {
            if (geometry.id != SPECIAL_GEOMETRYID)
                packed = std::as_const(*segment_data).GetForwardGeometry(geometry.id);
        }
        else if (const auto it = m_edge_id_to_list_index_map.find(edge);
                 it != m_edge_id_to_list_index_map.end())
        {
            bucket = m_compressed_oneway_geometries.at(it->second);
        }

        const auto size = packed.empty() ? bucket.size() : packed.size() - 1;
        return std::views::iota(std::size_t{0}, size) |
               std::views::transform(
                   [bucket, packed, forward = bool(geometry.forward)](auto i)
                   {
                       if (packed.empty())
                           return bucket[i].node_id;
                       return packed[forward ? i + 1 : packed.size() - 2 - i];
                   });
    }

    bool HasEntryForID(const EdgeID edge_id) const;
    void PrintStatistics() const;
    unsigned GetPositionForID(const EdgeID edge_id) const;
    const OnewayEdgeBucket &GetBucketReference(const EdgeID edge_id) const;
    bool IsTrivial(const EdgeID edge_id) const;
    NodeID GetFirstEdgeTargetID(const EdgeID edge_id) const;
    NodeID GetLastEdgeTargetID(const EdgeID edge_id) const;
    NodeID GetLastEdgeSourceID(const EdgeID edge_id) const;

    // Invalidates the internal storage
    std::unique_ptr<SegmentDataContainer> ToSegmentData();

  private:
    SegmentWeight ClipWeight(const EdgeWeight weight);
    SegmentDuration ClipDuration(const EdgeDuration duration);

    int free_list_maximum = 0;
    std::atomic_size_t clipped_weights{0};
    std::atomic_size_t clipped_durations{0};

    void IncreaseFreeList();
    std::vector<OnewayEdgeBucket> m_compressed_oneway_geometries;
    std::vector<unsigned> m_free_list;
    std::unordered_map<EdgeID, unsigned> m_edge_id_to_list_index_map;
    std::unique_ptr<SegmentDataContainer> segment_data;
    bool m_use_packed_geometry = false;
};
} // namespace osrm::extractor

#endif // GEOMETRY_COMPRESSOR_HPP_

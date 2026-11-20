#include "extractor/compressed_edge_container.hpp"
#include "extractor/files.hpp"
#include "extractor/heap_pages.hpp"
#include "util/log.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <filesystem>

namespace osrm::extractor
{

CompressedEdgeContainer::CompressedEdgeContainer(std::filesystem::path spool_parent)
    : m_geometry_records(std::move(spool_parent)),
      m_geometry_buckets(std::make_unique<GeometryBuckets>())
{
    m_free_list.reserve(100);
    IncreaseFreeList();
}

void CompressedEdgeContainer::IncreaseFreeList()
{
    for (unsigned i = 100; i > 0; --i)
    {
        m_geometry_buckets->buckets.emplace_back();
        m_free_list.emplace_back(free_list_maximum);
        ++free_list_maximum;
    }
}

void CompressedEdgeContainer::initializeEdgeIndex(std::size_t edge_capacity)
{
    m_edge_id_to_list_index.resize(edge_capacity);
    std::fill(m_edge_id_to_list_index.begin(), m_edge_id_to_list_index.end(), NoBucket);
}

void CompressedEdgeContainer::moveEdgeMappings(EdgeID source, EdgeID destination, unsigned count)
{
    const auto begin = m_edge_id_to_list_index.begin();
    std::move(begin + source, begin + source + count, begin + destination);
}

void CompressedEdgeContainer::resizeEdgeIndex(std::size_t edge_capacity)
{
    BOOST_ASSERT(edge_capacity <= m_edge_id_to_list_index.size());
    m_edge_id_to_list_index.resize(edge_capacity);
}

void CompressedEdgeContainer::assignBucket(EdgeID edge)
{
    if (edge >= m_edge_id_to_list_index.size())
    {
        const auto old_size = m_edge_id_to_list_index.size();
        m_edge_id_to_list_index.resize(std::size_t(edge) + 1);
        std::fill(
            m_edge_id_to_list_index.begin() + old_size, m_edge_id_to_list_index.end(), NoBucket);
    }
    if (m_free_list.empty())
        IncreaseFreeList();
    m_edge_id_to_list_index[edge] = m_free_list.back();
    m_free_list.pop_back();
}

bool CompressedEdgeContainer::HasEntryForID(const EdgeID edge_id) const
{ return m_geometry_buckets && findBucket(edge_id) < m_geometry_buckets->buckets.size(); }

unsigned CompressedEdgeContainer::GetPositionForID(const EdgeID edge_id) const
{
    const auto bucket = findBucket(edge_id);
    BOOST_ASSERT(bucket < m_geometry_buckets->buckets.size());
    return bucket;
}

SegmentWeight CompressedEdgeContainer::ClipWeight(const EdgeWeight weight)
{
    SegmentWeight seg_weight = alias_cast<SegmentWeight>(weight);
    if (seg_weight >= INVALID_SEGMENT_WEIGHT)
    {
        clipped_weights++;
        return MAX_SEGMENT_WEIGHT;
    }
    return seg_weight;
}

SegmentDuration CompressedEdgeContainer::ClipDuration(const EdgeDuration duration)
{
    SegmentDuration seg_duration = alias_cast<SegmentDuration>(duration);
    if (seg_duration >= INVALID_SEGMENT_DURATION)
    {
        clipped_weights++;
        return MAX_SEGMENT_DURATION;
    }
    return seg_duration;
}

// Adds info for a compressed edge to the container.   edge_id_2
// has been removed from the graph, so we have to save These edges/nodes
// have already been trimmed from the graph, this function just stores
// the original data for unpacking later.
//
//     edge_id_1               edge_id_2
//   ----------> via_node_id -----------> target_node_id
//     weight_1                weight_2
//     duration_1              duration_2
void CompressedEdgeContainer::CompressEdge(const EdgeID edge_id_1,
                                           const EdgeID edge_id_2,
                                           const NodeID via_node_id,
                                           const NodeID target_node_id,
                                           const EdgeWeight weight1,
                                           const EdgeWeight weight2,
                                           const EdgeDuration duration1,
                                           const EdgeDuration duration2,
                                           const EdgeWeight node_weight_penalty,
                                           const EdgeDuration node_duration_penalty)
{
    // remove super-trivial geometries
    BOOST_ASSERT(SPECIAL_EDGEID != edge_id_1);
    BOOST_ASSERT(SPECIAL_EDGEID != edge_id_2);
    BOOST_ASSERT(SPECIAL_NODEID != via_node_id);
    BOOST_ASSERT(SPECIAL_NODEID != target_node_id);
    BOOST_ASSERT(INVALID_EDGE_WEIGHT != weight1);
    BOOST_ASSERT(INVALID_EDGE_WEIGHT != weight2);

    // append list of removed edge_id plus via node to surviving edge id:
    // <surv_1, .. , surv_n, via_node_id, rem_1, .. rem_n
    //
    // General scheme:
    // 1. append via node id to list of edge_id_1
    // 2. find list for edge_id_2, if yes add all elements and delete it

    if (!HasEntryForID(edge_id_1))
        assignBucket(edge_id_1);
    const auto edge_bucket_id1 = GetPositionForID(edge_id_1);

    OnewayEdgeBucket &edge_bucket_list1 = m_geometry_buckets->buckets[edge_bucket_id1];

    bool was_empty = edge_bucket_list1.empty();

    // note we don't save the start coordinate: it is implicitly given by edge 1
    // weight1 is the distance to the (currently) last coordinate in the bucket
    if (was_empty)
    {
        edge_bucket_list1.emplace_back(
            OnewayCompressedEdge{via_node_id, ClipWeight(weight1), ClipDuration(duration1)});
    }

    BOOST_ASSERT(0 < edge_bucket_list1.size());
    BOOST_ASSERT(!edge_bucket_list1.empty());

    // if the via-node offers a penalty, we add the weight of the penalty as an artificial
    // segment that references SPECIAL_NODEID
    if (node_weight_penalty != INVALID_EDGE_WEIGHT &&
        node_duration_penalty != MAXIMAL_EDGE_DURATION)
    {
        edge_bucket_list1.emplace_back(OnewayCompressedEdge{
            via_node_id, ClipWeight(node_weight_penalty), ClipDuration(node_duration_penalty)});
    }

    if (HasEntryForID(edge_id_2))
    {
        // second edge is not atomic anymore
        const unsigned list_to_remove_index = GetPositionForID(edge_id_2);
        BOOST_ASSERT(list_to_remove_index < m_geometry_buckets->buckets.size());

        OnewayEdgeBucket &edge_bucket_list2 = m_geometry_buckets->buckets[list_to_remove_index];

        // found an existing list, append it to the list of edge_id_1
        edge_bucket_list1.insert(
            edge_bucket_list1.end(), edge_bucket_list2.begin(), edge_bucket_list2.end());

        // remove the list of edge_id_2
        m_edge_id_to_list_index[edge_id_2] = NoBucket;
        edge_bucket_list2.clear();
        BOOST_ASSERT(0 == edge_bucket_list2.size());
        m_free_list.emplace_back(list_to_remove_index);
        BOOST_ASSERT(list_to_remove_index == m_free_list.back());
    }
    else
    {
        // we are certain that the second edge is atomic.
        edge_bucket_list1.emplace_back(
            OnewayCompressedEdge{target_node_id, ClipWeight(weight2), ClipDuration(duration2)});
    }
}

void CompressedEdgeContainer::AddUncompressedEdge(const EdgeID edge_id,
                                                  const NodeID target_node_id,
                                                  const EdgeWeight weight,
                                                  const EdgeDuration duration)
{
    // remove super-trivial geometries
    BOOST_ASSERT(SPECIAL_EDGEID != edge_id);
    BOOST_ASSERT(SPECIAL_NODEID != target_node_id);
    BOOST_ASSERT(INVALID_EDGE_WEIGHT != weight);
    BOOST_ASSERT(INVALID_EDGE_DURATION != duration);

    if (!HasEntryForID(edge_id))
        assignBucket(edge_id);
    auto &edge_bucket_list = m_geometry_buckets->buckets[GetPositionForID(edge_id)];

    // note we don't save the start coordinate: it is implicitly given by edge_id
    // weight is the distance to the (currently) last coordinate in the bucket
    // Don't re-add this if it's already in there.
    if (edge_bucket_list.empty())
    {
        edge_bucket_list.emplace_back(
            OnewayCompressedEdge{target_node_id, ClipWeight(weight), ClipDuration(duration)});
    }
}

unsigned CompressedEdgeContainer::ZipEdges(const EdgeID f_edge_id, const EdgeID r_edge_id)
{
    if (!segment_data)
        segment_data = std::make_unique<SegmentDataContainer>();

    const auto &forward_bucket = GetBucketReference(f_edge_id);
    const auto &reverse_bucket = GetBucketReference(r_edge_id);
    BOOST_ASSERT(forward_bucket.size() == reverse_bucket.size());
    BOOST_ASSERT(!forward_bucket.empty());

    const unsigned geometry_id = segment_data->index.size();
    segment_data->index.emplace_back(m_geometry_records.size());

    const auto &first_node = reverse_bucket.back();
    m_geometry_records.append({first_node.node_id,
                               INVALID_SEGMENT_WEIGHT,
                               first_node.weight,
                               INVALID_SEGMENT_DURATION,
                               first_node.duration});
    for (std::size_t i = 0; i + 1 < forward_bucket.size(); ++i)
    {
        const auto &forward = forward_bucket.at(i);
        const auto &reverse = reverse_bucket.at(reverse_bucket.size() - 2 - i);
        BOOST_ASSERT(forward.node_id == reverse.node_id);
        m_geometry_records.append(
            {forward.node_id, forward.weight, reverse.weight, forward.duration, reverse.duration});
    }
    const auto &last_node = forward_bucket.back();
    m_geometry_records.append({last_node.node_id,
                               last_node.weight,
                               INVALID_SEGMENT_WEIGHT,
                               last_node.duration,
                               INVALID_SEGMENT_DURATION});
    return geometry_id;
}

void CompressedEdgeContainer::releaseEquivalentEdgeGeometry(EdgeID forward, EdgeID reverse)
{
    auto &forward_bucket = m_geometry_buckets->buckets[GetPositionForID(forward)];
    auto &reverse_bucket = m_geometry_buckets->buckets[GetPositionForID(reverse)];
    const auto forward_interior = std::span(forward_bucket).first(forward_bucket.size() - 1);
    const auto reverse_interior = std::span(reverse_bucket).first(reverse_bucket.size() - 1);
    if (std::ranges::equal(forward_interior,
                           reverse_interior | std::views::reverse,
                           {},
                           &OnewayCompressedEdge::node_id,
                           &OnewayCompressedEdge::node_id))
    {
        OnewayEdgeBucket(&m_geometry_buckets->memory).swap(forward_bucket);
        OnewayEdgeBucket(&m_geometry_buckets->memory).swap(reverse_bucket);
        m_edge_id_to_list_index[forward] = PackedGeometry;
        m_edge_id_to_list_index[reverse] = PackedGeometry;
    }
}

void CompressedEdgeContainer::releaseGeometryBuckets()
{
    m_geometry_buckets.reset();
    m_edge_id_to_list_index.clear();
    decltype(m_free_list)().swap(m_free_list);
}

void CompressedEdgeContainer::loadPackedGeometry()
{
    m_geometry_records.finish();
    const auto count = m_geometry_records.size();
    segment_data->index.push_back(count);
    segment_data->nodes.reserve(count);
    segment_data->fwd_weights.reserve(count);
    segment_data->rev_weights.reserve(count);
    segment_data->fwd_durations.reserve(count);
    segment_data->rev_durations.reserve(count);
    m_geometry_records.forEachBatch(
        [&](std::span<const GeometryRecord> records)
        {
            for (const auto &record : records)
            {
                segment_data->nodes.emplace_back(record.node);
                segment_data->fwd_weights.emplace_back(record.forward_weight);
                segment_data->rev_weights.emplace_back(record.reverse_weight);
                segment_data->fwd_durations.emplace_back(record.forward_duration);
                segment_data->rev_durations.emplace_back(record.reverse_duration);
            }
        });
    m_geometry_records.clear();
    segment_data->fwd_datasources.assign(count, 0);
    segment_data->rev_datasources.assign(count, 0);
}

bool CompressedEdgeContainer::finalizeGeometry(const util::NodeBasedDynamicGraph &graph)
{
    const auto ready = [&](EdgeID edge)
    {
        const auto bucket = findBucket(edge);
        return bucket == PackedGeometry ||
               (bucket == NoBucket && graph.GetEdgeData(edge).geometry_id.id == SPECIAL_GEOMETRYID);
    };
    const auto all_packed = [&]
    {
        for (const auto node : util::irange(0u, graph.GetNumberOfNodes()))
        {
            if (!std::ranges::all_of(graph.GetAdjacentEdgeRange(node), ready))
                return false;
        }
        return true;
    }();
    if (all_packed)
    {
        releaseGeometryBuckets();
        releaseUnusedHeapPages();
    }
    loadPackedGeometry();
    m_use_packed_geometry = true;

    if (!all_packed)
    {
        const auto matches = [&](EdgeID edge)
        {
            if (ready(edge))
                return true;
            const auto geometry = graph.GetEdgeData(edge).geometry_id;
            const auto bucket = findBucket(edge);
            return geometry.id < segment_data->GetNumberOfGeometries() &&
                   bucket < m_geometry_buckets->buckets.size() &&
                   std::ranges::equal(m_geometry_buckets->buckets[bucket],
                                      getGeometryNodes(edge, geometry),
                                      {},
                                      &OnewayCompressedEdge::node_id);
        };
        for (const auto node : util::irange(0u, graph.GetNumberOfNodes()))
        {
            if (!std::ranges::all_of(graph.GetAdjacentEdgeRange(node), matches))
            {
                m_has_fallback_geometry = true;
                util::Log() << "Retaining per-edge geometry for mismatched sequences";
                return false;
            }
        }
        releaseGeometryBuckets();
    }
    util::Log() << "Using packed geometry for edge expansion and guidance";
    return true;
}

void CompressedEdgeContainer::PrintStatistics() const
{
    const uint64_t compressed_edges = m_geometry_buckets->buckets.size();
    BOOST_ASSERT(0 == compressed_edges % 2);
    BOOST_ASSERT(m_geometry_buckets->buckets.size() + m_free_list.size() > 0);

    uint64_t compressed_geometries = 0;
    uint64_t longest_chain_length = 0;
    for (const OnewayEdgeBucket &current_vector : m_geometry_buckets->buckets)
    {
        compressed_geometries += current_vector.size();
        longest_chain_length = std::max(longest_chain_length, (uint64_t)current_vector.size());
    }

    if (clipped_weights > 0)
    {
        util::Log(logWARNING) << "Clipped " << clipped_weights << " segment weights to "
                              << MAX_SEGMENT_WEIGHT;
    }
    if (clipped_durations > 0)
    {
        util::Log(logWARNING) << "Clipped " << clipped_durations << " segment durations to "
                              << MAX_SEGMENT_DURATION;
    }

    util::Log() << "Geometry successfully removed:"
                   "\n  compressed edges: "
                << compressed_edges << "\n  compressed geometries: " << compressed_geometries
                << "\n  longest chain length: " << longest_chain_length << "\n  cmpr ratio: "
                << ((float)compressed_edges / std::max(compressed_geometries, (uint64_t)1))
                << "\n  avg chain length: "
                << (float)compressed_geometries / std::max((uint64_t)1, compressed_edges);
}

const CompressedEdgeContainer::OnewayEdgeBucket &
CompressedEdgeContainer::GetBucketReference(const EdgeID edge_id) const
{
    const unsigned index = GetPositionForID(edge_id);
    return m_geometry_buckets->buckets.at(index);
}

// Since all edges are technically in the compressed geometry container,
// regardless of whether a compressed edge actually contains multiple
// original segments, we use 'Trivial' here to describe compressed edges
// that only contain one original segment
bool CompressedEdgeContainer::IsTrivial(const EdgeID edge_id) const
{
    const auto &bucket = GetBucketReference(edge_id);
    return bucket.size() == 1;
}

NodeID CompressedEdgeContainer::GetFirstEdgeTargetID(const EdgeID edge_id) const
{
    const auto &bucket = GetBucketReference(edge_id);
    BOOST_ASSERT(bucket.size() >= 1);
    return bucket.front().node_id;
}
NodeID CompressedEdgeContainer::GetLastEdgeTargetID(const EdgeID edge_id) const
{
    const auto &bucket = GetBucketReference(edge_id);
    BOOST_ASSERT(bucket.size() >= 1);
    return bucket.back().node_id;
}
NodeID CompressedEdgeContainer::GetLastEdgeSourceID(const EdgeID edge_id) const
{
    const auto &bucket = GetBucketReference(edge_id);
    BOOST_ASSERT(bucket.size() >= 2);
    return bucket[bucket.size() - 2].node_id;
}

void CompressedEdgeContainer::writeGeometryAndReleaseCosts(const std::filesystem::path &path)
{
    files::writeSegmentData(path, *segment_data);
    segment_data->fwd_weights = {};
    segment_data->rev_weights = {};
    segment_data->fwd_durations = {};
    segment_data->rev_durations = {};
    decltype(segment_data->fwd_datasources)().swap(segment_data->fwd_datasources);
    decltype(segment_data->rev_datasources)().swap(segment_data->rev_datasources);
}

void CompressedEdgeContainer::releaseGeometry()
{
    segment_data.reset();
    releaseGeometryBuckets();
}
} // namespace osrm::extractor

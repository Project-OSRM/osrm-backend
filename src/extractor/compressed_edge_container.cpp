#include "extractor/compressed_edge_container.hpp"
#include "extractor/files.hpp"
#include "util/log.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <filesystem>

namespace osrm::extractor
{

CompressedEdgeContainer::CompressedEdgeContainer()
{
    m_free_list.reserve(100);
    IncreaseFreeList();
}

void CompressedEdgeContainer::IncreaseFreeList()
{
    m_compressed_oneway_geometries.resize(m_compressed_oneway_geometries.size() + 100);
    for (unsigned i = 100; i > 0; --i)
    {
        m_free_list.emplace_back(free_list_maximum);
        ++free_list_maximum;
    }
}

void CompressedEdgeContainer::initializeEdgeIndex(std::size_t edge_capacity)
{ m_edge_id_to_list_index.assign(edge_capacity, NoBucket); }

void CompressedEdgeContainer::assignBucket(EdgeID edge)
{
    if (edge >= m_edge_id_to_list_index.size())
        m_edge_id_to_list_index.resize(std::size_t(edge) + 1, NoBucket);
    if (m_free_list.empty())
        IncreaseFreeList();
    m_edge_id_to_list_index[edge] = m_free_list.back();
    m_free_list.pop_back();
}

bool CompressedEdgeContainer::HasEntryForID(const EdgeID edge_id) const
{ return findBucket(edge_id) < m_compressed_oneway_geometries.size(); }

unsigned CompressedEdgeContainer::GetPositionForID(const EdgeID edge_id) const
{
    const auto bucket = findBucket(edge_id);
    BOOST_ASSERT(bucket < m_compressed_oneway_geometries.size());
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

    std::vector<OnewayCompressedEdge> &edge_bucket_list1 =
        m_compressed_oneway_geometries[edge_bucket_id1];

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
        BOOST_ASSERT(list_to_remove_index < m_compressed_oneway_geometries.size());

        std::vector<OnewayCompressedEdge> &edge_bucket_list2 =
            m_compressed_oneway_geometries[list_to_remove_index];

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
    auto &edge_bucket_list = m_compressed_oneway_geometries[GetPositionForID(edge_id)];

    // note we don't save the start coordinate: it is implicitly given by edge_id
    // weight is the distance to the (currently) last coordinate in the bucket
    // Don't re-add this if it's already in there.
    if (edge_bucket_list.empty())
    {
        edge_bucket_list.emplace_back(
            OnewayCompressedEdge{target_node_id, ClipWeight(weight), ClipDuration(duration)});
    }
}

void CompressedEdgeContainer::InitializeBothwayVector()
{
    segment_data = std::make_unique<SegmentDataContainer>();
    std::size_t nonempty_buckets = 0;
    std::size_t segments = 0;
    for (const auto &bucket : m_compressed_oneway_geometries)
    {
        nonempty_buckets += !bucket.empty();
        segments += bucket.size();
    }
    const auto geometries = nonempty_buckets / 2;
    const auto locations = segments / 2 + geometries;
    segment_data->index.reserve(geometries + 1);
    segment_data->nodes.reserve(locations);
    segment_data->fwd_weights.reserve(locations);
    segment_data->rev_weights.reserve(locations);
    segment_data->fwd_durations.reserve(locations);
    segment_data->rev_durations.reserve(locations);
    segment_data->fwd_datasources.reserve(locations);
    segment_data->rev_datasources.reserve(locations);
}

unsigned CompressedEdgeContainer::ZipEdges(const EdgeID f_edge_id, const EdgeID r_edge_id)
{
    if (!segment_data)
        InitializeBothwayVector();

    const auto &forward_bucket = GetBucketReference(f_edge_id);
    const auto &reverse_bucket = GetBucketReference(r_edge_id);

    BOOST_ASSERT(forward_bucket.size() == reverse_bucket.size());

    const unsigned zipped_geometry_id = segment_data->index.size();

    segment_data->index.emplace_back(segment_data->nodes.size());

    const auto &first_node = reverse_bucket.back();

    constexpr DatasourceID LUA_SOURCE = 0;

    segment_data->nodes.emplace_back(first_node.node_id);
    segment_data->fwd_weights.emplace_back(INVALID_SEGMENT_WEIGHT);
    segment_data->rev_weights.emplace_back(first_node.weight);
    segment_data->fwd_durations.emplace_back(INVALID_SEGMENT_DURATION);
    segment_data->rev_durations.emplace_back(first_node.duration);
    segment_data->fwd_datasources.emplace_back(LUA_SOURCE);
    segment_data->rev_datasources.emplace_back(LUA_SOURCE);

    for (std::size_t i = 0; i < forward_bucket.size() - 1; ++i)
    {
        const auto &fwd_node = forward_bucket.at(i);
        const auto &rev_node = reverse_bucket.at(reverse_bucket.size() - 2 - i);

        BOOST_ASSERT(fwd_node.node_id == rev_node.node_id);

        segment_data->nodes.emplace_back(fwd_node.node_id);
        segment_data->fwd_weights.emplace_back(fwd_node.weight);
        segment_data->rev_weights.emplace_back(rev_node.weight);
        segment_data->fwd_durations.emplace_back(fwd_node.duration);
        segment_data->rev_durations.emplace_back(rev_node.duration);
        segment_data->fwd_datasources.emplace_back(LUA_SOURCE);
        segment_data->rev_datasources.emplace_back(LUA_SOURCE);
    }

    const auto &last_node = forward_bucket.back();

    segment_data->nodes.emplace_back(last_node.node_id);
    segment_data->fwd_weights.emplace_back(last_node.weight);
    segment_data->rev_weights.emplace_back(INVALID_SEGMENT_WEIGHT);
    segment_data->fwd_durations.emplace_back(last_node.duration);
    segment_data->rev_durations.emplace_back(INVALID_SEGMENT_DURATION);
    segment_data->fwd_datasources.emplace_back(LUA_SOURCE);
    segment_data->rev_datasources.emplace_back(LUA_SOURCE);

    return zipped_geometry_id;
}

void CompressedEdgeContainer::releaseEquivalentEdgeGeometry(EdgeID forward,
                                                            EdgeID reverse,
                                                            unsigned geometry_id)
{
    BOOST_ASSERT(geometry_id + 1 == segment_data->index.size());
    const auto nodes =
        std::span<const NodeID>(segment_data->nodes).subspan(segment_data->index[geometry_id]);
    const auto forward_bucket = GetPositionForID(forward);
    const auto reverse_bucket = GetPositionForID(reverse);
    if (std::ranges::equal(m_compressed_oneway_geometries[forward_bucket],
                           nodes.subspan(1),
                           {},
                           &OnewayCompressedEdge::node_id) &&
        std::ranges::equal(m_compressed_oneway_geometries[reverse_bucket],
                           nodes.first(nodes.size() - 1) | std::views::reverse,
                           {},
                           &OnewayCompressedEdge::node_id))
    {
        OnewayEdgeBucket().swap(m_compressed_oneway_geometries[forward_bucket]);
        OnewayEdgeBucket().swap(m_compressed_oneway_geometries[reverse_bucket]);
        m_edge_id_to_list_index[forward] = PackedGeometry;
        m_edge_id_to_list_index[reverse] = PackedGeometry;
    }
}

bool CompressedEdgeContainer::finalizeGeometry(const util::NodeBasedDynamicGraph &graph)
{
    segment_data->index.push_back(segment_data->nodes.size());
    m_use_packed_geometry = true;

    const auto matches = [&](EdgeID edge)
    {
        const auto geometry = graph.GetEdgeData(edge).geometry_id;
        const auto bucket = findBucket(edge);
        if (bucket == PackedGeometry)
            return true;
        if (geometry.id == SPECIAL_GEOMETRYID)
            return bucket == NoBucket;
        return geometry.id < segment_data->GetNumberOfGeometries() &&
               bucket < m_compressed_oneway_geometries.size() &&
               std::ranges::equal(m_compressed_oneway_geometries[bucket],
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

    decltype(m_compressed_oneway_geometries)().swap(m_compressed_oneway_geometries);
    decltype(m_edge_id_to_list_index)().swap(m_edge_id_to_list_index);
    decltype(m_free_list)().swap(m_free_list);
    util::Log() << "Using packed geometry for edge expansion and guidance";
    return true;
}

void CompressedEdgeContainer::PrintStatistics() const
{
    const uint64_t compressed_edges = m_compressed_oneway_geometries.size();
    BOOST_ASSERT(0 == compressed_edges % 2);
    BOOST_ASSERT(m_compressed_oneway_geometries.size() + m_free_list.size() > 0);

    uint64_t compressed_geometries = 0;
    uint64_t longest_chain_length = 0;
    for (const std::vector<OnewayCompressedEdge> &current_vector : m_compressed_oneway_geometries)
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
    return m_compressed_oneway_geometries.at(index);
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

void CompressedEdgeContainer::releaseGeometry() { segment_data.reset(); }
} // namespace osrm::extractor

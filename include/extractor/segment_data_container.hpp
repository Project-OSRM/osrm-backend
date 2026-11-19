#ifndef OSRM_EXTRACTOR_SEGMENT_DATA_CONTAINER_HPP_
#define OSRM_EXTRACTOR_SEGMENT_DATA_CONTAINER_HPP_

#include "util/packed_vector.hpp"
#include "util/typedefs.hpp"
#include "util/vector_view.hpp"

#include "storage/shared_memory_ownership.hpp"
#include "storage/tar_fwd.hpp"

#include <ranges>
#include <string>

namespace osrm::extractor
{

class CompressedEdgeContainer;

namespace detail
{
template <storage::Ownership Ownership> class SegmentDataContainerImpl;
} // namespace detail

namespace serialization
{
template <storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::SegmentDataContainerImpl<Ownership> &segment_data);
template <storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::SegmentDataContainerImpl<Ownership> &segment_data);
} // namespace serialization

namespace detail
{
template <storage::Ownership Ownership> class SegmentDataContainerImpl
{
    template <typename T> using Vector = util::ViewOrVector<T, Ownership>;
    template <typename T, std::size_t Bits>
    using PackedVector = util::detail::PackedVector<T, Bits, Ownership>;

    friend CompressedEdgeContainer;

  public:
    // FIXME We should change the indexing to Edge-Based-Node id
    using DirectionalGeometryID = std::uint32_t;
    using SegmentOffset = std::uint32_t;
    using SegmentNodeVector = Vector<NodeID>;
    using SegmentWeightVector = PackedVector<SegmentWeight, SEGMENT_WEIGHT_BITS>;
    using SegmentDurationVector = PackedVector<SegmentDuration, SEGMENT_DURATION_BITS>;
    using SegmentDatasourceVector = Vector<DatasourceID>;

    SegmentDataContainerImpl() = default;

    SegmentDataContainerImpl(Vector<std::uint32_t> index_,
                             SegmentNodeVector nodes_,
                             SegmentWeightVector fwd_weights_,
                             SegmentWeightVector rev_weights_,
                             SegmentDurationVector fwd_durations_,
                             SegmentDurationVector rev_durations_,
                             SegmentDatasourceVector fwd_datasources_,
                             SegmentDatasourceVector rev_datasources_)
        : index(std::move(index_)), nodes(std::move(nodes_)), fwd_weights(std::move(fwd_weights_)),
          rev_weights(std::move(rev_weights_)), fwd_durations(std::move(fwd_durations_)),
          rev_durations(std::move(rev_durations_)), fwd_datasources(std::move(fwd_datasources_)),
          rev_datasources(std::move(rev_datasources_))
    {
    }

    SegmentDataContainerImpl(Vector<std::uint32_t> index_,
                             SegmentNodeVector nodes_,
                             SegmentDatasourceVector fwd_sources,
                             SegmentDatasourceVector rev_sources,
                             Vector<std::uint64_t> descriptors,
                             SegmentWeightVector weights,
                             SegmentDurationVector durations)
        : index(std::move(index_)), nodes(std::move(nodes_)),
          fwd_datasources(std::move(fwd_sources)), rev_datasources(std::move(rev_sources)),
          shared_descriptors(std::move(descriptors)), shared_weights(std::move(weights)),
          shared_durations(std::move(durations))
    {
    }

    auto GetForwardGeometry(const DirectionalGeometryID id)
    {
        auto ptr = nodes.data() + index[id];
        auto len = static_cast<std::size_t>(index[id + 1] - index[id]);
        return std::span<NodeID>(ptr, len);
    }

    auto GetReverseGeometry(const DirectionalGeometryID id)
    { return GetForwardGeometry(id) | std::views::reverse; }

    auto GetForwardDurations(const DirectionalGeometryID id)
    {
        const auto begin = fwd_durations.begin() + index[id] + 1;
        const auto end = fwd_durations.begin() + index[id + 1];

        return std::ranges::subrange(begin, end);
    }

    auto GetReverseDurations(const DirectionalGeometryID id)
    {
        const auto begin = rev_durations.begin() + index[id];
        const auto end = rev_durations.begin() + index[id + 1] - 1;

        return std::ranges::subrange(begin, end) | std::views::reverse;
    }

    auto GetForwardWeights(const DirectionalGeometryID id)
    {
        const auto begin = fwd_weights.begin() + index[id] + 1;
        const auto end = fwd_weights.begin() + index[id + 1];

        return std::ranges::subrange(begin, end);
    }

    auto GetReverseWeights(const DirectionalGeometryID id)
    {
        const auto begin = rev_weights.begin() + index[id];
        const auto end = rev_weights.begin() + index[id + 1] - 1;

        return std::ranges::subrange(begin, end) | std::views::reverse;
    }

    auto GetForwardDatasources(const DirectionalGeometryID id)
    {
        if (fwd_datasources.size() != nodes.size())
        {
            const auto len = index[id + 1] - index[id] - 1;
            BOOST_ASSERT(len <= fwd_datasources.size());
            return std::span<DatasourceID>(fwd_datasources.data(), len);
        }
        auto ptr = fwd_datasources.data() + index[id] + 1;
        auto len = static_cast<std::size_t>(index[id + 1] - (index[id] + 1));
        return std::span<DatasourceID>(ptr, len);
    }

    auto GetReverseDatasources(const DirectionalGeometryID id)
    {
        if (rev_datasources.size() != nodes.size())
        {
            const auto len = index[id + 1] - index[id] - 1;
            BOOST_ASSERT(len <= rev_datasources.size());
            return std::span<DatasourceID>(rev_datasources.data(), len) | std::views::reverse;
        }
        auto ptr = rev_datasources.data() + index[id];
        auto len = static_cast<std::size_t>((index[id + 1] - 1) - index[id]);
        return std::span<DatasourceID>(ptr, len) | std::views::reverse;
    }

    auto GetForwardGeometry(const DirectionalGeometryID id) const
    {
        auto ptr = nodes.data() + index[id];
        auto len = static_cast<std::size_t>(index[id + 1] - index[id]);
        return std::span<const NodeID>(ptr, len);
    }

    auto GetReverseGeometry(const DirectionalGeometryID id) const
    { return GetForwardGeometry(id) | std::views::reverse; }

    auto GetForwardDurations(const DirectionalGeometryID id) const
    {
        if (!shared_descriptors.empty())
        {
            const auto descriptor = shared_descriptors[id];
            const auto length = (descriptor >> 32) & 65535;
            const auto ordinal = ((descriptor >> 48) & 3);
            if (descriptor >> 63)
            {
                const auto width = (descriptor >> 54) & 31;
                const auto base = (descriptor & 0xffffffff) + ordinal * length * width;
                using Iterator = typename SegmentDurationVector::const_iterator;
                const auto begin = Iterator::encoded(
                    shared_durations.data(), base, width, (descriptor >> 59) & 1, 0);
                const auto end = Iterator::encoded(
                    shared_durations.data(), base, width, (descriptor >> 59) & 1, length);
                return std::ranges::subrange(begin, end);
            }
            const auto offset = (descriptor & 0xffffffff) + ordinal * length;
            const auto begin = shared_durations.begin() + offset;
            return std::ranges::subrange(begin, begin + length);
        }
        const auto begin = fwd_durations.cbegin() + index[id] + 1;
        const auto end = fwd_durations.cbegin() + index[id + 1];

        return std::ranges::subrange(begin, end);
    }

    auto GetReverseDurations(const DirectionalGeometryID id) const
    {
        if (!shared_descriptors.empty())
        {
            const auto descriptor = shared_descriptors[id];
            const auto length = (descriptor >> 32) & 65535;
            const auto ordinal = ((descriptor >> 52) & 3);
            if (descriptor >> 63)
            {
                const auto width = (descriptor >> 54) & 31;
                const auto base = (descriptor & 0xffffffff) + ordinal * length * width;
                using Iterator = typename SegmentDurationVector::const_iterator;
                const auto begin = Iterator::encoded(
                    shared_durations.data(), base, width, (descriptor >> 59) & 1, 0);
                const auto end = Iterator::encoded(
                    shared_durations.data(), base, width, (descriptor >> 59) & 1, length);
                return std::ranges::subrange(begin, end) | std::views::reverse;
            }
            const auto offset = (descriptor & 0xffffffff) + ordinal * length;
            const auto begin = shared_durations.begin() + offset;
            return std::ranges::subrange(begin, begin + length) | std::views::reverse;
        }
        const auto begin = rev_durations.cbegin() + index[id];
        const auto end = rev_durations.cbegin() + index[id + 1] - 1;

        return std::ranges::subrange(begin, end) | std::views::reverse;
    }

    auto GetForwardWeights(const DirectionalGeometryID id) const
    {
        if (!shared_descriptors.empty())
        {
            const auto descriptor = shared_descriptors[id];
            const auto length = (descriptor >> 32) & 65535;
            const auto ordinal = 0;
            if (descriptor >> 63)
            {
                const auto width = (descriptor >> 54) & 31;
                const auto base = (descriptor & 0xffffffff) + ordinal * length * width;
                using Iterator = typename SegmentWeightVector::const_iterator;
                const auto begin = Iterator::encoded(
                    shared_weights.data(), base, width, (descriptor >> 59) & 1, 0);
                const auto end = Iterator::encoded(
                    shared_weights.data(), base, width, (descriptor >> 59) & 1, length);
                return std::ranges::subrange(begin, end);
            }
            const auto offset = (descriptor & 0xffffffff) + ordinal * length;
            const auto begin = shared_weights.begin() + offset;
            return std::ranges::subrange(begin, begin + length);
        }
        const auto begin = fwd_weights.cbegin() + index[id] + 1;
        const auto end = fwd_weights.cbegin() + index[id + 1];

        return std::ranges::subrange(begin, end);
    }

    auto GetReverseWeights(const DirectionalGeometryID id) const
    {
        if (!shared_descriptors.empty())
        {
            const auto descriptor = shared_descriptors[id];
            const auto length = (descriptor >> 32) & 65535;
            const auto ordinal = ((descriptor >> 50) & 3);
            if (descriptor >> 63)
            {
                const auto width = (descriptor >> 54) & 31;
                const auto base = (descriptor & 0xffffffff) + ordinal * length * width;
                using Iterator = typename SegmentWeightVector::const_iterator;
                const auto begin = Iterator::encoded(
                    shared_weights.data(), base, width, (descriptor >> 59) & 1, 0);
                const auto end = Iterator::encoded(
                    shared_weights.data(), base, width, (descriptor >> 59) & 1, length);
                return std::ranges::subrange(begin, end) | std::views::reverse;
            }
            const auto offset = (descriptor & 0xffffffff) + ordinal * length;
            const auto begin = shared_weights.begin() + offset;
            return std::ranges::subrange(begin, begin + length) | std::views::reverse;
        }
        const auto begin = rev_weights.cbegin() + index[id];
        const auto end = rev_weights.cbegin() + index[id + 1] - 1;

        return std::ranges::subrange(begin, end) | std::views::reverse;
    }

    auto GetForwardDatasources(const DirectionalGeometryID id) const
    {
        if (fwd_datasources.size() != nodes.size())
        {
            const auto len = index[id + 1] - index[id] - 1;
            BOOST_ASSERT(len <= fwd_datasources.size());
            return std::span<const DatasourceID>(fwd_datasources.data(), len);
        }
        auto ptr = fwd_datasources.data() + index[id] + 1;
        auto len = static_cast<std::size_t>(index[id + 1] - (index[id] + 1));
        return std::span<const DatasourceID>(ptr, len);
    }

    auto GetReverseDatasources(const DirectionalGeometryID id) const
    {
        if (rev_datasources.size() != nodes.size())
        {
            const auto len = index[id + 1] - index[id] - 1;
            BOOST_ASSERT(len <= rev_datasources.size());
            return std::span<const DatasourceID>(rev_datasources.data(), len) | std::views::reverse;
        }
        auto ptr = rev_datasources.data() + index[id];
        auto len = static_cast<std::size_t>((index[id + 1] - 1) - index[id]);
        return std::span<const DatasourceID>(ptr, len) | std::views::reverse;
    }

    auto GetNumberOfGeometries() const { return index.empty() ? 0 : index.size() - 1; }
    auto GetNumberOfSegments() const { return nodes.size(); }

    friend void
    serialization::read<Ownership>(storage::tar::FileReader &reader,
                                   const std::string &name,
                                   detail::SegmentDataContainerImpl<Ownership> &segment_data);
    friend void serialization::write<Ownership>(
        storage::tar::FileWriter &writer,
        const std::string &name,
        const detail::SegmentDataContainerImpl<Ownership> &segment_data);

  private:
    Vector<std::uint32_t> index;
    SegmentNodeVector nodes;
    SegmentWeightVector fwd_weights;
    SegmentWeightVector rev_weights;
    SegmentDurationVector fwd_durations;
    SegmentDurationVector rev_durations;
    SegmentDatasourceVector fwd_datasources;
    SegmentDatasourceVector rev_datasources;
    Vector<std::uint64_t> shared_descriptors;
    SegmentWeightVector shared_weights;
    SegmentDurationVector shared_durations;
};
} // namespace detail

using SegmentDataView = detail::SegmentDataContainerImpl<storage::Ownership::View>;
using SegmentDataContainer = detail::SegmentDataContainerImpl<storage::Ownership::Container>;
} // namespace osrm::extractor

#endif

#ifndef OSRM_EXTRACTOR_SEGMENT_DATA_CONTAINER_HPP_
#define OSRM_EXTRACTOR_SEGMENT_DATA_CONTAINER_HPP_

#include "util/shared_memory_vector_wrapper.hpp"
#include "util/typedefs.hpp"

#include "storage/shared_memory.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/iterator_range.hpp>

#include <unordered_map>

#include <string>
#include <vector>

namespace osrm
{
namespace extractor
{

class CompressedEdgeContainer;

namespace detail
{
template <osrm::storage::MemorySetting MemorySetting> class SegmentDataContainerImpl;
}

namespace io
{
template <osrm::storage::MemorySetting MemorySetting>
inline void read(const boost::filesystem::path &path,
                 detail::SegmentDataContainerImpl<MemorySetting> &segment_data);
template <osrm::storage::MemorySetting MemorySetting>
inline void write(const boost::filesystem::path &path,
                  const detail::SegmentDataContainerImpl<MemorySetting> &segment_data);
}

namespace detail
{
template <osrm::storage::MemorySetting MemorySetting> class SegmentDataContainerImpl
{
    template <typename T> using Vector = typename util::ShM<T, MemorySetting>::vector;

    friend CompressedEdgeContainer;

  public:
    // FIXME We should change the indexing to Edge-Based-Node id
    using DirectionalGeometryID = std::uint32_t;
    using SegmentOffset = std::uint32_t;

    SegmentDataContainerImpl() = default;

    SegmentDataContainerImpl(Vector<std::uint32_t> index_,
                             Vector<NodeID> nodes_,
                             Vector<EdgeWeight> fwd_weights_,
                             Vector<EdgeWeight> rev_weights_,
                             Vector<EdgeWeight> fwd_durations_,
                             Vector<EdgeWeight> rev_durations_,
                             Vector<DatasourceID> datasources_)
        : index(std::move(index_)), nodes(std::move(nodes_)), fwd_weights(std::move(fwd_weights_)),
          rev_weights(std::move(rev_weights_)), fwd_durations(std::move(fwd_durations_)),
          rev_durations(std::move(rev_durations_)), datasources(std::move(datasources_))
    {
    }

    auto GetForwardGeometry(const DirectionalGeometryID id)
    {
        const auto begin = nodes.begin() + index[id];
        const auto end = nodes.begin() + index[id + 1];

        return boost::make_iterator_range(begin, end);
    }

    auto GetReverseGeometry(const DirectionalGeometryID id)
    {
        return boost::adaptors::reverse(GetForwardGeometry(id));
    }

    auto GetForwardDurations(const DirectionalGeometryID id)
    {
        const auto begin = fwd_durations.begin() + index[id] + 1;
        const auto end = fwd_durations.begin() + index[id + 1];

        return boost::make_iterator_range(begin, end);
    }

    auto GetReverseDurations(const DirectionalGeometryID id)
    {
        const auto begin = rev_durations.begin() + index[id];
        const auto end = rev_durations.begin() + index[id + 1] - 1;

        return boost::adaptors::reverse(boost::make_iterator_range(begin, end));
    }

    auto GetForwardWeights(const DirectionalGeometryID id)
    {
        const auto begin = fwd_weights.begin() + index[id] + 1;
        const auto end = fwd_weights.begin() + index[id + 1];

        return boost::make_iterator_range(begin, end);
    }

    auto GetReverseWeights(const DirectionalGeometryID id)
    {
        const auto begin = rev_weights.begin() + index[id];
        const auto end = rev_weights.begin() + index[id + 1] - 1;

        return boost::adaptors::reverse(boost::make_iterator_range(begin, end));
    }

    auto GetForwardDatasources(const DirectionalGeometryID id)
    {
        const auto begin = datasources.begin() + index[id] + 1;
        const auto end = datasources.begin() + index[id + 1];

        return boost::make_iterator_range(begin, end);
    }

    auto GetReverseDatasources(const DirectionalGeometryID id)
    {
        const auto begin = datasources.begin() + index[id];
        const auto end = datasources.begin() + index[id + 1] - 1;

        return boost::adaptors::reverse(boost::make_iterator_range(begin, end));
    }

    auto GetForwardGeometry(const DirectionalGeometryID id) const
    {
        const auto begin = nodes.cbegin() + index[id];
        const auto end = nodes.cbegin() + index[id + 1];

        return boost::make_iterator_range(begin, end);
    }

    auto GetReverseGeometry(const DirectionalGeometryID id) const
    {
        return boost::adaptors::reverse(GetForwardGeometry(id));
    }

    auto GetForwardDurations(const DirectionalGeometryID id) const
    {
        const auto begin = fwd_durations.cbegin() + index[id] + 1;
        const auto end = fwd_durations.cbegin() + index[id + 1];

        return boost::make_iterator_range(begin, end);
    }

    auto GetReverseDurations(const DirectionalGeometryID id) const
    {
        const auto begin = rev_durations.cbegin() + index[id];
        const auto end = rev_durations.cbegin() + index[id + 1] - 1;

        return boost::adaptors::reverse(boost::make_iterator_range(begin, end));
    }

    auto GetForwardWeights(const DirectionalGeometryID id) const
    {
        const auto begin = fwd_weights.cbegin() + index[id] + 1;
        const auto end = fwd_weights.cbegin() + index[id + 1];

        return boost::make_iterator_range(begin, end);
    }

    auto GetReverseWeights(const DirectionalGeometryID id) const
    {
        const auto begin = rev_weights.cbegin() + index[id];
        const auto end = rev_weights.cbegin() + index[id + 1] - 1;

        return boost::adaptors::reverse(boost::make_iterator_range(begin, end));
    }

    auto GetForwardDatasources(const DirectionalGeometryID id) const
    {
        const auto begin = datasources.cbegin() + index[id] + 1;
        const auto end = datasources.cbegin() + index[id + 1];

        return boost::make_iterator_range(begin, end);
    }

    auto GetReverseDatasources(const DirectionalGeometryID id) const
    {
        const auto begin = datasources.cbegin() + index[id];
        const auto end = datasources.cbegin() + index[id + 1] - 1;

        return boost::adaptors::reverse(boost::make_iterator_range(begin, end));
    }

    auto GetNumberOfGeometries() const { return index.size() - 1; }
    auto GetNumberOfSegments() const { return fwd_weights.size(); }

    friend void
    io::read<MemorySetting>(const boost::filesystem::path &path,
                             detail::SegmentDataContainerImpl<MemorySetting> &segment_data);
    friend void
    io::write<MemorySetting>(const boost::filesystem::path &path,
                              const detail::SegmentDataContainerImpl<MemorySetting> &segment_data);

  private:
    Vector<std::uint32_t> index;
    Vector<NodeID> nodes;
    Vector<EdgeWeight> fwd_weights;
    Vector<EdgeWeight> rev_weights;
    Vector<EdgeWeight> fwd_durations;
    Vector<EdgeWeight> rev_durations;
    Vector<DatasourceID> datasources;
};
}

using SegmentDataView = detail::SegmentDataContainerImpl<osrm::storage::MemorySetting::SharedMemory>;
using SegmentDataContainer = detail::SegmentDataContainerImpl<osrm::storage::MemorySetting::InternalMemory>;
}
}

#endif

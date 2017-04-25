#ifndef OSRM_EXTRACTOR_NODE_DATA_CONTAINER_HPP
#define OSRM_EXTRACTOR_NODE_DATA_CONTAINER_HPP

#include "storage/io_fwd.hpp"
#include "storage/shared_memory_ownership.hpp"

#include "util/typedefs.hpp"
#include "util/vector_view.hpp"

namespace osrm
{
namespace extractor
{
namespace detail
{
template <storage::Ownership Ownership> class EdgeBasedNodeDataContainerImpl;
}

namespace serialization
{
template <storage::Ownership Ownership>
void read(storage::io::FileReader &reader,
          detail::EdgeBasedNodeDataContainerImpl<Ownership> &ebn_data);

template <storage::Ownership Ownership>
void write(storage::io::FileWriter &writer,
           const detail::EdgeBasedNodeDataContainerImpl<Ownership> &ebn_data);
}

namespace detail
{
template <storage::Ownership Ownership> class EdgeBasedNodeDataContainerImpl
{
    template <typename T> using Vector = util::ViewOrVector<T, Ownership>;

  public:
    EdgeBasedNodeDataContainerImpl() = default;

    EdgeBasedNodeDataContainerImpl(Vector<GeometryID> geometry_ids,
                                   Vector<NameID> name_ids,
                                   Vector<extractor::TravelMode> travel_modes)
        : geometry_ids(std::move(geometry_ids)), name_ids(std::move(name_ids)),
          travel_modes(std::move(travel_modes))
    {
    }

    GeometryID GetGeometryID(const NodeID id) const { return geometry_ids[id]; }

    extractor::TravelMode GetTravelMode(const NodeID id) const { return travel_modes[id]; }

    NameID GetNameID(const NodeID id) const { return name_ids[id]; }

    // Used by EdgeBasedGraphFactory to fill data structure
    template <typename = std::enable_if<Ownership == storage::Ownership::Container>>
    void push_back(GeometryID geometry_id, NameID name_id, extractor::TravelMode travel_mode)
    {
        geometry_ids.push_back(geometry_id);
        name_ids.push_back(name_id);
        travel_modes.push_back(travel_mode);
    }

    friend void serialization::read<Ownership>(storage::io::FileReader &reader,
                                               EdgeBasedNodeDataContainerImpl &ebn_data_container);
    friend void
    serialization::write<Ownership>(storage::io::FileWriter &writer,
                                    const EdgeBasedNodeDataContainerImpl &ebn_data_container);

  private:
    Vector<GeometryID> geometry_ids;
    Vector<NameID> name_ids;
    Vector<extractor::TravelMode> travel_modes;
};
}

using EdgeBasedNodeDataExternalContainer =
    detail::EdgeBasedNodeDataContainerImpl<storage::Ownership::External>;
using EdgeBasedNodeDataContainer =
    detail::EdgeBasedNodeDataContainerImpl<storage::Ownership::Container>;
using EdgeBasedNodeDataView = detail::EdgeBasedNodeDataContainerImpl<storage::Ownership::View>;
}
}

#endif

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
    using TravelMode = extractor::TravelMode;

  public:
    EdgeBasedNodeDataContainerImpl() = default;

    EdgeBasedNodeDataContainerImpl(std::size_t size)
        : geometry_ids(size), name_ids(size), component_ids(size), travel_modes(size)
    {
    }

    EdgeBasedNodeDataContainerImpl(Vector<GeometryID> geometry_ids,
                                   Vector<NameID> name_ids,
                                   Vector<ComponentID> component_ids,
                                   Vector<TravelMode> travel_modes)
        : geometry_ids(std::move(geometry_ids)), name_ids(std::move(name_ids)),
          component_ids(std::move(component_ids)), travel_modes(std::move(travel_modes))
    {
    }

    GeometryID GetGeometryID(const NodeID node_id) const { return geometry_ids[node_id]; }

    TravelMode GetTravelMode(const NodeID node_id) const { return travel_modes[node_id]; }

    NameID GetNameID(const NodeID node_id) const { return name_ids[node_id]; }

    ComponentID GetComponentID(const NodeID node_id) const { return component_ids[node_id]; }

    // Used by EdgeBasedGraphFactory to fill data structure
    template <typename = std::enable_if<Ownership == storage::Ownership::Container>>
    void SetData(NodeID node_id, GeometryID geometry_id, NameID name_id, TravelMode travel_mode)
    {
        geometry_ids[node_id] = geometry_id;
        name_ids[node_id] = name_id;
        travel_modes[node_id] = travel_mode;
    }

    // Used by EdgeBasedGraphFactory to fill data structure
    template <typename = std::enable_if<Ownership == storage::Ownership::Container>>
    void SetData(NodeID node_id, ComponentID component_id)
    {
        component_ids[node_id] = component_id;
    }

    friend void serialization::read<Ownership>(storage::io::FileReader &reader,
                                               EdgeBasedNodeDataContainerImpl &ebn_data_container);
    friend void
    serialization::write<Ownership>(storage::io::FileWriter &writer,
                                    const EdgeBasedNodeDataContainerImpl &ebn_data_container);

  private:
    Vector<GeometryID> geometry_ids;
    Vector<NameID> name_ids;
    Vector<ComponentID> component_ids;
    Vector<TravelMode> travel_modes;
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

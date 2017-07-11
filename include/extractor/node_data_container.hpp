#ifndef OSRM_EXTRACTOR_NODE_DATA_CONTAINER_HPP
#define OSRM_EXTRACTOR_NODE_DATA_CONTAINER_HPP

#include "extractor/class_data.hpp"
#include "extractor/travel_mode.hpp"

#include "storage/io_fwd.hpp"
#include "storage/shared_memory_ownership.hpp"

#include "util/permutation.hpp"
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
        : geometry_ids(size), name_ids(size), component_ids(size), travel_modes(size), classes(size)
    {
    }

    EdgeBasedNodeDataContainerImpl(Vector<GeometryID> geometry_ids,
                                   Vector<NameID> name_ids,
                                   Vector<ComponentID> component_ids,
                                   Vector<TravelMode> travel_modes,
                                   Vector<ClassData> classes)
        : geometry_ids(std::move(geometry_ids)), name_ids(std::move(name_ids)),
          component_ids(std::move(component_ids)), travel_modes(std::move(travel_modes)),
          classes(std::move(classes))
    {
    }

    GeometryID GetGeometryID(const NodeID node_id) const { return geometry_ids[node_id]; }

    TravelMode GetTravelMode(const NodeID node_id) const { return travel_modes[node_id]; }

    NameID GetNameID(const NodeID node_id) const { return name_ids[node_id]; }

    ComponentID GetComponentID(const NodeID node_id) const { return component_ids[node_id]; }

    ClassData GetClassData(const NodeID node_id) const { return classes[node_id]; }

    // Used by EdgeBasedGraphFactory to fill data structure
    template <typename = std::enable_if<Ownership == storage::Ownership::Container>>
    void SetData(NodeID node_id,
                 GeometryID geometry_id,
                 NameID name_id,
                 TravelMode travel_mode,
                 ClassData class_data)
    {
        geometry_ids[node_id] = geometry_id;
        name_ids[node_id] = name_id;
        travel_modes[node_id] = travel_mode;
        classes[node_id] = class_data;
    }

    // Used by EdgeBasedGraphFactory to fill data structure
    template <typename = std::enable_if<Ownership == storage::Ownership::Container>>
    void SetComponentID(NodeID node_id, ComponentID component_id)
    {
        component_ids[node_id] = component_id;
    }

    friend void serialization::read<Ownership>(storage::io::FileReader &reader,
                                               EdgeBasedNodeDataContainerImpl &ebn_data_container);
    friend void
    serialization::write<Ownership>(storage::io::FileWriter &writer,
                                    const EdgeBasedNodeDataContainerImpl &ebn_data_container);

    template <typename = std::enable_if<Ownership == storage::Ownership::Container>>
    void Renumber(const std::vector<std::uint32_t> &permutation)
    {
        util::inplacePermutation(geometry_ids.begin(), geometry_ids.end(), permutation);
        util::inplacePermutation(name_ids.begin(), name_ids.end(), permutation);
        util::inplacePermutation(component_ids.begin(), component_ids.end(), permutation);
        util::inplacePermutation(travel_modes.begin(), travel_modes.end(), permutation);
        util::inplacePermutation(classes.begin(), classes.end(), permutation);
    }

    // all containers have the exact same size
    std::size_t Size() const
    {
        BOOST_ASSERT(geometry_ids.size() == name_ids.size());
        BOOST_ASSERT(geometry_ids.size() == component_ids.size());
        BOOST_ASSERT(geometry_ids.size() == travel_modes.size());
        BOOST_ASSERT(geometry_ids.size() == classes.size());
        return geometry_ids.size();
    }

  private:
    Vector<GeometryID> geometry_ids;
    Vector<NameID> name_ids;
    Vector<ComponentID> component_ids;
    Vector<TravelMode> travel_modes;
    Vector<ClassData> classes;
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

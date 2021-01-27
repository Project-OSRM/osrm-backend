#ifndef OSRM_EXTRACTOR_NODE_DATA_CONTAINER_HPP
#define OSRM_EXTRACTOR_NODE_DATA_CONTAINER_HPP

#include "extractor/class_data.hpp"
#include "extractor/edge_based_node.hpp"
#include "extractor/node_based_edge.hpp"
#include "extractor/travel_mode.hpp"

#include "storage/shared_memory_ownership.hpp"
#include "storage/tar_fwd.hpp"

#include "util/permutation.hpp"
#include "util/typedefs.hpp"
#include "util/vector_view.hpp"

namespace osrm
{
namespace extractor
{

class Extractor;
class EdgeBasedGraphFactory;

namespace detail
{
template <storage::Ownership Ownership> class EdgeBasedNodeDataContainerImpl;
}

namespace serialization
{
template <storage::Ownership Ownership>
void read(storage::tar::FileReader &reader,
          const std::string &name,
          detail::EdgeBasedNodeDataContainerImpl<Ownership> &ebn_data);

template <storage::Ownership Ownership>
void write(storage::tar::FileWriter &writer,
           const std::string &name,
           const detail::EdgeBasedNodeDataContainerImpl<Ownership> &ebn_data);
} // namespace serialization

namespace detail
{
template <storage::Ownership Ownership> class EdgeBasedNodeDataContainerImpl
{
    template <typename T> using Vector = util::ViewOrVector<T, Ownership>;
    using TravelMode = extractor::TravelMode;

    // to fill in data on edgeBasedNodes
    friend class osrm::extractor::Extractor;
    friend class osrm::extractor::EdgeBasedGraphFactory;

  public:
    EdgeBasedNodeDataContainerImpl() = default;

    EdgeBasedNodeDataContainerImpl(const NodeID number_of_edge_based_nodes,
                                   const AnnotationID number_of_annotations)
        : nodes(number_of_edge_based_nodes), annotation_data(number_of_annotations)
    {
    }

    EdgeBasedNodeDataContainerImpl(Vector<EdgeBasedNode> nodes,
                                   Vector<NodeBasedEdgeAnnotation> annotation_data)
        : nodes(std::move(nodes)), annotation_data(std::move(annotation_data))
    {
    }

    GeometryID GetGeometryID(const NodeID node_id) const { return nodes[node_id].geometry_id; }

    ComponentID GetComponentID(const NodeID node_id) const { return nodes[node_id].component_id; }

    TravelMode GetTravelMode(const NodeID node_id) const
    {
        return annotation_data[nodes[node_id].annotation_id].travel_mode;
    }

    bool IsLeftHandDriving(const NodeID node_id) const
    {
        return annotation_data[nodes[node_id].annotation_id].is_left_hand_driving;
    }

    bool IsSegregated(const NodeID node_id) const { return nodes[node_id].segregated; }

    NameID GetNameID(const NodeID node_id) const
    {
        return annotation_data[nodes[node_id].annotation_id].name_id;
    }

    ClassData GetClassData(const NodeID node_id) const
    {
        return annotation_data[nodes[node_id].annotation_id].classes;
    }

    friend void serialization::read<Ownership>(storage::tar::FileReader &reader,
                                               const std::string &name,
                                               EdgeBasedNodeDataContainerImpl &ebn_data_container);
    friend void
    serialization::write<Ownership>(storage::tar::FileWriter &writer,
                                    const std::string &name,
                                    const EdgeBasedNodeDataContainerImpl &ebn_data_container);

    template <typename = std::enable_if<Ownership == storage::Ownership::Container>>
    void Renumber(const std::vector<std::uint32_t> &permutation)
    {
        util::inplacePermutation(nodes.begin(), nodes.end(), permutation);
    }

    NodeID NumberOfNodes() const { return nodes.size(); }

    // the number of annotations differs from the number of nodes, since annotations can be shared
    // between a large set of nodes
    AnnotationID NumberOfAnnotations() const { return annotation_data.size(); }

    EdgeBasedNode const &GetNode(const NodeID node_id) const { return nodes[node_id]; }

    NodeBasedEdgeAnnotation const &GetAnnotation(const AnnotationID annotation) const
    {
        return annotation_data[annotation];
    }

  private:
    Vector<EdgeBasedNode> nodes;
    Vector<NodeBasedEdgeAnnotation> annotation_data;
};
} // namespace detail

using EdgeBasedNodeDataExternalContainer =
    detail::EdgeBasedNodeDataContainerImpl<storage::Ownership::External>;
using EdgeBasedNodeDataContainer =
    detail::EdgeBasedNodeDataContainerImpl<storage::Ownership::Container>;
using EdgeBasedNodeDataView = detail::EdgeBasedNodeDataContainerImpl<storage::Ownership::View>;
} // namespace extractor
} // namespace osrm

#endif

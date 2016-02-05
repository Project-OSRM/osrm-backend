#ifndef DATAFACADE_BASE_HPP
#define DATAFACADE_BASE_HPP

// Exposes all data access interfaces to the algorithms via base class ptr

#include "extractor/edge_based_node.hpp"
#include "extractor/external_memory_node.hpp"
#include "engine/phantom_node.hpp"
#include "extractor/turn_instructions.hpp"
#include "util/integer_range.hpp"
#include "util/osrm_exception.hpp"
#include "util/string_util.hpp"
#include "util/typedefs.hpp"

#include "osrm/coordinate.hpp"

#include <cstddef>

#include <vector>
#include <utility>
#include <string>

namespace osrm
{
namespace engine
{
namespace datafacade
{

using EdgeRange = util::range<EdgeID>;

template <class EdgeDataT> class BaseDataFacade
{
  public:
    using RTreeLeaf = extractor::EdgeBasedNode;
    using EdgeData = EdgeDataT;
    BaseDataFacade() {}
    virtual ~BaseDataFacade() {}

    // search graph access
    virtual unsigned GetNumberOfNodes() const = 0;

    virtual unsigned GetNumberOfEdges() const = 0;

    virtual unsigned GetOutDegree(const NodeID n) const = 0;

    virtual NodeID GetTarget(const EdgeID e) const = 0;

    virtual const EdgeDataT &GetEdgeData(const EdgeID e) const = 0;

    virtual EdgeID BeginEdges(const NodeID n) const = 0;

    virtual EdgeID EndEdges(const NodeID n) const = 0;

    virtual EdgeRange GetAdjacentEdgeRange(const NodeID node) const = 0;

    // searches for a specific edge
    virtual EdgeID FindEdge(const NodeID from, const NodeID to) const = 0;

    virtual EdgeID FindEdgeInEitherDirection(const NodeID from, const NodeID to) const = 0;

    virtual EdgeID
    FindEdgeIndicateIfReverse(const NodeID from, const NodeID to, bool &result) const = 0;

    // node and edge information access
    virtual util::FixedPointCoordinate GetCoordinateOfNode(const unsigned id) const = 0;

    virtual bool EdgeIsCompressed(const unsigned id) const = 0;

    virtual unsigned GetGeometryIndexForEdgeID(const unsigned id) const = 0;

    virtual void GetUncompressedGeometry(const unsigned id,
                                         std::vector<unsigned> &result_nodes) const = 0;

    virtual extractor::TurnInstruction GetTurnInstructionForEdgeID(const unsigned id) const = 0;

    virtual extractor::TravelMode GetTravelModeForEdgeID(const unsigned id) const = 0;

    virtual std::vector<PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::FixedPointCoordinate input_coordinate,
                               const float max_distance,
                               const int bearing = 0,
                               const int bearing_range = 180) = 0;

    virtual std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::FixedPointCoordinate input_coordinate,
                        const unsigned max_results,
                        const int bearing = 0,
                        const int bearing_range = 180) = 0;

    virtual std::pair<PhantomNode, PhantomNode> NearestPhantomNodeWithAlternativeFromBigComponent(
        const util::FixedPointCoordinate input_coordinate,
        const int bearing = 0,
        const int bearing_range = 180) = 0;

    virtual unsigned GetCheckSum() const = 0;

    virtual bool IsCoreNode(const NodeID id) const = 0;

    virtual unsigned GetNameIndexFromEdgeID(const unsigned id) const = 0;

    virtual std::string get_name_for_id(const unsigned name_id) const = 0;

    virtual std::size_t GetCoreSize() const = 0;

    virtual std::string GetTimestamp() const = 0;
};
}
}
}

#endif // DATAFACADE_BASE_HPP

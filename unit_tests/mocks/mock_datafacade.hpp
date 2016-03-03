#ifndef MOCK_DATAFACADE_HPP
#define MOCK_DATAFACADE_HPP

// implements all data storage when shared memory _IS_ used

#include "engine/datafacade/datafacade_base.hpp"
#include "contractor/query_edge.hpp"

namespace osrm
{
namespace test
{

template <class EdgeDataT>
class MockDataFacadeT final : public osrm::engine::datafacade::BaseDataFacade<EdgeDataT>
{
  private:
    EdgeDataT foo;

  public:
    unsigned GetNumberOfNodes() const { return 0; }
    unsigned GetNumberOfEdges() const { return 0; }
    unsigned GetOutDegree(const NodeID /* n */) const { return 0; }
    NodeID GetTarget(const EdgeID /* e */) const { return SPECIAL_NODEID; }
    const EdgeDataT &GetEdgeData(const EdgeID /* e */) const { return foo; }
    EdgeID BeginEdges(const NodeID /* n */) const { return SPECIAL_EDGEID; }
    EdgeID EndEdges(const NodeID /* n */) const { return SPECIAL_EDGEID; }
    osrm::engine::datafacade::EdgeRange GetAdjacentEdgeRange(const NodeID /* node */) const
    {
        return util::irange(static_cast<EdgeID>(0), static_cast<EdgeID>(0));
    }
    EdgeID FindEdge(const NodeID /* from */, const NodeID /* to */) const { return SPECIAL_EDGEID; }
    EdgeID FindEdgeInEitherDirection(const NodeID /* from */, const NodeID /* to */) const
    {
        return SPECIAL_EDGEID;
    }
    EdgeID FindEdgeIndicateIfReverse(const NodeID /* from */,
                                     const NodeID /* to */,
                                     bool & /* result */) const
    {
        return SPECIAL_EDGEID;
    }
    util::FixedPointCoordinate GetCoordinateOfNode(const unsigned /* id */) const
    {
        FixedPointCoordinate foo(0, 0);
        return foo;
    }
    bool EdgeIsCompressed(const unsigned /* id */) const { return false; }
    unsigned GetGeometryIndexForEdgeID(const unsigned /* id */) const { return SPECIAL_NODEID; }
    void GetUncompressedGeometry(const EdgeID /* id */,
                                 std::vector<NodeID> & /* result_nodes */) const
    {
    }
    void GetUncompressedWeights(const EdgeID /* id */,
                                std::vector<EdgeWeight> & /* result_weights */) const
    {
    }
    extractor::TurnInstruction GetTurnInstructionForEdgeID(const unsigned /* id */) const
    {
        return osrm::extractor::TurnInstruction::NoTurn;
    }
    extractor::TravelMode GetTravelModeForEdgeID(const unsigned /* id */) const
    {
        return TRAVEL_MODE_DEFAULT;
    }
    std::vector<typename osrm::engine::datafacade::BaseDataFacade<EdgeDataT>::RTreeLeaf>
    GetEdgesInBox(const util::FixedPointCoordinate & /* south_west */,
                  const util::FixedPointCoordinate & /*north_east */)
    {
        std::vector<typename osrm::engine::datafacade::BaseDataFacade<EdgeDataT>::RTreeLeaf> foo;
        return foo;
    }
    std::vector<osrm::engine::PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::FixedPointCoordinate /* input_coordinate */,
                               const float /* max_distance */,
                               const int /* bearing = 0 */,
                               const int /* bearing_range = 180 */)
    {
        std::vector<osrm::engine::PhantomNodeWithDistance> foo;
        return foo;
    }
    std::vector<osrm::engine::PhantomNodeWithDistance>
    NearestPhantomNodes(const util::FixedPointCoordinate /* input_coordinate */,
                        const unsigned /* max_results */,
                        const int /* bearing = 0 */,
                        const int /* bearing_range = 180 */)
    {
        std::vector<osrm::engine::PhantomNodeWithDistance> foo;
        return foo;
    }
    std::pair<osrm::engine::PhantomNode, osrm::engine::PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(
        const util::FixedPointCoordinate /* input_coordinate */,
        const int /* bearing = 0 */,
        const int /* bearing_range = 180 */)
    {
        std::pair<osrm::engine::PhantomNode, osrm::engine::PhantomNode> foo;
        return foo;
    }
    unsigned GetCheckSum() const { return 0; }
    bool IsCoreNode(const NodeID /* id */) const { return false; }
    unsigned GetNameIndexFromEdgeID(const unsigned /* id */) const { return 0; }
    std::string get_name_for_id(const unsigned /* name_id */) const { return ""; }
    std::size_t GetCoreSize() const { return 0; }
    std::string GetTimestamp() const { return ""; }
};

using MockDataFacade = MockDataFacadeT<contractor::QueryEdge::EdgeData>;
} // osrm::test::
} // osrm::

#endif // MOCK_DATAFACADE_HPP

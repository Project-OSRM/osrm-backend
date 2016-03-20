#ifndef MOCK_DATAFACADE_HPP
#define MOCK_DATAFACADE_HPP

// implements all data storage when shared memory _IS_ used

#include "extractor/guidance/turn_instruction.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "contractor/query_edge.hpp"

namespace osrm
{
namespace test
{

class MockDataFacade final : public engine::datafacade::BaseDataFacade
{
  private:
    EdgeData foo;

  public:
    unsigned GetNumberOfNodes() const { return 0; }
    unsigned GetNumberOfEdges() const { return 0; }
    unsigned GetOutDegree(const NodeID /* n */) const { return 0; }
    NodeID GetTarget(const EdgeID /* e */) const { return SPECIAL_NODEID; }
    const EdgeData &GetEdgeData(const EdgeID /* e */) const { return foo; }
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
    util::Coordinate GetCoordinateOfNode(const unsigned /* id */) const
    {
        return {util::FixedLongitude{0}, util::FixedLatitude{0}};
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
    void GetUncompressedDatasources(const EdgeID /*id*/,
                                    std::vector<uint8_t> & /*data_sources*/) const
    {
    }
    std::string GetDatasourceName(const uint8_t /*datasource_name_id*/) const { return ""; }
    extractor::guidance::TurnInstruction GetTurnInstructionForEdgeID(const unsigned /* id */) const
    {
        return extractor::guidance::TurnInstruction::NO_TURN();
    }
    extractor::TravelMode GetTravelModeForEdgeID(const unsigned /* id */) const
    {
        return TRAVEL_MODE_INACCESSIBLE;
    }
    std::vector<RTreeLeaf> GetEdgesInBox(const util::Coordinate /* south_west */,
                                         const util::Coordinate /*north_east */)
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate /*input_coordinate*/,
                               const float /*max_distance*/,
                               const int /*bearing*/,
                               const int /*bearing_range*/)
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate /*input_coordinate*/,
                               const float /*max_distance*/)
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate /*input_coordinate*/,
                        const unsigned /*max_results*/,
                        const double /*max_distance*/,
                        const int /*bearing*/,
                        const int /*bearing_range*/)
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate /*input_coordinate*/,
                        const unsigned /*max_results*/,
                        const int /*bearing*/,
                        const int /*bearing_range*/)
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate /*input_coordinate*/, const unsigned /*max_results*/)
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate /*input_coordinate*/,
                        const unsigned /*max_results*/,
                        const double /*max_distance*/)
    {
        return {};
    }

    std::pair<engine::PhantomNode, engine::PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate /*input_coordinate*/)
    {
        return {};
    }

    std::pair<engine::PhantomNode, engine::PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate /*input_coordinate*/,
                                                      const double /*max_distance*/)
    {
        return {};
    }

    std::pair<engine::PhantomNode, engine::PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate /*input_coordinate*/,
                                                      const double /*max_distance*/,
                                                      const int /*bearing*/,
                                                      const int /*bearing_range*/)
    {
        return {};
    }

    std::pair<engine::PhantomNode, engine::PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate /*input_coordinate*/,
                                                      const int /*bearing*/,
                                                      const int /*bearing_range*/)
    {
        return {};
    };

    unsigned GetCheckSum() const { return 0; }
    bool IsCoreNode(const NodeID /* id */) const { return false; }
    unsigned GetNameIndexFromEdgeID(const unsigned /* id */) const { return 0; }
    std::string GetNameForID(const unsigned /* name_id */) const { return ""; }
    std::size_t GetCoreSize() const { return 0; }
    std::string GetTimestamp() const { return ""; }
    bool GetUTurnsDefault() const override { return true; }
};
} // ns test
} // ns osrm

#endif // MOCK_DATAFACADE_HPP

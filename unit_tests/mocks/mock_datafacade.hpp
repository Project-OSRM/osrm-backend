#ifndef MOCK_DATAFACADE_HPP
#define MOCK_DATAFACADE_HPP

// implements all data storage when shared memory _IS_ used

#include "contractor/query_edge.hpp"
#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/guidance/turn_lane_types.hpp"
#include "engine/algorithm.hpp"
#include "engine/datafacade/algorithm_datafacade.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/guidance/turn_bearing.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace test
{

class MockBaseDataFacade : public engine::datafacade::BaseDataFacade
{
    using StringView = util::StringView;

  public:
    util::Coordinate GetCoordinateOfNode(const NodeID /* id */) const override
    {
        return {util::FixedLongitude{0}, util::FixedLatitude{0}};
    }
    OSMNodeID GetOSMNodeIDOfNode(const NodeID /* id */) const override { return OSMNodeID{0}; }
    bool EdgeIsCompressed(const EdgeID /* id */) const { return false; }
    GeometryID GetGeometryIndexForEdgeID(const EdgeID /* id */) const override
    {
        return GeometryID{SPECIAL_GEOMETRYID, false};
    }
    TurnPenalty GetWeightPenaltyForEdgeID(const unsigned /* id */) const override final
    {
        return 0;
    }
    TurnPenalty GetDurationPenaltyForEdgeID(const unsigned /* id */) const override final
    {
        return 0;
    }
    std::vector<NodeID> GetUncompressedForwardGeometry(const EdgeID /* id */) const override
    {
        return {};
    }
    std::vector<NodeID> GetUncompressedReverseGeometry(const EdgeID /* id */) const override
    {
        return {};
    }
    std::vector<EdgeWeight> GetUncompressedForwardWeights(const EdgeID /* id */) const override
    {
        std::vector<EdgeWeight> result_weights;
        result_weights.resize(1);
        result_weights[0] = 1;
        return result_weights;
    }
    std::vector<EdgeWeight> GetUncompressedReverseWeights(const EdgeID id) const override
    {
        return GetUncompressedForwardWeights(id);
    }
    std::vector<EdgeWeight> GetUncompressedForwardDurations(const EdgeID id) const override
    {
        return GetUncompressedForwardWeights(id);
    }
    std::vector<EdgeWeight> GetUncompressedReverseDurations(const EdgeID id) const override
    {
        return GetUncompressedForwardWeights(id);
    }
    std::vector<DatasourceID> GetUncompressedForwardDatasources(const EdgeID /*id*/) const override
    {
        return {};
    }
    std::vector<DatasourceID> GetUncompressedReverseDatasources(const EdgeID /*id*/) const override
    {
        return {};
    }

    StringView GetDatasourceName(const DatasourceID) const override final { return {}; }

    extractor::guidance::TurnInstruction
    GetTurnInstructionForEdgeID(const EdgeID /* id */) const override
    {
        return extractor::guidance::TurnInstruction::NO_TURN();
    }
    extractor::TravelMode GetTravelModeForEdgeID(const EdgeID /* id */) const override
    {
        return TRAVEL_MODE_INACCESSIBLE;
    }
    std::vector<RTreeLeaf> GetEdgesInBox(const util::Coordinate /* south_west */,
                                         const util::Coordinate /*north_east */) const override
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate /*input_coordinate*/,
                               const float /*max_distance*/,
                               const int /*bearing*/,
                               const int /*bearing_range*/) const override
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate /*input_coordinate*/,
                               const float /*max_distance*/) const override
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate /*input_coordinate*/,
                        const unsigned /*max_results*/,
                        const double /*max_distance*/,
                        const int /*bearing*/,
                        const int /*bearing_range*/) const override
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate /*input_coordinate*/,
                        const unsigned /*max_results*/,
                        const int /*bearing*/,
                        const int /*bearing_range*/) const override
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate /*input_coordinate*/,
                        const unsigned /*max_results*/) const override
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate /*input_coordinate*/,
                        const unsigned /*max_results*/,
                        const double /*max_distance*/) const override
    {
        return {};
    }

    std::pair<engine::PhantomNode, engine::PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(
        const util::Coordinate /*input_coordinate*/) const override
    {
        return {};
    }

    std::pair<engine::PhantomNode, engine::PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate /*input_coordinate*/,
                                                      const double /*max_distance*/) const override
    {
        return {};
    }

    std::pair<engine::PhantomNode, engine::PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate /*input_coordinate*/,
                                                      const double /*max_distance*/,
                                                      const int /*bearing*/,
                                                      const int /*bearing_range*/) const override
    {
        return {};
    }

    std::pair<engine::PhantomNode, engine::PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate /*input_coordinate*/,
                                                      const int /*bearing*/,
                                                      const int /*bearing_range*/) const override
    {
        return {};
    }

    unsigned GetCheckSum() const override { return 0; }

    NameID GetNameIndexFromEdgeID(const EdgeID /* id */) const override { return 0; }

    StringView GetNameForID(const NameID) const override final { return {}; }
    StringView GetRefForID(const NameID) const override final { return {}; }
    StringView GetPronunciationForID(const NameID) const override final { return {}; }
    StringView GetDestinationsForID(const NameID) const override final { return {}; }

    std::string GetTimestamp() const override { return ""; }
    bool GetContinueStraightDefault() const override { return true; }
    double GetMapMatchingMaxSpeed() const override { return 180 / 3.6; }
    const char *GetWeightName() const override final { return "duration"; }
    unsigned GetWeightPrecision() const override final { return 1; }
    double GetWeightMultiplier() const override final { return 10.; }
    BearingClassID GetBearingClassID(const NodeID /*id*/) const override { return 0; }
    EntryClassID GetEntryClassID(const EdgeID /*id*/) const override { return 0; }

    util::guidance::TurnBearing PreTurnBearing(const EdgeID /*eid*/) const override final
    {
        return util::guidance::TurnBearing{0.0};
    }
    util::guidance::TurnBearing PostTurnBearing(const EdgeID /*eid*/) const override final
    {
        return util::guidance::TurnBearing{0.0};
    }

    bool HasLaneData(const EdgeID /*id*/) const override final { return true; };
    util::guidance::LaneTupleIdPair GetLaneData(const EdgeID /*id*/) const override final
    {
        return {{0, 0}, 0};
    }
    extractor::guidance::TurnLaneDescription
    GetTurnDescription(const LaneDescriptionID /*lane_description_id*/) const override final
    {
        return {};
    }

    util::guidance::BearingClass
    GetBearingClass(const BearingClassID /*bearing_class_id*/) const override
    {
        util::guidance::BearingClass result;
        result.add(0);
        result.add(90);
        result.add(180);
        result.add(270);
        return result;
    }

    util::guidance::EntryClass GetEntryClass(const EntryClassID /*entry_class_id*/) const override
    {
        util::guidance::EntryClass result;
        result.activate(1);
        result.activate(2);
        result.activate(3);
        return result;
    }
};

template <typename AlgorithmT> class MockAlgorithmDataFacade;

template <>
class MockAlgorithmDataFacade<engine::datafacade::CH>
    : public engine::datafacade::AlgorithmDataFacade<engine::datafacade::CH>
{
  private:
    EdgeData foo;

  public:
    unsigned GetNumberOfNodes() const override { return 0; }
    unsigned GetNumberOfEdges() const override { return 0; }
    unsigned GetOutDegree(const NodeID /* n */) const override { return 0; }
    NodeID GetTarget(const EdgeID /* e */) const override { return SPECIAL_NODEID; }
    const EdgeData &GetEdgeData(const EdgeID /* e */) const override { return foo; }
    EdgeID BeginEdges(const NodeID /* n */) const override { return SPECIAL_EDGEID; }
    EdgeID EndEdges(const NodeID /* n */) const override { return SPECIAL_EDGEID; }
    osrm::engine::datafacade::EdgeRange GetAdjacentEdgeRange(const NodeID /* node */) const override
    {
        return util::irange(static_cast<EdgeID>(0), static_cast<EdgeID>(0));
    }
    EdgeID FindEdge(const NodeID /* from */, const NodeID /* to */) const override
    {
        return SPECIAL_EDGEID;
    }
    EdgeID FindEdgeInEitherDirection(const NodeID /* from */, const NodeID /* to */) const override
    {
        return SPECIAL_EDGEID;
    }

    EdgeID FindSmallestEdge(const NodeID /* from */,
                            const NodeID /* to */,
                            std::function<bool(EdgeData)> /* filter */) const override
    {
        return SPECIAL_EDGEID;
    }

    EdgeID FindEdgeIndicateIfReverse(const NodeID /* from */,
                                     const NodeID /* to */,
                                     bool & /* result */) const override
    {
        return SPECIAL_EDGEID;
    }
};

template <>
class MockAlgorithmDataFacade<engine::datafacade::CoreCH>
    : public engine::datafacade::AlgorithmDataFacade<engine::datafacade::CoreCH>
{
  private:
    EdgeData foo;

  public:
    bool IsCoreNode(const NodeID /* id */) const override { return false; }
};

template <typename AlgorithmT>
class MockDataFacade final : public MockBaseDataFacade, public MockAlgorithmDataFacade<AlgorithmT>
{
};

template <>
class MockDataFacade<engine::datafacade::CoreCH> final
    : public MockBaseDataFacade,
      public MockAlgorithmDataFacade<engine::datafacade::CH>,
      public MockAlgorithmDataFacade<engine::datafacade::CoreCH>
{
};

} // ns test
} // ns osrm

#endif // MOCK_DATAFACADE_HPP

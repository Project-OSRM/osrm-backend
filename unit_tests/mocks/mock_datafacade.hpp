#ifndef MOCK_DATAFACADE_HPP
#define MOCK_DATAFACADE_HPP

// implements all data storage when shared memory _IS_ used

#include "contractor/query_edge.hpp"
#include "extractor/class_data.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/travel_mode.hpp"
#include "extractor/turn_lane_types.hpp"
#include "guidance/turn_bearing.hpp"
#include "guidance/turn_instruction.hpp"

#include "engine/algorithm.hpp"
#include "engine/datafacade/algorithm_datafacade.hpp"
#include "engine/datafacade/datafacade_base.hpp"

#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace test
{

class MockBaseDataFacade : public engine::datafacade::BaseDataFacade
{
    using StringView = util::StringView;

  public:
    bool ExcludeNode(const NodeID) const override { return false; };

    util::Coordinate GetCoordinateOfNode(const NodeID /* id */) const override
    {
        return {util::FixedLongitude{0}, util::FixedLatitude{0}};
    }
    OSMNodeID GetOSMNodeIDOfNode(const NodeID /* id */) const override { return OSMNodeID{0}; }
    bool EdgeIsCompressed(const EdgeID /* id */) const { return false; }
    GeometryID GetGeometryIndex(const NodeID /* id */) const override
    {
        return GeometryID{SPECIAL_GEOMETRYID, false};
    }
    ComponentID GetComponentID(const NodeID /* id */) const override
    {
        return ComponentID{INVALID_COMPONENTID, false};
    }
    TurnPenalty GetWeightPenaltyForEdgeID(const unsigned /* id */) const override final
    {
        return 0;
    }
    TurnPenalty GetDurationPenaltyForEdgeID(const unsigned /* id */) const override final
    {
        return 0;
    }
    std::string GetTimestamp() const override { return ""; }
    NodeForwardRange GetUncompressedForwardGeometry(const EdgeID /* id */) const override
    {
        static NodeID data[] = {0, 1, 2, 3};
        static extractor::SegmentDataView::SegmentNodeVector nodes(data, 4);
        return boost::make_iterator_range(nodes.cbegin(), nodes.cend());
    }
    NodeReverseRange GetUncompressedReverseGeometry(const EdgeID id) const override
    {
        return NodeReverseRange(GetUncompressedForwardGeometry(id));
    }
    WeightForwardRange GetUncompressedForwardWeights(const EdgeID /* id */) const override
    {
        static std::uint64_t data[] = {1, 2, 3};
        static const extractor::SegmentDataView::SegmentWeightVector weights(
            util::vector_view<std::uint64_t>(data, 3), 3);
        return WeightForwardRange(weights.begin(), weights.end());
    }
    WeightReverseRange GetUncompressedReverseWeights(const EdgeID id) const override
    {
        return WeightReverseRange(GetUncompressedForwardWeights(id));
    }
    DurationForwardRange GetUncompressedForwardDurations(const EdgeID /*id*/) const override
    {
        static std::uint64_t data[] = {1, 2, 3};
        static const extractor::SegmentDataView::SegmentDurationVector durations(
            util::vector_view<std::uint64_t>(data, 3), 3);
        return DurationForwardRange(durations.begin(), durations.end());
    }
    DurationReverseRange GetUncompressedReverseDurations(const EdgeID id) const override
    {
        return DurationReverseRange(GetUncompressedForwardDurations(id));
    }
    DatasourceForwardRange GetUncompressedForwardDatasources(const EdgeID /*id*/) const override
    {
        return {};
    }
    DatasourceReverseRange GetUncompressedReverseDatasources(const EdgeID /*id*/) const override
    {
        return DatasourceReverseRange(DatasourceForwardRange());
    }

    StringView GetDatasourceName(const DatasourceID) const override final { return {}; }

    osrm::guidance::TurnInstruction
    GetTurnInstructionForEdgeID(const EdgeID /* id */) const override
    {
        return osrm::guidance::TurnInstruction::NO_TURN();
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
                               const int /*bearing_range*/,
                               const engine::Approach /*approach*/,
                               const bool /*use_all_edges*/) const override
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate /*input_coordinate*/,
                               const float /*max_distance*/,
                               const engine::Approach /*approach*/,
                               const bool /*use_all_edges*/) const override
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate /*input_coordinate*/,
                        const unsigned /*max_results*/,
                        const double /*max_distance*/,
                        const int /*bearing*/,
                        const int /*bearing_range*/,
                        const engine::Approach /*approach*/) const override
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate /*input_coordinate*/,
                        const unsigned /*max_results*/,
                        const int /*bearing*/,
                        const int /*bearing_range*/,
                        const engine::Approach /*approach*/) const override
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate /*input_coordinate*/,
                        const unsigned /*max_results*/,
                        const engine::Approach /*approach*/) const override
    {
        return {};
    }

    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate /*input_coordinate*/,
                        const unsigned /*max_results*/,
                        const double /*max_distance*/,
                        const engine::Approach /*approach*/) const override
    {
        return {};
    }

    std::pair<engine::PhantomNode, engine::PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(
        const util::Coordinate /*input_coordinate*/,
        const engine::Approach /*approach*/) const override
    {
        return {};
    }

    std::pair<engine::PhantomNode, engine::PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(
        const util::Coordinate /*input_coordinate*/,
        const double /*max_distance*/,
        const engine::Approach /*approach*/) const override
    {
        return {};
    }

    std::pair<engine::PhantomNode, engine::PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(
        const util::Coordinate /*input_coordinate*/,
        const double /*max_distance*/,
        const int /*bearing*/,
        const int /*bearing_range*/,
        const engine::Approach /*approach*/) const override
    {
        return {};
    }

    std::pair<engine::PhantomNode, engine::PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(
        const util::Coordinate /*input_coordinate*/,
        const int /*bearing*/,
        const int /*bearing_range*/,
        const engine::Approach /*approach*/) const override
    {
        return {};
    }

    std::uint32_t GetCheckSum() const override { return 0; }

    extractor::TravelMode GetTravelMode(const NodeID /* id */) const override
    {
        return extractor::TRAVEL_MODE_INACCESSIBLE;
    }

    extractor::ClassData GetClassData(const NodeID /*id*/) const override final { return 0; }

    std::vector<std::string> GetClasses(const extractor::ClassData /*data*/) const override final
    {
        return {};
    }

    NameID GetNameIndex(const NodeID /* id */) const override { return 0; }

    StringView GetNameForID(const NameID) const override final { return {}; }
    StringView GetRefForID(const NameID) const override final { return {}; }
    StringView GetPronunciationForID(const NameID) const override final { return {}; }
    StringView GetDestinationsForID(const NameID) const override final { return {}; }
    StringView GetExitsForID(const NameID) const override final { return {}; }

    bool GetContinueStraightDefault() const override { return true; }
    double GetMapMatchingMaxSpeed() const override { return 180 / 3.6; }
    const char *GetWeightName() const override final { return "duration"; }
    unsigned GetWeightPrecision() const override final { return 1; }
    double GetWeightMultiplier() const override final { return 10.; }
    bool IsLeftHandDriving(const NodeID /*id*/) const override { return false; }
    bool IsSegregated(const NodeID /*id*/) const override { return false; }

    guidance::TurnBearing PreTurnBearing(const EdgeID /*eid*/) const override final
    {
        return guidance::TurnBearing{0.0};
    }
    guidance::TurnBearing PostTurnBearing(const EdgeID /*eid*/) const override final
    {
        return guidance::TurnBearing{0.0};
    }

    bool HasLaneData(const EdgeID /*id*/) const override final { return true; };
    util::guidance::LaneTupleIdPair GetLaneData(const EdgeID /*id*/) const override final
    {
        return {{0, 0}, 0};
    }
    extractor::TurnLaneDescription
    GetTurnDescription(const LaneDescriptionID /*lane_description_id*/) const override final
    {
        return {};
    }

    util::guidance::BearingClass GetBearingClass(const NodeID /*node*/) const override
    {
        util::guidance::BearingClass result;
        result.add(0);
        result.add(90);
        result.add(180);
        result.add(270);
        return result;
    }

    util::guidance::EntryClass GetEntryClass(const EdgeID /*id*/) const override
    {
        util::guidance::EntryClass result;
        result.activate(1);
        result.activate(2);
        result.activate(3);
        return result;
    }

    std::vector<extractor::ManeuverOverride>
    GetOverridesThatStartAt(const NodeID /* edge_based_node_id */) const override
    {
        return {};
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
    EdgeRange GetAdjacentEdgeRange(const NodeID /* node */) const override
    {
        return EdgeRange(static_cast<EdgeID>(0), static_cast<EdgeID>(0), {});
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

template <typename AlgorithmT>
class MockDataFacade final : public MockBaseDataFacade, public MockAlgorithmDataFacade<AlgorithmT>
{
};

} // ns test
} // ns osrm

#endif // MOCK_DATAFACADE_HPP

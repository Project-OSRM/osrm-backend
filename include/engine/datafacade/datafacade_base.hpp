#ifndef DATAFACADE_BASE_HPP
#define DATAFACADE_BASE_HPP

// Exposes all data access interfaces to the algorithms via base class ptr

#include "engine/approach.hpp"
#include "engine/phantom_node.hpp"

#include "contractor/query_edge.hpp"

#include "extractor/class_data.hpp"
#include "extractor/edge_based_node_segment.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/query_node.hpp"
#include "extractor/segment_data_container.hpp"
#include "extractor/travel_mode.hpp"
#include "extractor/turn_lane_types.hpp"

#include "guidance/turn_bearing.hpp"
#include "guidance/turn_instruction.hpp"

#include "util/exception.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/guidance/turn_lanes.hpp"
#include "util/integer_range.hpp"
#include "util/packed_vector.hpp"
#include "util/string_util.hpp"
#include "util/typedefs.hpp"

#include "osrm/coordinate.hpp"

#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/any_range.hpp>
#include <cstddef>

#include <engine/bearing.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace osrm::engine::datafacade
{

class BaseDataFacade
{
  public:
    using RTreeLeaf = extractor::EdgeBasedNodeSegment;

    using NodeForwardRange =
        boost::iterator_range<extractor::SegmentDataView::SegmentNodeVector::const_iterator>;
    using NodeReverseRange = boost::reversed_range<const NodeForwardRange>;

    using WeightForwardRange =
        boost::iterator_range<extractor::SegmentDataView::SegmentWeightVector::const_iterator>;
    using WeightReverseRange = boost::reversed_range<const WeightForwardRange>;

    using DurationForwardRange =
        boost::iterator_range<extractor::SegmentDataView::SegmentDurationVector::const_iterator>;
    using DurationReverseRange = boost::reversed_range<const DurationForwardRange>;

    using DatasourceForwardRange =
        boost::iterator_range<extractor::SegmentDataView::SegmentDatasourceVector::const_iterator>;
    using DatasourceReverseRange = boost::reversed_range<const DatasourceForwardRange>;

    BaseDataFacade() {}
    virtual ~BaseDataFacade() {}

    virtual std::uint32_t GetCheckSum() const = 0;

    virtual std::string GetTimestamp() const = 0;

    // node and edge information access
    virtual util::Coordinate GetCoordinateOfNode(const NodeID node_based_node_id) const = 0;

    virtual OSMNodeID GetOSMNodeIDOfNode(const NodeID node_based_node_id) const = 0;

    virtual GeometryID GetGeometryIndex(const NodeID edge_based_node_id) const = 0;

    virtual ComponentID GetComponentID(const NodeID edge_based_node_id) const = 0;

    virtual NodeForwardRange GetUncompressedForwardGeometry(const PackedGeometryID id) const = 0;
    virtual NodeReverseRange GetUncompressedReverseGeometry(const PackedGeometryID id) const = 0;

    virtual TurnPenalty GetWeightPenaltyForEdgeID(const EdgeID edge_based_edge_id) const = 0;

    virtual TurnPenalty GetDurationPenaltyForEdgeID(const EdgeID edge_based_edge_id) const = 0;

    // Gets the weight values for each segment in an uncompressed geometry.
    // Should always be 1 shorter than GetUncompressedGeometry
    virtual WeightForwardRange GetUncompressedForwardWeights(const PackedGeometryID id) const = 0;
    virtual WeightReverseRange GetUncompressedReverseWeights(const PackedGeometryID id) const = 0;

    // Gets the duration values for each segment in an uncompressed geometry.
    // Should always be 1 shorter than GetUncompressedGeometry
    virtual DurationForwardRange
    GetUncompressedForwardDurations(const PackedGeometryID id) const = 0;
    virtual DurationReverseRange
    GetUncompressedReverseDurations(const PackedGeometryID id) const = 0;

    // Returns the data source ids that were used to supply the edge
    // weights.  Will return an empty array when only the base profile is used.
    virtual DatasourceForwardRange
    GetUncompressedForwardDatasources(const PackedGeometryID id) const = 0;
    virtual DatasourceReverseRange
    GetUncompressedReverseDatasources(const PackedGeometryID id) const = 0;

    // Gets the name of a datasource
    virtual std::string_view GetDatasourceName(const DatasourceID id) const = 0;

    virtual osrm::guidance::TurnInstruction
    GetTurnInstructionForEdgeID(const EdgeID edge_based_edge_id) const = 0;

    virtual extractor::TravelMode GetTravelMode(const NodeID edge_based_node_id) const = 0;

    virtual extractor::ClassData GetClassData(const NodeID edge_based_node_id) const = 0;

    virtual bool ExcludeNode(const NodeID edge_based_node_id) const = 0;

    virtual std::vector<std::string> GetClasses(const extractor::ClassData class_data) const = 0;

    virtual std::vector<RTreeLeaf> GetEdgesInBox(const util::Coordinate south_west,
                                                 const util::Coordinate north_east) const = 0;

    virtual std::vector<PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate input_coordinate,
                               const double max_distance,
                               const std::optional<Bearing> bearing,
                               const Approach approach,
                               const bool use_all_edges) const = 0;

    virtual std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const size_t max_results,
                        const std::optional<double> max_distance,
                        const std::optional<Bearing> bearing,
                        const Approach approach) const = 0;

    virtual PhantomCandidateAlternatives
    NearestCandidatesWithAlternativeFromBigComponent(const util::Coordinate input_coordinate,
                                                     const std::optional<double> max_distance,
                                                     const std::optional<Bearing> bearing,
                                                     const Approach approach,
                                                     const bool use_all_edges) const = 0;

    virtual bool HasLaneData(const EdgeID edge_based_edge_id) const = 0;
    virtual util::guidance::LaneTupleIdPair GetLaneData(const EdgeID edge_based_edge_id) const = 0;
    virtual extractor::TurnLaneDescription
    GetTurnDescription(const LaneDescriptionID lane_description_id) const = 0;

    virtual NameID GetNameIndex(const NodeID edge_based_node_id) const = 0;

    virtual std::string_view GetNameForID(const NameID id) const = 0;

    virtual std::string_view GetRefForID(const NameID id) const = 0;

    virtual std::string_view GetPronunciationForID(const NameID id) const = 0;

    virtual std::string_view GetDestinationsForID(const NameID id) const = 0;

    virtual std::string_view GetExitsForID(const NameID id) const = 0;

    virtual bool GetContinueStraightDefault() const = 0;

    virtual double GetMapMatchingMaxSpeed() const = 0;

    virtual const char *GetWeightName() const = 0;

    virtual unsigned GetWeightPrecision() const = 0;

    virtual double GetWeightMultiplier() const = 0;

    virtual osrm::guidance::TurnBearing PreTurnBearing(const EdgeID edge_based_edge_id) const = 0;
    virtual osrm::guidance::TurnBearing PostTurnBearing(const EdgeID edge_based_edge_id) const = 0;

    virtual util::guidance::BearingClass GetBearingClass(const NodeID node_based_node_id) const = 0;

    virtual util::guidance::EntryClass GetEntryClass(const EdgeID edge_based_edge_id) const = 0;

    virtual bool IsLeftHandDriving(const NodeID edge_based_node_id) const = 0;

    virtual bool IsSegregated(const NodeID edge_based_node_id) const = 0;

    virtual std::vector<extractor::ManeuverOverride>
    GetOverridesThatStartAt(const NodeID edge_based_node_id) const = 0;
};
} // namespace osrm::engine::datafacade

#endif // DATAFACADE_BASE_HPP

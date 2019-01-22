#include "engine/routing_algorithms/routing_base_mld.hpp"
#include "engine/routing_algorithms/shortest_path_impl.hpp"
#include "engine/search_engine_data.hpp"
#include "util/integer_range.hpp"

#include <boost/test/unit_test.hpp>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

// Declare offline data facade algorithm
namespace offline
{
struct Algorithm final
{
};
}

} // routing_algorithms

// Define engine data for offline data facade
template <> struct SearchEngineData<routing_algorithms::offline::Algorithm>
{
    using QueryHeap = SearchEngineData<routing_algorithms::mld::Algorithm>::QueryHeap;

    using SearchEngineHeapPtr = std::unique_ptr<QueryHeap>;

    SearchEngineHeapPtr forward_heap_1;
    SearchEngineHeapPtr reverse_heap_1;

    void InitializeOrClearFirstThreadLocalStorage(unsigned number_of_nodes)
    {
        if (forward_heap_1.get())
        {
            forward_heap_1->Clear();
        }
        else
        {
            forward_heap_1.reset(new QueryHeap(number_of_nodes, 0));
        }

        if (reverse_heap_1.get())
        {
            reverse_heap_1->Clear();
        }
        else
        {
            reverse_heap_1.reset(new QueryHeap(number_of_nodes, 0));
        }
    }
};

// Define offline multilevel partition
namespace datafacade
{
struct ExternalMultiLevelPartition
{
    CellID GetCell(LevelID /*l*/, NodeID /*node*/) const { return 0; }
    LevelID GetQueryLevel(NodeID /*start*/, NodeID /*target*/, NodeID /*node*/) const { return 0; }
    LevelID GetHighestDifferentLevel(NodeID /*first*/, NodeID /*second*/) const { return 0; }
    std::uint8_t GetNumberOfLevels() const { return 0; }
    std::uint32_t GetNumberOfCells(LevelID /*level*/) const { return 0; }
    CellID BeginChildren(LevelID /*level*/, CellID /*cell*/) const { return 0; }
    CellID EndChildren(LevelID /*level*/, CellID /*cell*/) const { return 0; }
};

struct ExternalCellMetric
{
};

// Define external cell storage
struct ExternalCellStorage
{
    struct CellImpl
    {
        auto GetOutWeight(NodeID /*node*/) const
        {
            return boost::make_iterator_range((EdgeWeight *)0, (EdgeWeight *)0);
        }

        auto GetInWeight(NodeID /*node*/) const
        {
            return boost::make_iterator_range((EdgeWeight *)0, (EdgeWeight *)0);
        }

        auto GetSourceNodes() const
        {
            return boost::make_iterator_range((EdgeWeight *)0, (EdgeWeight *)0);
        }

        auto GetDestinationNodes() const
        {
            return boost::make_iterator_range((EdgeWeight *)0, (EdgeWeight *)0);
        }
    };

    using Cell = CellImpl;
    using ConstCell = const CellImpl;

    ConstCell GetCell(ExternalCellMetric, LevelID /*level*/, CellID /*id*/) const { return Cell{}; }
};

// Define external data facade
template <>
class ContiguousInternalMemoryDataFacade<routing_algorithms::offline::Algorithm> final
    : public BaseDataFacade
{
    ExternalMultiLevelPartition external_partition;
    ExternalCellStorage external_cell_storage;
    ExternalCellMetric external_cell_metric;

  public:
    using EdgeData = extractor::EdgeBasedEdge::EdgeData;
    // using RTreeLeaf = extractor::EdgeBasedNode;

    ContiguousInternalMemoryDataFacade() {}

    ~ContiguousInternalMemoryDataFacade() {}

    unsigned GetNumberOfNodes() const { return 0; }

    NodeID GetTarget(const EdgeID /*edgeID*/) const { return 0; }

    const EdgeData &GetEdgeData(const EdgeID /*edgeID*/) const
    {
        static EdgeData outData;
        return outData;
    }

    const auto &GetMultiLevelPartition() const { return external_partition; }

    const auto &GetCellStorage() const { return external_cell_storage; }

    const auto &GetCellMetric() const { return external_cell_metric; }

    auto GetBorderEdgeRange(const LevelID /*level*/, const NodeID /*node*/) const
    {
        return util::irange<EdgeID>(0, 0);
    }

    EdgeID FindEdge(const NodeID /*from*/, const NodeID /*to*/) const { return SPECIAL_EDGEID; }

    unsigned GetCheckSum() const override { return 0; }

    // node and edge information access
    util::Coordinate GetCoordinateOfNode(const NodeID /*id*/) const override
    {
        return {osrm::util::FloatLongitude{7.437069}, osrm::util::FloatLatitude{43.749249}};
    }

    OSMNodeID GetOSMNodeIDOfNode(const NodeID /*id*/) const override { return OSMNodeID(); }

    GeometryID GetGeometryIndex(const NodeID /*id*/) const override { return GeometryID{0, false}; }

    NodeForwardRange GetUncompressedForwardGeometry(const EdgeID /*id*/) const override
    {
        return {};
    }

    NodeReverseRange GetUncompressedReverseGeometry(const EdgeID /*id*/) const override
    {
        return NodeReverseRange(NodeForwardRange());
    }

    TurnPenalty GetWeightPenaltyForEdgeID(const unsigned /*id*/) const override
    {
        return INVALID_TURN_PENALTY;
    }

    TurnPenalty GetDurationPenaltyForEdgeID(const unsigned /*id*/) const override
    {
        return INVALID_TURN_PENALTY;
    }

    WeightForwardRange GetUncompressedForwardWeights(const EdgeID /*id*/) const override
    {
        return {};
    }

    WeightReverseRange GetUncompressedReverseWeights(const EdgeID /*id*/) const override
    {
        return WeightReverseRange(WeightForwardRange());
    }

    DurationForwardRange GetUncompressedForwardDurations(const EdgeID /*geomID*/) const override
    {
        return {};
    }

    DurationReverseRange GetUncompressedReverseDurations(const EdgeID /*geomID*/) const override
    {
        return DurationReverseRange(DurationForwardRange());
    }

    DatasourceForwardRange GetUncompressedForwardDatasources(const EdgeID /*id*/) const override
    {
        return {};
    }

    DatasourceReverseRange GetUncompressedReverseDatasources(const EdgeID /*id*/) const override
    {
        return DatasourceReverseRange(DatasourceForwardRange());
    }

    StringView GetDatasourceName(const DatasourceID /*id*/) const override { return StringView{}; }

    guidance::TurnInstruction GetTurnInstructionForEdgeID(const EdgeID /*id*/) const override
    {
        return guidance::TurnInstruction{};
    }

    extractor::TravelMode GetTravelMode(const NodeID /*id*/) const override
    {
        return extractor::TRAVEL_MODE_DRIVING;
    }

    std::vector<RTreeLeaf> GetEdgesInBox(const util::Coordinate /*south_west*/,
                                         const util::Coordinate /*north_east*/) const override
    {
        return {};
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate /*input_coordinate*/,
                               const float /*max_distance*/,
                               const int /*bearing*/,
                               const int /*bearing_range*/,
                               const Approach /*approach*/,
                               const bool /*use_all_edges*/) const override
    {
        return {};
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate /*input_coordinate*/,
                               const float /*max_distance*/,
                               const Approach /*approach*/,
                               const bool /*use_all_edges*/) const override
    {
        return {};
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate /*input_coordinate*/,
                        const unsigned /*max_results*/,
                        const double /*max_distance*/,
                        const int /*bearing*/,
                        const int /*bearing_range*/,
                        const Approach /*approach*/) const override
    {
        return {};
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate /*input_coordinate*/,
                        const unsigned /*max_results*/,
                        const int /*bearing*/,
                        const int /*bearing_range*/,
                        const Approach /*approach*/) const override
    {
        return {};
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate /*input_coordinate*/,
                        const unsigned /*max_results*/,
                        const Approach /*approach*/) const override
    {
        return {};
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate /*input_coordinate*/,
                        const unsigned /*max_results*/,
                        const double /*max_distance*/,
                        const Approach /*approach*/) const override
    {
        return {};
    }

    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate /*input_coordinate*/,
                                                      const Approach /*approach*/) const override
    {
        return {};
    }

    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate /*input_coordinate*/,
                                                      const double /*max_distance*/,
                                                      const Approach /*approach*/) const override
    {
        return {};
    }

    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate /*input_coordinate*/,
                                                      const double /*max_distance*/,
                                                      const int /*bearing*/,
                                                      const int /*bearing_range*/,
                                                      const Approach /*approach*/) const override
    {
        return {};
    }

    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate /*input_coordinate*/,
                                                      const int /*bearing*/,
                                                      const int /*bearing_range*/,
                                                      const Approach /*approach*/) const override
    {
        return {};
    }

    util::guidance::LaneTupleIdPair GetLaneData(const EdgeID /*id*/) const override
    {
        return util::guidance::LaneTupleIdPair{};
    }

    extractor::TurnLaneDescription
    GetTurnDescription(const LaneDescriptionID /*laneDescriptionID*/) const override
    {
        return {};
    }

    EdgeWeight GetNodeWeight(const NodeID /*node*/) const { return 0; }

    bool IsForwardEdge(const NodeID /*edge*/) const { return true; }

    bool IsBackwardEdge(const NodeID /*edge*/) const { return true; }

    bool HasLaneData(const EdgeID /*id*/) const override { return false; }
    NameID GetNameIndex(const NodeID /*nodeID*/) const override { return EMPTY_NAMEID; }
    StringView GetNameForID(const NameID /*id*/) const override { return StringView{}; }
    StringView GetRefForID(const NameID /*id*/) const override { return StringView{}; }
    StringView GetPronunciationForID(const NameID /*id*/) const override { return StringView{}; }
    StringView GetDestinationsForID(const NameID /*id*/) const override { return StringView{}; }
    StringView GetExitsForID(const NameID /*id*/) const override { return StringView{}; }
    bool GetContinueStraightDefault() const override { return false; }
    std::string GetTimestamp() const override { return ""; }
    double GetMapMatchingMaxSpeed() const override { return 0; }
    const char *GetWeightName() const override { return ""; }
    unsigned GetWeightPrecision() const override { return 0; }
    double GetWeightMultiplier() const override { return 1; }
    ComponentID GetComponentID(NodeID) const override { return ComponentID{}; }
    bool ExcludeNode(const NodeID) const override { return false; }

    guidance::TurnBearing PreTurnBearing(const EdgeID /*eid*/) const override
    {
        return guidance::TurnBearing(0);
    }

    guidance::TurnBearing PostTurnBearing(const EdgeID /*eid*/) const override
    {
        return guidance::TurnBearing(0);
    }

    util::guidance::BearingClass
    GetBearingClass(const BearingClassID /*bearing_class_id*/) const override
    {
        return util::guidance::BearingClass{};
    }

    osrm::extractor::ClassData GetClassData(const NodeID /*id*/) const override { return 0; }
    std::vector<std::string> GetClasses(const extractor::ClassData /*class_data*/) const override
    {
        return {};
    }

    util::guidance::EntryClass GetEntryClass(const EdgeID /*turn_id*/) const override { return {}; }
    bool IsLeftHandDriving(const NodeID /*id*/) const override { return false; }
    bool IsSegregated(const NodeID /*id*/) const override { return false; }

    std::vector<extractor::ManeuverOverride>
    GetOverridesThatStartAt(const NodeID /* edge_based_node_id */) const override
    {
        return {};
    }
};

} // datafacade

// Fallback to MLD algorithm: requires from data facade MLD specific members
namespace routing_algorithms
{
namespace offline
{

inline void search(SearchEngineData<Algorithm> &engine_working_data,
                   const datafacade::ContiguousInternalMemoryDataFacade<Algorithm> &facade,
                   typename SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                   typename SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
                   EdgeWeight &weight,
                   std::vector<NodeID> &packed_leg,
                   const bool force_loop_forward,
                   const bool force_loop_reverse,
                   const PhantomNodes &phantom_nodes,
                   const EdgeWeight weight_upper_bound = INVALID_EDGE_WEIGHT)
{
    mld::search(engine_working_data,
                facade,
                forward_heap,
                reverse_heap,
                weight,
                packed_leg,
                force_loop_forward,
                force_loop_reverse,
                phantom_nodes,
                weight_upper_bound);
}

template <typename RandomIter, typename FacadeT>
void unpackPath(const FacadeT &facade,
                RandomIter packed_path_begin,
                RandomIter packed_path_end,
                const PhantomNodes &phantom_nodes,
                std::vector<PathData> &unpacked_path)
{
    mld::unpackPath(facade, packed_path_begin, packed_path_end, phantom_nodes, unpacked_path);
}

} // offline
} // routing_algorithms

} // engine
} // osrm

BOOST_AUTO_TEST_SUITE(offline_facade)

BOOST_AUTO_TEST_CASE(shortest_path)
{
    using Algorithm = osrm::engine::routing_algorithms::offline::Algorithm;

    osrm::engine::SearchEngineData<Algorithm> heaps;
    osrm::engine::datafacade::ContiguousInternalMemoryDataFacade<Algorithm> facade;

    std::vector<osrm::engine::PhantomNodes> phantom_nodes;
    phantom_nodes.push_back({osrm::engine::PhantomNode{}, osrm::engine::PhantomNode{}});

    auto route =
        osrm::engine::routing_algorithms::shortestPathSearch(heaps, facade, phantom_nodes, false);

    BOOST_CHECK_EQUAL(route.shortest_path_weight, INVALID_EDGE_WEIGHT);
}

BOOST_AUTO_TEST_CASE(facade_uncompressed_methods)
{
    using Algorithm = osrm::engine::routing_algorithms::offline::Algorithm;

    osrm::engine::SearchEngineData<Algorithm> heaps;
    osrm::engine::datafacade::ContiguousInternalMemoryDataFacade<Algorithm> facade;

    BOOST_CHECK_EQUAL(facade.GetUncompressedForwardGeometry(0).size(), 0);
    BOOST_CHECK_EQUAL(facade.GetUncompressedReverseGeometry(0).size(), 0);
    BOOST_CHECK_EQUAL(facade.GetUncompressedForwardWeights(0).size(), 0);
    BOOST_CHECK_EQUAL(facade.GetUncompressedReverseWeights(0).size(), 0);
    BOOST_CHECK_EQUAL(facade.GetUncompressedForwardDurations(0).size(), 0);
    BOOST_CHECK_EQUAL(facade.GetUncompressedReverseDurations(0).size(), 0);
    BOOST_CHECK_EQUAL(facade.GetUncompressedForwardDatasources(0).size(), 0);
    BOOST_CHECK_EQUAL(facade.GetUncompressedReverseDatasources(0).size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

#ifndef CONTIGUOUS_INTERNALMEM_DATAFACADE_HPP
#define CONTIGUOUS_INTERNALMEM_DATAFACADE_HPP

#include "engine/datafacade/algorithm_datafacade.hpp"
#include "engine/datafacade/contiguous_block_allocator.hpp"
#include "engine/datafacade/datafacade_base.hpp"

#include "engine/algorithm.hpp"
#include "engine/geospatial_query.hpp"

#include "customizer/edge_based_graph.hpp"

#include "extractor/datasources.hpp"
#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/guidance/turn_lane_types.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/segment_data_container.hpp"
#include "extractor/turn_data_container.hpp"

#include "contractor/query_graph.hpp"

#include "partition/cell_storage.hpp"
#include "partition/multi_level_partition.hpp"

#include "storage/shared_datatype.hpp"
#include "storage/shared_memory_ownership.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/guidance/turn_bearing.hpp"
#include "util/guidance/turn_lanes.hpp"
#include "util/log.hpp"
#include "util/name_table.hpp"
#include "util/packed_vector.hpp"
#include "util/range_table.hpp"
#include "util/rectangle.hpp"
#include "util/static_graph.hpp"
#include "util/static_rtree.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace osrm
{
namespace engine
{
namespace datafacade
{

template <typename AlgorithmT> class ContiguousInternalMemoryAlgorithmDataFacade;

template <>
class ContiguousInternalMemoryAlgorithmDataFacade<CH> : public datafacade::AlgorithmDataFacade<CH>
{
  private:
    using QueryGraph = contractor::QueryGraphView;
    using GraphNode = QueryGraph::NodeArrayEntry;
    using GraphEdge = QueryGraph::EdgeArrayEntry;

    QueryGraph m_query_graph;

    // allocator that keeps the allocation data
    std::shared_ptr<ContiguousBlockAllocator> allocator;

    void InitializeGraphPointer(storage::DataLayout &data_layout, char *memory_block)
    {
        auto graph_nodes_ptr = data_layout.GetBlockPtr<GraphNode>(
            memory_block, storage::DataLayout::CH_GRAPH_NODE_LIST);

        auto graph_edges_ptr = data_layout.GetBlockPtr<GraphEdge>(
            memory_block, storage::DataLayout::CH_GRAPH_EDGE_LIST);

        util::vector_view<GraphNode> node_list(
            graph_nodes_ptr, data_layout.num_entries[storage::DataLayout::CH_GRAPH_NODE_LIST]);
        util::vector_view<GraphEdge> edge_list(
            graph_edges_ptr, data_layout.num_entries[storage::DataLayout::CH_GRAPH_EDGE_LIST]);
        m_query_graph = QueryGraph(node_list, edge_list);
    }

  public:
    ContiguousInternalMemoryAlgorithmDataFacade(
        std::shared_ptr<ContiguousBlockAllocator> allocator_)
        : allocator(std::move(allocator_))
    {
        InitializeInternalPointers(allocator->GetLayout(), allocator->GetMemory());
    }

    void InitializeInternalPointers(storage::DataLayout &data_layout, char *memory_block)
    {
        InitializeGraphPointer(data_layout, memory_block);
    }

    // search graph access
    unsigned GetNumberOfNodes() const override final { return m_query_graph.GetNumberOfNodes(); }

    unsigned GetNumberOfEdges() const override final { return m_query_graph.GetNumberOfEdges(); }

    unsigned GetOutDegree(const NodeID n) const override final
    {
        return m_query_graph.GetOutDegree(n);
    }

    NodeID GetTarget(const EdgeID e) const override final { return m_query_graph.GetTarget(e); }

    const EdgeData &GetEdgeData(const EdgeID e) const override final
    {
        return m_query_graph.GetEdgeData(e);
    }

    EdgeID BeginEdges(const NodeID n) const override final { return m_query_graph.BeginEdges(n); }

    EdgeID EndEdges(const NodeID n) const override final { return m_query_graph.EndEdges(n); }

    EdgeRange GetAdjacentEdgeRange(const NodeID node) const override final
    {
        return m_query_graph.GetAdjacentEdgeRange(node);
    }

    // searches for a specific edge
    EdgeID FindEdge(const NodeID from, const NodeID to) const override final
    {
        return m_query_graph.FindEdge(from, to);
    }

    EdgeID FindEdgeInEitherDirection(const NodeID from, const NodeID to) const override final
    {
        return m_query_graph.FindEdgeInEitherDirection(from, to);
    }

    EdgeID
    FindEdgeIndicateIfReverse(const NodeID from, const NodeID to, bool &result) const override final
    {
        return m_query_graph.FindEdgeIndicateIfReverse(from, to, result);
    }

    EdgeID FindSmallestEdge(const NodeID from,
                            const NodeID to,
                            std::function<bool(EdgeData)> filter) const override final
    {
        return m_query_graph.FindSmallestEdge(from, to, filter);
    }
};

template <>
class ContiguousInternalMemoryAlgorithmDataFacade<CoreCH>
    : public datafacade::AlgorithmDataFacade<CoreCH>
{
  private:
    util::vector_view<bool> m_is_core_node;

    // allocator that keeps the allocation data
    std::shared_ptr<ContiguousBlockAllocator> allocator;

    void InitializeCoreInformationPointer(storage::DataLayout &data_layout, char *memory_block)
    {
        auto core_marker_ptr =
            data_layout.GetBlockPtr<unsigned>(memory_block, storage::DataLayout::CH_CORE_MARKER);
        util::vector_view<bool> is_core_node(
            core_marker_ptr, data_layout.num_entries[storage::DataLayout::CH_CORE_MARKER]);
        m_is_core_node = std::move(is_core_node);
    }

  public:
    ContiguousInternalMemoryAlgorithmDataFacade(
        std::shared_ptr<ContiguousBlockAllocator> allocator_)
        : allocator(std::move(allocator_))
    {
        InitializeInternalPointers(allocator->GetLayout(), allocator->GetMemory());
    }

    void InitializeInternalPointers(storage::DataLayout &data_layout, char *memory_block)
    {
        InitializeCoreInformationPointer(data_layout, memory_block);
    }

    bool IsCoreNode(const NodeID id) const override final
    {
        BOOST_ASSERT(id < m_is_core_node.size());
        return m_is_core_node[id];
    }
};

/**
 * This base class implements the Datafacade interface for accessing
 * data that's stored in a single large block of memory (RAM).
 *
 * In this case "internal memory" refers to RAM - as opposed to "external memory",
 * which usually refers to disk.
 */
class ContiguousInternalMemoryDataFacadeBase : public BaseDataFacade
{
  private:
    using super = BaseDataFacade;
    using IndexBlock = util::RangeTable<16, storage::Ownership::View>::BlockT;
    using RTreeLeaf = super::RTreeLeaf;
    using SharedRTree = util::StaticRTree<RTreeLeaf, storage::Ownership::View>;
    using SharedGeospatialQuery = GeospatialQuery<SharedRTree, BaseDataFacade>;
    using RTreeNode = SharedRTree::TreeNode;

    std::string m_timestamp;
    extractor::ProfileProperties *m_profile_properties;
    extractor::Datasources *m_datasources;

    unsigned m_check_sum;
    util::vector_view<util::Coordinate> m_coordinate_list;
    util::PackedVectorView<OSMNodeID> m_osmnodeid_list;
    util::NameTable m_names_table;
    util::vector_view<std::uint32_t> m_lane_description_offsets;
    util::vector_view<extractor::guidance::TurnLaneType::Mask> m_lane_description_masks;
    util::vector_view<TurnPenalty> m_turn_weight_penalties;
    util::vector_view<TurnPenalty> m_turn_duration_penalties;
    extractor::SegmentDataView segment_data;
    extractor::TurnDataView turn_data;

    util::vector_view<char> m_datasource_name_data;
    util::vector_view<std::size_t> m_datasource_name_offsets;
    util::vector_view<std::size_t> m_datasource_name_lengths;
    util::vector_view<util::guidance::LaneTupleIdPair> m_lane_tupel_id_pairs;

    std::unique_ptr<SharedRTree> m_static_rtree;
    std::unique_ptr<SharedGeospatialQuery> m_geospatial_query;
    boost::filesystem::path file_index_path;

    util::NameTable m_name_table;
    // bearing classes by node based node
    util::vector_view<BearingClassID> m_bearing_class_id_table;
    // entry class IDs
    util::vector_view<EntryClassID> m_entry_class_id_list;

    // the look-up table for entry classes. An entry class lists the possibility of entry for all
    // available turns. Such a class id is stored with every edge.
    util::vector_view<util::guidance::EntryClass> m_entry_class_table;
    // the look-up table for distinct bearing classes. A bearing class lists the available bearings
    // at an intersection
    std::shared_ptr<util::RangeTable<16, storage::Ownership::View>> m_bearing_ranges_table;
    util::vector_view<DiscreteBearing> m_bearing_values_table;

    // allocator that keeps the allocation data
    std::shared_ptr<ContiguousBlockAllocator> allocator;

    void InitializeProfilePropertiesPointer(storage::DataLayout &data_layout, char *memory_block)
    {
        m_profile_properties = data_layout.GetBlockPtr<extractor::ProfileProperties>(
            memory_block, storage::DataLayout::PROPERTIES);
    }

    void InitializeTimestampPointer(storage::DataLayout &data_layout, char *memory_block)
    {
        auto timestamp_ptr =
            data_layout.GetBlockPtr<char>(memory_block, storage::DataLayout::TIMESTAMP);
        m_timestamp.resize(data_layout.GetBlockSize(storage::DataLayout::TIMESTAMP));
        std::copy(timestamp_ptr,
                  timestamp_ptr + data_layout.GetBlockSize(storage::DataLayout::TIMESTAMP),
                  m_timestamp.begin());
    }

    void InitializeChecksumPointer(storage::DataLayout &data_layout, char *memory_block)
    {
        m_check_sum =
            *data_layout.GetBlockPtr<unsigned>(memory_block, storage::DataLayout::HSGR_CHECKSUM);
        util::Log() << "set checksum: " << m_check_sum;
    }

    void InitializeRTreePointers(storage::DataLayout &data_layout, char *memory_block)
    {
        BOOST_ASSERT_MSG(!m_coordinate_list.empty(), "coordinates must be loaded before r-tree");

        const auto file_index_ptr =
            data_layout.GetBlockPtr<char>(memory_block, storage::DataLayout::FILE_INDEX_PATH);
        file_index_path = boost::filesystem::path(file_index_ptr);
        if (!boost::filesystem::exists(file_index_path))
        {
            util::Log(logDEBUG) << "Leaf file name " << file_index_path.string();
            throw util::exception("Could not load " + file_index_path.string() +
                                  "Is any data loaded into shared memory?" + SOURCE_REF);
        }

        auto tree_ptr =
            data_layout.GetBlockPtr<RTreeNode>(memory_block, storage::DataLayout::R_SEARCH_TREE);
        m_static_rtree.reset(
            new SharedRTree(tree_ptr,
                            data_layout.num_entries[storage::DataLayout::R_SEARCH_TREE],
                            file_index_path,
                            m_coordinate_list));
        m_geospatial_query.reset(
            new SharedGeospatialQuery(*m_static_rtree, m_coordinate_list, *this));
    }

    void InitializeNodeInformationPointers(storage::DataLayout &data_layout, char *memory_block)
    {
        const auto coordinate_list_ptr = data_layout.GetBlockPtr<util::Coordinate>(
            memory_block, storage::DataLayout::COORDINATE_LIST);
        m_coordinate_list.reset(coordinate_list_ptr,
                                data_layout.num_entries[storage::DataLayout::COORDINATE_LIST]);

        for (unsigned i = 0; i < m_coordinate_list.size(); ++i)
        {
            BOOST_ASSERT(GetCoordinateOfNode(i).IsValid());
        }

        const auto osmnodeid_list_ptr = data_layout.GetBlockPtr<std::uint64_t>(
            memory_block, storage::DataLayout::OSM_NODE_ID_LIST);
        m_osmnodeid_list.reset(osmnodeid_list_ptr,
                               data_layout.num_entries[storage::DataLayout::OSM_NODE_ID_LIST]);
        // We (ab)use the number of coordinates here because we know we have the same amount of ids
        m_osmnodeid_list.set_number_of_entries(
            data_layout.num_entries[storage::DataLayout::COORDINATE_LIST]);
    }

    void InitializeEdgeInformationPointers(storage::DataLayout &layout, char *memory_ptr)
    {
        auto via_geometry_list_ptr =
            layout.GetBlockPtr<GeometryID>(memory_ptr, storage::DataLayout::VIA_NODE_LIST);
        util::vector_view<GeometryID> geometry_ids(
            via_geometry_list_ptr, layout.num_entries[storage::DataLayout::VIA_NODE_LIST]);

        const auto travel_mode_list_ptr =
            layout.GetBlockPtr<extractor::TravelMode>(memory_ptr, storage::DataLayout::TRAVEL_MODE);
        util::vector_view<extractor::TravelMode> travel_modes(
            travel_mode_list_ptr, layout.num_entries[storage::DataLayout::TRAVEL_MODE]);

        const auto lane_data_id_ptr =
            layout.GetBlockPtr<LaneDataID>(memory_ptr, storage::DataLayout::LANE_DATA_ID);
        util::vector_view<LaneDataID> lane_data_ids(
            lane_data_id_ptr, layout.num_entries[storage::DataLayout::LANE_DATA_ID]);

        const auto turn_instruction_list_ptr =
            layout.GetBlockPtr<extractor::guidance::TurnInstruction>(
                memory_ptr, storage::DataLayout::TURN_INSTRUCTION);
        util::vector_view<extractor::guidance::TurnInstruction> turn_instructions(
            turn_instruction_list_ptr, layout.num_entries[storage::DataLayout::TURN_INSTRUCTION]);

        const auto name_id_list_ptr =
            layout.GetBlockPtr<NameID>(memory_ptr, storage::DataLayout::NAME_ID_LIST);
        util::vector_view<NameID> name_ids(name_id_list_ptr,
                                           layout.num_entries[storage::DataLayout::NAME_ID_LIST]);

        const auto entry_class_id_list_ptr =
            layout.GetBlockPtr<EntryClassID>(memory_ptr, storage::DataLayout::ENTRY_CLASSID);
        util::vector_view<EntryClassID> entry_class_ids(
            entry_class_id_list_ptr, layout.num_entries[storage::DataLayout::ENTRY_CLASSID]);

        const auto pre_turn_bearing_ptr = layout.GetBlockPtr<util::guidance::TurnBearing>(
            memory_ptr, storage::DataLayout::PRE_TURN_BEARING);
        util::vector_view<util::guidance::TurnBearing> pre_turn_bearings(
            pre_turn_bearing_ptr, layout.num_entries[storage::DataLayout::PRE_TURN_BEARING]);

        const auto post_turn_bearing_ptr = layout.GetBlockPtr<util::guidance::TurnBearing>(
            memory_ptr, storage::DataLayout::POST_TURN_BEARING);
        util::vector_view<util::guidance::TurnBearing> post_turn_bearings(
            post_turn_bearing_ptr, layout.num_entries[storage::DataLayout::POST_TURN_BEARING]);

        turn_data = extractor::TurnDataView(std::move(geometry_ids),
                                            std::move(name_ids),
                                            std::move(turn_instructions),
                                            std::move(lane_data_ids),
                                            std::move(travel_modes),
                                            std::move(entry_class_ids),
                                            std::move(pre_turn_bearings),
                                            std::move(post_turn_bearings));
    }

    void InitializeNamePointers(storage::DataLayout &data_layout, char *memory_block)
    {
        auto name_data_ptr =
            data_layout.GetBlockPtr<char>(memory_block, storage::DataLayout::NAME_CHAR_DATA);
        const auto name_data_size = data_layout.num_entries[storage::DataLayout::NAME_CHAR_DATA];
        m_name_table.reset(name_data_ptr, name_data_ptr + name_data_size);
    }

    void InitializeTurnLaneDescriptionsPointers(storage::DataLayout &data_layout,
                                                char *memory_block)
    {
        auto offsets_ptr = data_layout.GetBlockPtr<std::uint32_t>(
            memory_block, storage::DataLayout::LANE_DESCRIPTION_OFFSETS);
        util::vector_view<std::uint32_t> offsets(
            offsets_ptr, data_layout.num_entries[storage::DataLayout::LANE_DESCRIPTION_OFFSETS]);
        m_lane_description_offsets = std::move(offsets);

        auto masks_ptr = data_layout.GetBlockPtr<extractor::guidance::TurnLaneType::Mask>(
            memory_block, storage::DataLayout::LANE_DESCRIPTION_MASKS);

        util::vector_view<extractor::guidance::TurnLaneType::Mask> masks(
            masks_ptr, data_layout.num_entries[storage::DataLayout::LANE_DESCRIPTION_MASKS]);
        m_lane_description_masks = std::move(masks);

        const auto lane_tupel_id_pair_ptr =
            data_layout.GetBlockPtr<util::guidance::LaneTupleIdPair>(
                memory_block, storage::DataLayout::TURN_LANE_DATA);
        util::vector_view<util::guidance::LaneTupleIdPair> lane_tupel_id_pair(
            lane_tupel_id_pair_ptr, data_layout.num_entries[storage::DataLayout::TURN_LANE_DATA]);
        m_lane_tupel_id_pairs = std::move(lane_tupel_id_pair);
    }

    void InitializeTurnPenalties(storage::DataLayout &data_layout, char *memory_block)
    {
        auto turn_weight_penalties_ptr = data_layout.GetBlockPtr<TurnPenalty>(
            memory_block, storage::DataLayout::TURN_WEIGHT_PENALTIES);
        m_turn_weight_penalties = util::vector_view<TurnPenalty>(
            turn_weight_penalties_ptr,
            data_layout.num_entries[storage::DataLayout::TURN_WEIGHT_PENALTIES]);
        auto turn_duration_penalties_ptr = data_layout.GetBlockPtr<TurnPenalty>(
            memory_block, storage::DataLayout::TURN_DURATION_PENALTIES);
        m_turn_duration_penalties = util::vector_view<TurnPenalty>(
            turn_duration_penalties_ptr,
            data_layout.num_entries[storage::DataLayout::TURN_DURATION_PENALTIES]);
    }

    void InitializeGeometryPointers(storage::DataLayout &data_layout, char *memory_block)
    {
        auto geometries_index_ptr =
            data_layout.GetBlockPtr<unsigned>(memory_block, storage::DataLayout::GEOMETRIES_INDEX);
        util::vector_view<unsigned> geometry_begin_indices(
            geometries_index_ptr, data_layout.num_entries[storage::DataLayout::GEOMETRIES_INDEX]);

        auto geometries_node_list_ptr = data_layout.GetBlockPtr<NodeID>(
            memory_block, storage::DataLayout::GEOMETRIES_NODE_LIST);
        util::vector_view<NodeID> geometry_node_list(
            geometries_node_list_ptr,
            data_layout.num_entries[storage::DataLayout::GEOMETRIES_NODE_LIST]);

        auto geometries_fwd_weight_list_ptr = data_layout.GetBlockPtr<EdgeWeight>(
            memory_block, storage::DataLayout::GEOMETRIES_FWD_WEIGHT_LIST);
        util::vector_view<EdgeWeight> geometry_fwd_weight_list(
            geometries_fwd_weight_list_ptr,
            data_layout.num_entries[storage::DataLayout::GEOMETRIES_FWD_WEIGHT_LIST]);

        auto geometries_rev_weight_list_ptr = data_layout.GetBlockPtr<EdgeWeight>(
            memory_block, storage::DataLayout::GEOMETRIES_REV_WEIGHT_LIST);
        util::vector_view<EdgeWeight> geometry_rev_weight_list(
            geometries_rev_weight_list_ptr,
            data_layout.num_entries[storage::DataLayout::GEOMETRIES_REV_WEIGHT_LIST]);

        auto geometries_fwd_duration_list_ptr = data_layout.GetBlockPtr<EdgeWeight>(
            memory_block, storage::DataLayout::GEOMETRIES_FWD_DURATION_LIST);
        util::vector_view<EdgeWeight> geometry_fwd_duration_list(
            geometries_fwd_duration_list_ptr,
            data_layout.num_entries[storage::DataLayout::GEOMETRIES_FWD_DURATION_LIST]);

        auto geometries_rev_duration_list_ptr = data_layout.GetBlockPtr<EdgeWeight>(
            memory_block, storage::DataLayout::GEOMETRIES_REV_DURATION_LIST);
        util::vector_view<EdgeWeight> geometry_rev_duration_list(
            geometries_rev_duration_list_ptr,
            data_layout.num_entries[storage::DataLayout::GEOMETRIES_REV_DURATION_LIST]);

        auto datasources_list_ptr = data_layout.GetBlockPtr<DatasourceID>(
            memory_block, storage::DataLayout::DATASOURCES_LIST);
        util::vector_view<DatasourceID> datasources_list(
            datasources_list_ptr, data_layout.num_entries[storage::DataLayout::DATASOURCES_LIST]);

        segment_data = extractor::SegmentDataView{std::move(geometry_begin_indices),
                                                  std::move(geometry_node_list),
                                                  std::move(geometry_fwd_weight_list),
                                                  std::move(geometry_rev_weight_list),
                                                  std::move(geometry_fwd_duration_list),
                                                  std::move(geometry_rev_duration_list),
                                                  std::move(datasources_list)};

        m_datasources = data_layout.GetBlockPtr<extractor::Datasources>(
            memory_block, storage::DataLayout::DATASOURCES_NAMES);
    }

    void InitializeIntersectionClassPointers(storage::DataLayout &data_layout, char *memory_block)
    {
        auto bearing_class_id_ptr = data_layout.GetBlockPtr<BearingClassID>(
            memory_block, storage::DataLayout::BEARING_CLASSID);
        typename util::vector_view<BearingClassID> bearing_class_id_table(
            bearing_class_id_ptr, data_layout.num_entries[storage::DataLayout::BEARING_CLASSID]);
        m_bearing_class_id_table = std::move(bearing_class_id_table);

        auto bearing_class_ptr = data_layout.GetBlockPtr<DiscreteBearing>(
            memory_block, storage::DataLayout::BEARING_VALUES);
        typename util::vector_view<DiscreteBearing> bearing_class_table(
            bearing_class_ptr, data_layout.num_entries[storage::DataLayout::BEARING_VALUES]);
        m_bearing_values_table = std::move(bearing_class_table);

        auto offsets_ptr =
            data_layout.GetBlockPtr<unsigned>(memory_block, storage::DataLayout::BEARING_OFFSETS);
        auto blocks_ptr =
            data_layout.GetBlockPtr<IndexBlock>(memory_block, storage::DataLayout::BEARING_BLOCKS);
        util::vector_view<unsigned> bearing_offsets(
            offsets_ptr, data_layout.num_entries[storage::DataLayout::BEARING_OFFSETS]);
        util::vector_view<IndexBlock> bearing_blocks(
            blocks_ptr, data_layout.num_entries[storage::DataLayout::BEARING_BLOCKS]);

        m_bearing_ranges_table = std::make_unique<util::RangeTable<16, storage::Ownership::View>>(
            bearing_offsets, bearing_blocks, static_cast<unsigned>(m_bearing_values_table.size()));

        auto entry_class_ptr = data_layout.GetBlockPtr<util::guidance::EntryClass>(
            memory_block, storage::DataLayout::ENTRY_CLASS);
        typename util::vector_view<util::guidance::EntryClass> entry_class_table(
            entry_class_ptr, data_layout.num_entries[storage::DataLayout::ENTRY_CLASS]);
        m_entry_class_table = std::move(entry_class_table);
    }

    void InitializeInternalPointers(storage::DataLayout &data_layout, char *memory_block)
    {
        InitializeChecksumPointer(data_layout, memory_block);
        InitializeNodeInformationPointers(data_layout, memory_block);
        InitializeEdgeInformationPointers(data_layout, memory_block);
        InitializeTurnPenalties(data_layout, memory_block);
        InitializeGeometryPointers(data_layout, memory_block);
        InitializeTimestampPointer(data_layout, memory_block);
        InitializeNamePointers(data_layout, memory_block);
        InitializeTurnLaneDescriptionsPointers(data_layout, memory_block);
        InitializeProfilePropertiesPointer(data_layout, memory_block);
        InitializeRTreePointers(data_layout, memory_block);
        InitializeIntersectionClassPointers(data_layout, memory_block);
    }

  public:
    // allows switching between process_memory/shared_memory datafacade, based on the type of
    // allocator
    ContiguousInternalMemoryDataFacadeBase(std::shared_ptr<ContiguousBlockAllocator> allocator_)
        : allocator(std::move(allocator_))
    {
        InitializeInternalPointers(allocator->GetLayout(), allocator->GetMemory());
    }

    // node and edge information access
    util::Coordinate GetCoordinateOfNode(const NodeID id) const override final
    {
        return m_coordinate_list[id];
    }

    OSMNodeID GetOSMNodeIDOfNode(const NodeID id) const override final
    {
        return m_osmnodeid_list.at(id);
    }

    virtual std::vector<NodeID> GetUncompressedForwardGeometry(const EdgeID id) const override final
    {

        auto range = segment_data.GetForwardGeometry(id);
        return std::vector<NodeID>{range.begin(), range.end()};
    }

    virtual std::vector<NodeID> GetUncompressedReverseGeometry(const EdgeID id) const override final
    {
        auto range = segment_data.GetReverseGeometry(id);
        return std::vector<NodeID>{range.begin(), range.end()};
    }

    virtual std::vector<EdgeWeight>
    GetUncompressedForwardDurations(const EdgeID id) const override final
    {
        auto range = segment_data.GetForwardDurations(id);
        return std::vector<EdgeWeight>{range.begin(), range.end()};
    }

    virtual std::vector<EdgeWeight>
    GetUncompressedReverseDurations(const EdgeID id) const override final
    {
        auto range = segment_data.GetReverseDurations(id);
        return std::vector<EdgeWeight>{range.begin(), range.end()};
    }

    virtual std::vector<EdgeWeight>
    GetUncompressedForwardWeights(const EdgeID id) const override final
    {
        auto range = segment_data.GetForwardWeights(id);
        return std::vector<EdgeWeight>{range.begin(), range.end()};
    }

    virtual std::vector<EdgeWeight>
    GetUncompressedReverseWeights(const EdgeID id) const override final
    {
        auto range = segment_data.GetReverseWeights(id);
        return std::vector<EdgeWeight>{range.begin(), range.end()};
    }

    // Returns the data source ids that were used to supply the edge
    // weights.
    virtual std::vector<DatasourceID>
    GetUncompressedForwardDatasources(const EdgeID id) const override final
    {
        auto range = segment_data.GetForwardDatasources(id);
        return std::vector<DatasourceID>{range.begin(), range.end()};
    }

    // Returns the data source ids that were used to supply the edge
    // weights.
    virtual std::vector<DatasourceID>
    GetUncompressedReverseDatasources(const EdgeID id) const override final
    {
        auto range = segment_data.GetReverseDatasources(id);
        return std::vector<DatasourceID>{range.begin(), range.end()};
    }

    virtual GeometryID GetGeometryIndexForEdgeID(const EdgeID id) const override final
    {
        return turn_data.GetGeometryID(id);
    }

    virtual TurnPenalty GetWeightPenaltyForEdgeID(const unsigned id) const override final
    {
        BOOST_ASSERT(m_turn_weight_penalties.size() > id);
        return m_turn_weight_penalties[id];
    }

    virtual TurnPenalty GetDurationPenaltyForEdgeID(const unsigned id) const override final
    {
        BOOST_ASSERT(m_turn_duration_penalties.size() > id);
        return m_turn_duration_penalties[id];
    }

    extractor::guidance::TurnInstruction
    GetTurnInstructionForEdgeID(const EdgeID id) const override final
    {
        return turn_data.GetTurnInstruction(id);
    }

    extractor::TravelMode GetTravelModeForEdgeID(const EdgeID id) const override final
    {
        return turn_data.GetTravelMode(id);
    }

    std::vector<RTreeLeaf> GetEdgesInBox(const util::Coordinate south_west,
                                         const util::Coordinate north_east) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());
        const util::RectangleInt2D bbox{
            south_west.lon, north_east.lon, south_west.lat, north_east.lat};
        return m_geospatial_query->Search(bbox);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate input_coordinate,
                               const float max_distance) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodesInRange(input_coordinate, max_distance);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate input_coordinate,
                               const float max_distance,
                               const int bearing,
                               const int bearing_range) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodesInRange(
            input_coordinate, max_distance, bearing, bearing_range);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodes(input_coordinate, max_results);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const double max_distance) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodes(input_coordinate, max_results, max_distance);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const int bearing,
                        const int bearing_range) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodes(
            input_coordinate, max_results, bearing, bearing_range);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const double max_distance,
                        const int bearing,
                        const int bearing_range) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodes(
            input_coordinate, max_results, max_distance, bearing, bearing_range);
    }

    std::pair<PhantomNode, PhantomNode> NearestPhantomNodeWithAlternativeFromBigComponent(
        const util::Coordinate input_coordinate) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodeWithAlternativeFromBigComponent(
            input_coordinate);
    }

    std::pair<PhantomNode, PhantomNode> NearestPhantomNodeWithAlternativeFromBigComponent(
        const util::Coordinate input_coordinate, const double max_distance) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodeWithAlternativeFromBigComponent(
            input_coordinate, max_distance);
    }

    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate input_coordinate,
                                                      const double max_distance,
                                                      const int bearing,
                                                      const int bearing_range) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodeWithAlternativeFromBigComponent(
            input_coordinate, max_distance, bearing, bearing_range);
    }

    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate input_coordinate,
                                                      const int bearing,
                                                      const int bearing_range) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodeWithAlternativeFromBigComponent(
            input_coordinate, bearing, bearing_range);
    }

    unsigned GetCheckSum() const override final { return m_check_sum; }

    NameID GetNameIndexFromEdgeID(const EdgeID id) const override final
    {
        return turn_data.GetNameID(id);
    }

    StringView GetNameForID(const NameID id) const override final
    {
        return m_name_table.GetNameForID(id);
    }

    StringView GetRefForID(const NameID id) const override final
    {
        return m_name_table.GetRefForID(id);
    }

    StringView GetPronunciationForID(const NameID id) const override final
    {
        return m_name_table.GetPronunciationForID(id);
    }

    StringView GetDestinationsForID(const NameID id) const override final
    {
        return m_name_table.GetDestinationsForID(id);
    }

    StringView GetDatasourceName(const DatasourceID id) const override final
    {
        return m_datasources->GetSourceName(id);
    }

    std::string GetTimestamp() const override final { return m_timestamp; }

    bool GetContinueStraightDefault() const override final
    {
        return m_profile_properties->continue_straight_at_waypoint;
    }

    double GetMapMatchingMaxSpeed() const override final
    {
        return m_profile_properties->max_speed_for_map_matching;
    }

    const char *GetWeightName() const override final { return m_profile_properties->weight_name; }

    unsigned GetWeightPrecision() const override final
    {
        return m_profile_properties->weight_precision;
    }

    double GetWeightMultiplier() const override final
    {
        return m_profile_properties->GetWeightMultiplier();
    }

    BearingClassID GetBearingClassID(const NodeID id) const override final
    {
        return m_bearing_class_id_table.at(id);
    }

    util::guidance::BearingClass
    GetBearingClass(const BearingClassID bearing_class_id) const override final
    {
        BOOST_ASSERT(bearing_class_id != INVALID_BEARING_CLASSID);
        auto range = m_bearing_ranges_table->GetRange(bearing_class_id);
        util::guidance::BearingClass result;
        for (auto itr = m_bearing_values_table.begin() + range.front();
             itr != m_bearing_values_table.begin() + range.back() + 1;
             ++itr)
            result.add(*itr);
        return result;
    }

    EntryClassID GetEntryClassID(const EdgeID eid) const override final
    {
        return turn_data.GetEntryClassID(eid);
    }

    util::guidance::TurnBearing PreTurnBearing(const EdgeID eid) const override final
    {
        return turn_data.GetPreTurnBearing(eid);
    }
    util::guidance::TurnBearing PostTurnBearing(const EdgeID eid) const override final
    {
        return turn_data.GetPostTurnBearing(eid);
    }

    util::guidance::EntryClass GetEntryClass(const EntryClassID entry_class_id) const override final
    {
        return m_entry_class_table.at(entry_class_id);
    }

    bool HasLaneData(const EdgeID id) const override final { return turn_data.HasLaneData(id); }

    util::guidance::LaneTupleIdPair GetLaneData(const EdgeID id) const override final
    {
        BOOST_ASSERT(HasLaneData(id));
        return m_lane_tupel_id_pairs.at(turn_data.GetLaneDataID(id));
    }

    extractor::guidance::TurnLaneDescription
    GetTurnDescription(const LaneDescriptionID lane_description_id) const override final
    {
        if (lane_description_id == INVALID_LANE_DESCRIPTIONID)
            return {};
        else
            return extractor::guidance::TurnLaneDescription(
                m_lane_description_masks.begin() + m_lane_description_offsets[lane_description_id],
                m_lane_description_masks.begin() +
                    m_lane_description_offsets[lane_description_id + 1]);
    }
};

template <typename AlgorithmT> class ContiguousInternalMemoryDataFacade;

template <>
class ContiguousInternalMemoryDataFacade<CH>
    : public ContiguousInternalMemoryDataFacadeBase,
      public ContiguousInternalMemoryAlgorithmDataFacade<CH>
{
  public:
    ContiguousInternalMemoryDataFacade(std::shared_ptr<ContiguousBlockAllocator> allocator)
        : ContiguousInternalMemoryDataFacadeBase(allocator),
          ContiguousInternalMemoryAlgorithmDataFacade<CH>(allocator)

    {
    }
};

template <>
class ContiguousInternalMemoryDataFacade<CoreCH> final
    : public ContiguousInternalMemoryDataFacade<CH>,
      public ContiguousInternalMemoryAlgorithmDataFacade<CoreCH>
{
  public:
    ContiguousInternalMemoryDataFacade(std::shared_ptr<ContiguousBlockAllocator> allocator)
        : ContiguousInternalMemoryDataFacade<CH>(allocator),
          ContiguousInternalMemoryAlgorithmDataFacade<CoreCH>(allocator)

    {
    }
};

template <> class ContiguousInternalMemoryAlgorithmDataFacade<MLD> : public AlgorithmDataFacade<MLD>
{
    // MLD data
    partition::MultiLevelPartitionView mld_partition;
    partition::CellStorageView mld_cell_storage;
    using QueryGraph = customizer::MultiLevelEdgeBasedGraphView;
    using GraphNode = QueryGraph::NodeArrayEntry;
    using GraphEdge = QueryGraph::EdgeArrayEntry;

    QueryGraph query_graph;

    void InitializeInternalPointers(storage::DataLayout &data_layout, char *memory_block)
    {
        InitializeMLDDataPointers(data_layout, memory_block);
        InitializeGraphPointer(data_layout, memory_block);
    }

    void InitializeMLDDataPointers(storage::DataLayout &data_layout, char *memory_block)
    {
        if (data_layout.GetBlockSize(storage::DataLayout::MLD_PARTITION) > 0)
        {
            BOOST_ASSERT(data_layout.GetBlockSize(storage::DataLayout::MLD_LEVEL_DATA) > 0);
            BOOST_ASSERT(data_layout.GetBlockSize(storage::DataLayout::MLD_CELL_TO_CHILDREN) > 0);

            auto level_data =
                data_layout.GetBlockPtr<partition::MultiLevelPartitionView::LevelData>(
                    memory_block, storage::DataLayout::MLD_LEVEL_DATA);

            auto mld_partition_ptr = data_layout.GetBlockPtr<PartitionID>(
                memory_block, storage::DataLayout::MLD_PARTITION);
            auto partition_entries_count =
                data_layout.GetBlockEntries(storage::DataLayout::MLD_PARTITION);
            util::vector_view<PartitionID> partition(mld_partition_ptr, partition_entries_count);

            auto mld_chilren_ptr = data_layout.GetBlockPtr<CellID>(
                memory_block, storage::DataLayout::MLD_CELL_TO_CHILDREN);
            auto children_entries_count =
                data_layout.GetBlockEntries(storage::DataLayout::MLD_CELL_TO_CHILDREN);
            util::vector_view<CellID> cell_to_children(mld_chilren_ptr, children_entries_count);

            mld_partition =
                partition::MultiLevelPartitionView{level_data, partition, cell_to_children};
        }

        if (data_layout.GetBlockSize(storage::DataLayout::MLD_CELL_WEIGHTS) > 0)
        {
            BOOST_ASSERT(data_layout.GetBlockSize(storage::DataLayout::MLD_CELLS) > 0);
            BOOST_ASSERT(data_layout.GetBlockSize(storage::DataLayout::MLD_CELL_LEVEL_OFFSETS) > 0);

            auto mld_cell_weights_ptr = data_layout.GetBlockPtr<EdgeWeight>(
                memory_block, storage::DataLayout::MLD_CELL_WEIGHTS);
            auto mld_source_boundary_ptr = data_layout.GetBlockPtr<NodeID>(
                memory_block, storage::DataLayout::MLD_CELL_SOURCE_BOUNDARY);
            auto mld_destination_boundary_ptr = data_layout.GetBlockPtr<NodeID>(
                memory_block, storage::DataLayout::MLD_CELL_DESTINATION_BOUNDARY);
            auto mld_cells_ptr = data_layout.GetBlockPtr<partition::CellStorageView::CellData>(
                memory_block, storage::DataLayout::MLD_CELLS);
            auto mld_cell_level_offsets_ptr = data_layout.GetBlockPtr<std::uint64_t>(
                memory_block, storage::DataLayout::MLD_CELL_LEVEL_OFFSETS);

            auto weight_entries_count =
                data_layout.GetBlockEntries(storage::DataLayout::MLD_CELL_WEIGHTS);
            auto source_boundary_entries_count =
                data_layout.GetBlockEntries(storage::DataLayout::MLD_CELL_SOURCE_BOUNDARY);
            auto destination_boundary_entries_count =
                data_layout.GetBlockEntries(storage::DataLayout::MLD_CELL_DESTINATION_BOUNDARY);
            auto cells_entries_counts = data_layout.GetBlockEntries(storage::DataLayout::MLD_CELLS);
            auto cell_level_offsets_entries_count =
                data_layout.GetBlockEntries(storage::DataLayout::MLD_CELL_LEVEL_OFFSETS);

            util::vector_view<EdgeWeight> weights(mld_cell_weights_ptr, weight_entries_count);
            util::vector_view<NodeID> source_boundary(mld_source_boundary_ptr,
                                                      source_boundary_entries_count);
            util::vector_view<NodeID> destination_boundary(mld_destination_boundary_ptr,
                                                           destination_boundary_entries_count);
            util::vector_view<partition::CellStorageView::CellData> cells(mld_cells_ptr,
                                                                          cells_entries_counts);
            util::vector_view<std::uint64_t> level_offsets(mld_cell_level_offsets_ptr,
                                                           cell_level_offsets_entries_count);

            mld_cell_storage = partition::CellStorageView{std::move(weights),
                                                          std::move(source_boundary),
                                                          std::move(destination_boundary),
                                                          std::move(cells),
                                                          std::move(level_offsets)};
        }
    }
    void InitializeGraphPointer(storage::DataLayout &data_layout, char *memory_block)
    {
        auto graph_nodes_ptr = data_layout.GetBlockPtr<GraphNode>(
            memory_block, storage::DataLayout::MLD_GRAPH_NODE_LIST);

        auto graph_edges_ptr = data_layout.GetBlockPtr<GraphEdge>(
            memory_block, storage::DataLayout::MLD_GRAPH_EDGE_LIST);

        auto graph_node_to_offset_ptr = data_layout.GetBlockPtr<QueryGraph::EdgeOffset>(
            memory_block, storage::DataLayout::MLD_GRAPH_NODE_TO_OFFSET);

        util::vector_view<GraphNode> node_list(
            graph_nodes_ptr, data_layout.num_entries[storage::DataLayout::MLD_GRAPH_NODE_LIST]);
        util::vector_view<GraphEdge> edge_list(
            graph_edges_ptr, data_layout.num_entries[storage::DataLayout::MLD_GRAPH_EDGE_LIST]);
        util::vector_view<QueryGraph::EdgeOffset> node_to_offset(
            graph_node_to_offset_ptr,
            data_layout.num_entries[storage::DataLayout::MLD_GRAPH_NODE_TO_OFFSET]);

        query_graph =
            QueryGraph(std::move(node_list), std::move(edge_list), std::move(node_to_offset));
    }

    // allocator that keeps the allocation data
    std::shared_ptr<ContiguousBlockAllocator> allocator;

  public:
    ContiguousInternalMemoryAlgorithmDataFacade(
        std::shared_ptr<ContiguousBlockAllocator> allocator_)
        : allocator(std::move(allocator_))
    {
        InitializeInternalPointers(allocator->GetLayout(), allocator->GetMemory());
    }

    const partition::MultiLevelPartitionView &GetMultiLevelPartition() const override
    {
        return mld_partition;
    }

    const partition::CellStorageView &GetCellStorage() const override { return mld_cell_storage; }

    // search graph access
    unsigned GetNumberOfNodes() const override final { return query_graph.GetNumberOfNodes(); }

    unsigned GetNumberOfEdges() const override final { return query_graph.GetNumberOfEdges(); }

    unsigned GetOutDegree(const NodeID n) const override final
    {
        return query_graph.GetOutDegree(n);
    }

    NodeID GetTarget(const EdgeID e) const override final { return query_graph.GetTarget(e); }

    const EdgeData &GetEdgeData(const EdgeID e) const override final
    {
        return query_graph.GetEdgeData(e);
    }

    EdgeID BeginEdges(const NodeID n) const override final { return query_graph.BeginEdges(n); }

    EdgeID EndEdges(const NodeID n) const override final { return query_graph.EndEdges(n); }

    EdgeRange GetAdjacentEdgeRange(const NodeID node) const override final
    {
        return query_graph.GetAdjacentEdgeRange(node);
    }

    EdgeRange GetBorderEdgeRange(const LevelID level, const NodeID node) const override final
    {
        return query_graph.GetBorderEdgeRange(level, node);
    }

    // searches for a specific edge
    EdgeID FindEdge(const NodeID from, const NodeID to) const override final
    {
        return query_graph.FindEdge(from, to);
    }
};

template <>
class ContiguousInternalMemoryDataFacade<MLD> final
    : public ContiguousInternalMemoryDataFacadeBase,
      public ContiguousInternalMemoryAlgorithmDataFacade<MLD>
{
  private:
  public:
    ContiguousInternalMemoryDataFacade(std::shared_ptr<ContiguousBlockAllocator> allocator)
        : ContiguousInternalMemoryDataFacadeBase(allocator),
          ContiguousInternalMemoryAlgorithmDataFacade<MLD>(allocator)

    {
    }
};
}
}
}

#endif // CONTIGUOUS_INTERNALMEM_DATAFACADE_HPP

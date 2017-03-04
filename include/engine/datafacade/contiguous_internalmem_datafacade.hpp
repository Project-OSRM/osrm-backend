#ifndef CONTIGUOUS_INTERNALMEM_DATAFACADE_HPP
#define CONTIGUOUS_INTERNALMEM_DATAFACADE_HPP

#include "engine/datafacade/algorithm_datafacade.hpp"
#include "engine/datafacade/contiguous_block_allocator.hpp"
#include "engine/datafacade/datafacade_base.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/guidance/turn_lane_types.hpp"
#include "extractor/profile_properties.hpp"
#include "storage/shared_datatype.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/guidance/turn_lanes.hpp"

#include "engine/algorithm.hpp"
#include "engine/geospatial_query.hpp"
#include "util/cell_storage.hpp"
#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/guidance/turn_bearing.hpp"
#include "util/log.hpp"
#include "util/multi_level_partition.hpp"
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
class ContiguousInternalMemoryAlgorithmDataFacade<algorithm::CH>
    : public datafacade::AlgorithmDataFacade<algorithm::CH>
{
  private:
    using QueryGraph = util::StaticGraph<EdgeData, true>;
    using GraphNode = QueryGraph::NodeArrayEntry;
    using GraphEdge = QueryGraph::EdgeArrayEntry;

    std::unique_ptr<QueryGraph> m_query_graph;

    // allocator that keeps the allocation data
    std::shared_ptr<ContiguousBlockAllocator> allocator;

    void InitializeGraphPointer(storage::DataLayout &data_layout, char *memory_block)
    {
        auto graph_nodes_ptr = data_layout.GetBlockPtr<GraphNode>(
            memory_block, storage::DataLayout::CH_GRAPH_NODE_LIST);

        auto graph_edges_ptr = data_layout.GetBlockPtr<GraphEdge>(
            memory_block, storage::DataLayout::CH_GRAPH_EDGE_LIST);

        util::ShM<GraphNode, true>::vector node_list(
            graph_nodes_ptr, data_layout.num_entries[storage::DataLayout::CH_GRAPH_NODE_LIST]);
        util::ShM<GraphEdge, true>::vector edge_list(
            graph_edges_ptr, data_layout.num_entries[storage::DataLayout::CH_GRAPH_EDGE_LIST]);
        m_query_graph.reset(new QueryGraph(node_list, edge_list));
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
    unsigned GetNumberOfNodes() const override final { return m_query_graph->GetNumberOfNodes(); }

    unsigned GetNumberOfEdges() const override final { return m_query_graph->GetNumberOfEdges(); }

    unsigned GetOutDegree(const NodeID n) const override final
    {
        return m_query_graph->GetOutDegree(n);
    }

    NodeID GetTarget(const EdgeID e) const override final { return m_query_graph->GetTarget(e); }

    EdgeData &GetEdgeData(const EdgeID e) const override final
    {
        return m_query_graph->GetEdgeData(e);
    }

    EdgeID BeginEdges(const NodeID n) const override final { return m_query_graph->BeginEdges(n); }

    EdgeID EndEdges(const NodeID n) const override final { return m_query_graph->EndEdges(n); }

    EdgeRange GetAdjacentEdgeRange(const NodeID node) const override final
    {
        return m_query_graph->GetAdjacentEdgeRange(node);
    }

    // searches for a specific edge
    EdgeID FindEdge(const NodeID from, const NodeID to) const override final
    {
        return m_query_graph->FindEdge(from, to);
    }

    EdgeID FindEdgeInEitherDirection(const NodeID from, const NodeID to) const override final
    {
        return m_query_graph->FindEdgeInEitherDirection(from, to);
    }

    EdgeID
    FindEdgeIndicateIfReverse(const NodeID from, const NodeID to, bool &result) const override final
    {
        return m_query_graph->FindEdgeIndicateIfReverse(from, to, result);
    }

    EdgeID FindSmallestEdge(const NodeID from,
                            const NodeID to,
                            std::function<bool(EdgeData)> filter) const override final
    {
        return m_query_graph->FindSmallestEdge(from, to, filter);
    }
};

template <>
class ContiguousInternalMemoryAlgorithmDataFacade<algorithm::CoreCH>
    : public datafacade::AlgorithmDataFacade<algorithm::CoreCH>
{
  private:
    util::ShM<bool, true>::vector m_is_core_node;

    // allocator that keeps the allocation data
    std::shared_ptr<ContiguousBlockAllocator> allocator;

    void InitializeCoreInformationPointer(storage::DataLayout &data_layout, char *memory_block)
    {
        auto core_marker_ptr =
            data_layout.GetBlockPtr<unsigned>(memory_block, storage::DataLayout::CH_CORE_MARKER);
        util::ShM<bool, true>::vector is_core_node(
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
    using IndexBlock = util::RangeTable<16, true>::BlockT;
    using RTreeLeaf = super::RTreeLeaf;
    using SharedRTree =
        util::StaticRTree<RTreeLeaf, util::ShM<util::Coordinate, true>::vector, true>;
    using SharedGeospatialQuery = GeospatialQuery<SharedRTree, BaseDataFacade>;
    using RTreeNode = SharedRTree::TreeNode;

    std::string m_timestamp;
    extractor::ProfileProperties *m_profile_properties;

    unsigned m_check_sum;
    util::ShM<util::Coordinate, true>::vector m_coordinate_list;
    util::PackedVector<OSMNodeID, true> m_osmnodeid_list;
    util::ShM<GeometryID, true>::vector m_via_geometry_list;
    util::ShM<NameID, true>::vector m_name_ID_list;
    util::ShM<LaneDataID, true>::vector m_lane_data_id;
    util::ShM<extractor::guidance::TurnInstruction, true>::vector m_turn_instruction_list;
    util::ShM<extractor::TravelMode, true>::vector m_travel_mode_list;
    util::ShM<util::guidance::TurnBearing, true>::vector m_pre_turn_bearing;
    util::ShM<util::guidance::TurnBearing, true>::vector m_post_turn_bearing;
    util::NameTable m_names_table;
    util::ShM<unsigned, true>::vector m_name_begin_indices;
    util::ShM<unsigned, true>::vector m_geometry_indices;
    util::ShM<NodeID, true>::vector m_geometry_node_list;
    util::ShM<EdgeWeight, true>::vector m_geometry_fwd_weight_list;
    util::ShM<EdgeWeight, true>::vector m_geometry_rev_weight_list;
    util::ShM<EdgeWeight, true>::vector m_geometry_fwd_duration_list;
    util::ShM<EdgeWeight, true>::vector m_geometry_rev_duration_list;
    util::ShM<bool, true>::vector m_is_core_node;
    util::ShM<DatasourceID, true>::vector m_datasource_list;
    util::ShM<std::uint32_t, true>::vector m_lane_description_offsets;
    util::ShM<extractor::guidance::TurnLaneType::Mask, true>::vector m_lane_description_masks;
    util::ShM<TurnPenalty, true>::vector m_turn_weight_penalties;
    util::ShM<TurnPenalty, true>::vector m_turn_duration_penalties;

    util::ShM<char, true>::vector m_datasource_name_data;
    util::ShM<std::size_t, true>::vector m_datasource_name_offsets;
    util::ShM<std::size_t, true>::vector m_datasource_name_lengths;
    util::ShM<util::guidance::LaneTupleIdPair, true>::vector m_lane_tupel_id_pairs;

    std::unique_ptr<SharedRTree> m_static_rtree;
    std::unique_ptr<SharedGeospatialQuery> m_geospatial_query;
    boost::filesystem::path file_index_path;

    util::NameTable m_name_table;
    // bearing classes by node based node
    util::ShM<BearingClassID, true>::vector m_bearing_class_id_table;
    // entry class IDs
    util::ShM<EntryClassID, true>::vector m_entry_class_id_list;

    // the look-up table for entry classes. An entry class lists the possibility of entry for all
    // available turns. Such a class id is stored with every edge.
    util::ShM<util::guidance::EntryClass, true>::vector m_entry_class_table;
    // the look-up table for distinct bearing classes. A bearing class lists the available bearings
    // at an intersection
    std::shared_ptr<util::RangeTable<16, true>> m_bearing_ranges_table;
    util::ShM<DiscreteBearing, true>::vector m_bearing_values_table;

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

    void InitializeNodeAndEdgeInformationPointers(storage::DataLayout &data_layout,
                                                  char *memory_block)
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

        const auto travel_mode_list_ptr = data_layout.GetBlockPtr<extractor::TravelMode>(
            memory_block, storage::DataLayout::TRAVEL_MODE);
        util::ShM<extractor::TravelMode, true>::vector travel_mode_list(
            travel_mode_list_ptr, data_layout.num_entries[storage::DataLayout::TRAVEL_MODE]);
        m_travel_mode_list = std::move(travel_mode_list);

        const auto lane_data_id_ptr =
            data_layout.GetBlockPtr<LaneDataID>(memory_block, storage::DataLayout::LANE_DATA_ID);
        util::ShM<LaneDataID, true>::vector lane_data_id(
            lane_data_id_ptr, data_layout.num_entries[storage::DataLayout::LANE_DATA_ID]);
        m_lane_data_id = std::move(lane_data_id);

        const auto lane_tupel_id_pair_ptr =
            data_layout.GetBlockPtr<util::guidance::LaneTupleIdPair>(
                memory_block, storage::DataLayout::TURN_LANE_DATA);
        util::ShM<util::guidance::LaneTupleIdPair, true>::vector lane_tupel_id_pair(
            lane_tupel_id_pair_ptr, data_layout.num_entries[storage::DataLayout::TURN_LANE_DATA]);
        m_lane_tupel_id_pairs = std::move(lane_tupel_id_pair);

        const auto turn_instruction_list_ptr =
            data_layout.GetBlockPtr<extractor::guidance::TurnInstruction>(
                memory_block, storage::DataLayout::TURN_INSTRUCTION);
        util::ShM<extractor::guidance::TurnInstruction, true>::vector turn_instruction_list(
            turn_instruction_list_ptr,
            data_layout.num_entries[storage::DataLayout::TURN_INSTRUCTION]);
        m_turn_instruction_list = std::move(turn_instruction_list);

        const auto name_id_list_ptr =
            data_layout.GetBlockPtr<NameID>(memory_block, storage::DataLayout::NAME_ID_LIST);
        util::ShM<NameID, true>::vector name_id_list(
            name_id_list_ptr, data_layout.num_entries[storage::DataLayout::NAME_ID_LIST]);
        m_name_ID_list = std::move(name_id_list);

        const auto entry_class_id_list_ptr =
            data_layout.GetBlockPtr<EntryClassID>(memory_block, storage::DataLayout::ENTRY_CLASSID);
        typename util::ShM<EntryClassID, true>::vector entry_class_id_list(
            entry_class_id_list_ptr, data_layout.num_entries[storage::DataLayout::ENTRY_CLASSID]);
        m_entry_class_id_list = std::move(entry_class_id_list);

        const auto pre_turn_bearing_ptr = data_layout.GetBlockPtr<util::guidance::TurnBearing>(
            memory_block, storage::DataLayout::PRE_TURN_BEARING);
        typename util::ShM<util::guidance::TurnBearing, true>::vector pre_turn_bearing(
            pre_turn_bearing_ptr, data_layout.num_entries[storage::DataLayout::PRE_TURN_BEARING]);
        m_pre_turn_bearing = std::move(pre_turn_bearing);

        const auto post_turn_bearing_ptr = data_layout.GetBlockPtr<util::guidance::TurnBearing>(
            memory_block, storage::DataLayout::POST_TURN_BEARING);
        typename util::ShM<util::guidance::TurnBearing, true>::vector post_turn_bearing(
            post_turn_bearing_ptr, data_layout.num_entries[storage::DataLayout::POST_TURN_BEARING]);
        m_post_turn_bearing = std::move(post_turn_bearing);
    }

    void InitializeViaNodeListPointer(storage::DataLayout &data_layout, char *memory_block)
    {
        auto via_geometry_list_ptr =
            data_layout.GetBlockPtr<GeometryID>(memory_block, storage::DataLayout::VIA_NODE_LIST);
        util::ShM<GeometryID, true>::vector via_geometry_list(
            via_geometry_list_ptr, data_layout.num_entries[storage::DataLayout::VIA_NODE_LIST]);
        m_via_geometry_list = std::move(via_geometry_list);
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
        util::ShM<std::uint32_t, true>::vector offsets(
            offsets_ptr, data_layout.num_entries[storage::DataLayout::LANE_DESCRIPTION_OFFSETS]);
        m_lane_description_offsets = std::move(offsets);

        auto masks_ptr = data_layout.GetBlockPtr<extractor::guidance::TurnLaneType::Mask>(
            memory_block, storage::DataLayout::LANE_DESCRIPTION_MASKS);

        util::ShM<extractor::guidance::TurnLaneType::Mask, true>::vector masks(
            masks_ptr, data_layout.num_entries[storage::DataLayout::LANE_DESCRIPTION_MASKS]);
        m_lane_description_masks = std::move(masks);
    }

    void InitializeTurnPenalties(storage::DataLayout &data_layout, char *memory_block)
    {
        auto turn_weight_penalties_ptr = data_layout.GetBlockPtr<TurnPenalty>(
            memory_block, storage::DataLayout::TURN_WEIGHT_PENALTIES);
        m_turn_weight_penalties = util::ShM<TurnPenalty, true>::vector(
            turn_weight_penalties_ptr,
            data_layout.num_entries[storage::DataLayout::TURN_WEIGHT_PENALTIES]);
        if (data_layout.num_entries[storage::DataLayout::TURN_DURATION_PENALTIES] == 0)
        { // Fallback to turn weight penalties that are turn duration penalties in deciseconds
            m_turn_duration_penalties = util::ShM<TurnPenalty, true>::vector(
                turn_weight_penalties_ptr,
                data_layout.num_entries[storage::DataLayout::TURN_WEIGHT_PENALTIES]);
        }
        else
        {
            auto turn_duration_penalties_ptr = data_layout.GetBlockPtr<TurnPenalty>(
                memory_block, storage::DataLayout::TURN_DURATION_PENALTIES);
            m_turn_duration_penalties = util::ShM<TurnPenalty, true>::vector(
                turn_duration_penalties_ptr,
                data_layout.num_entries[storage::DataLayout::TURN_DURATION_PENALTIES]);
        }
    }

    void InitializeGeometryPointers(storage::DataLayout &data_layout, char *memory_block)
    {
        auto geometries_index_ptr =
            data_layout.GetBlockPtr<unsigned>(memory_block, storage::DataLayout::GEOMETRIES_INDEX);
        util::ShM<unsigned, true>::vector geometry_begin_indices(
            geometries_index_ptr, data_layout.num_entries[storage::DataLayout::GEOMETRIES_INDEX]);
        m_geometry_indices = std::move(geometry_begin_indices);

        auto geometries_node_list_ptr = data_layout.GetBlockPtr<NodeID>(
            memory_block, storage::DataLayout::GEOMETRIES_NODE_LIST);
        util::ShM<NodeID, true>::vector geometry_node_list(
            geometries_node_list_ptr,
            data_layout.num_entries[storage::DataLayout::GEOMETRIES_NODE_LIST]);
        m_geometry_node_list = std::move(geometry_node_list);

        auto geometries_fwd_weight_list_ptr = data_layout.GetBlockPtr<EdgeWeight>(
            memory_block, storage::DataLayout::GEOMETRIES_FWD_WEIGHT_LIST);
        util::ShM<EdgeWeight, true>::vector geometry_fwd_weight_list(
            geometries_fwd_weight_list_ptr,
            data_layout.num_entries[storage::DataLayout::GEOMETRIES_FWD_WEIGHT_LIST]);
        m_geometry_fwd_weight_list = std::move(geometry_fwd_weight_list);

        auto geometries_rev_weight_list_ptr = data_layout.GetBlockPtr<EdgeWeight>(
            memory_block, storage::DataLayout::GEOMETRIES_REV_WEIGHT_LIST);
        util::ShM<EdgeWeight, true>::vector geometry_rev_weight_list(
            geometries_rev_weight_list_ptr,
            data_layout.num_entries[storage::DataLayout::GEOMETRIES_REV_WEIGHT_LIST]);
        m_geometry_rev_weight_list = std::move(geometry_rev_weight_list);

        auto datasources_list_ptr = data_layout.GetBlockPtr<DatasourceID>(
            memory_block, storage::DataLayout::DATASOURCES_LIST);
        util::ShM<DatasourceID, true>::vector datasources_list(
            datasources_list_ptr, data_layout.num_entries[storage::DataLayout::DATASOURCES_LIST]);
        m_datasource_list = std::move(datasources_list);

        auto geometries_fwd_duration_list_ptr = data_layout.GetBlockPtr<EdgeWeight>(
            memory_block, storage::DataLayout::GEOMETRIES_FWD_DURATION_LIST);
        util::ShM<EdgeWeight, true>::vector geometry_fwd_duration_list(
            geometries_fwd_duration_list_ptr,
            data_layout.num_entries[storage::DataLayout::GEOMETRIES_FWD_DURATION_LIST]);
        m_geometry_fwd_duration_list = std::move(geometry_fwd_duration_list);

        auto geometries_rev_duration_list_ptr = data_layout.GetBlockPtr<EdgeWeight>(
            memory_block, storage::DataLayout::GEOMETRIES_REV_DURATION_LIST);
        util::ShM<EdgeWeight, true>::vector geometry_rev_duration_list(
            geometries_rev_duration_list_ptr,
            data_layout.num_entries[storage::DataLayout::GEOMETRIES_REV_DURATION_LIST]);
        m_geometry_rev_duration_list = std::move(geometry_rev_duration_list);

        auto datasource_name_data_ptr =
            data_layout.GetBlockPtr<char>(memory_block, storage::DataLayout::DATASOURCE_NAME_DATA);
        util::ShM<char, true>::vector datasource_name_data(
            datasource_name_data_ptr,
            data_layout.num_entries[storage::DataLayout::DATASOURCE_NAME_DATA]);
        m_datasource_name_data = std::move(datasource_name_data);

        auto datasource_name_offsets_ptr = data_layout.GetBlockPtr<std::size_t>(
            memory_block, storage::DataLayout::DATASOURCE_NAME_OFFSETS);
        util::ShM<std::size_t, true>::vector datasource_name_offsets(
            datasource_name_offsets_ptr,
            data_layout.num_entries[storage::DataLayout::DATASOURCE_NAME_OFFSETS]);
        m_datasource_name_offsets = std::move(datasource_name_offsets);

        auto datasource_name_lengths_ptr = data_layout.GetBlockPtr<std::size_t>(
            memory_block, storage::DataLayout::DATASOURCE_NAME_LENGTHS);
        util::ShM<std::size_t, true>::vector datasource_name_lengths(
            datasource_name_lengths_ptr,
            data_layout.num_entries[storage::DataLayout::DATASOURCE_NAME_LENGTHS]);
        m_datasource_name_lengths = std::move(datasource_name_lengths);
    }

    void InitializeIntersectionClassPointers(storage::DataLayout &data_layout, char *memory_block)
    {
        auto bearing_class_id_ptr = data_layout.GetBlockPtr<BearingClassID>(
            memory_block, storage::DataLayout::BEARING_CLASSID);
        typename util::ShM<BearingClassID, true>::vector bearing_class_id_table(
            bearing_class_id_ptr, data_layout.num_entries[storage::DataLayout::BEARING_CLASSID]);
        m_bearing_class_id_table = std::move(bearing_class_id_table);

        auto bearing_class_ptr = data_layout.GetBlockPtr<DiscreteBearing>(
            memory_block, storage::DataLayout::BEARING_VALUES);
        typename util::ShM<DiscreteBearing, true>::vector bearing_class_table(
            bearing_class_ptr, data_layout.num_entries[storage::DataLayout::BEARING_VALUES]);
        m_bearing_values_table = std::move(bearing_class_table);

        auto offsets_ptr =
            data_layout.GetBlockPtr<unsigned>(memory_block, storage::DataLayout::BEARING_OFFSETS);
        auto blocks_ptr =
            data_layout.GetBlockPtr<IndexBlock>(memory_block, storage::DataLayout::BEARING_BLOCKS);
        util::ShM<unsigned, true>::vector bearing_offsets(
            offsets_ptr, data_layout.num_entries[storage::DataLayout::BEARING_OFFSETS]);
        util::ShM<IndexBlock, true>::vector bearing_blocks(
            blocks_ptr, data_layout.num_entries[storage::DataLayout::BEARING_BLOCKS]);

        m_bearing_ranges_table = std::make_unique<util::RangeTable<16, true>>(
            bearing_offsets, bearing_blocks, static_cast<unsigned>(m_bearing_values_table.size()));

        auto entry_class_ptr = data_layout.GetBlockPtr<util::guidance::EntryClass>(
            memory_block, storage::DataLayout::ENTRY_CLASS);
        typename util::ShM<util::guidance::EntryClass, true>::vector entry_class_table(
            entry_class_ptr, data_layout.num_entries[storage::DataLayout::ENTRY_CLASS]);
        m_entry_class_table = std::move(entry_class_table);
    }

    void InitializeInternalPointers(storage::DataLayout &data_layout, char *memory_block)
    {
        InitializeChecksumPointer(data_layout, memory_block);
        InitializeNodeAndEdgeInformationPointers(data_layout, memory_block);
        InitializeTurnPenalties(data_layout, memory_block);
        InitializeGeometryPointers(data_layout, memory_block);
        InitializeTimestampPointer(data_layout, memory_block);
        InitializeViaNodeListPointer(data_layout, memory_block);
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
        /*
         * NodeID's for geometries are stored in one place for
         * both forward and reverse segments along the same bi-
         * directional edge. The m_geometry_indices stores
         * refences to where to find the beginning of the bi-
         * directional edge in the m_geometry_node_list vector. For
         * forward geometries of bi-directional edges, edges 2 to
         * n of that edge need to be read.
         */
        const auto begin = m_geometry_indices.at(id);
        const auto end = m_geometry_indices.at(id + 1);

        std::vector<NodeID> result_nodes;
        result_nodes.reserve(end - begin);

        std::copy(m_geometry_node_list.begin() + begin,
                  m_geometry_node_list.begin() + end,
                  std::back_inserter(result_nodes));

        return result_nodes;
    }

    virtual std::vector<NodeID> GetUncompressedReverseGeometry(const EdgeID id) const override final
    {
        /*
         * NodeID's for geometries are stored in one place for
         * both forward and reverse segments along the same bi-
         * directional edge. The m_geometry_indices stores
         * refences to where to find the beginning of the bi-
         * directional edge in the m_geometry_node_list vector.
         * */
        const auto begin = m_geometry_indices.at(id);
        const auto end = m_geometry_indices.at(id + 1);

        std::vector<NodeID> result_nodes;
        result_nodes.reserve(end - begin);

        std::reverse_copy(m_geometry_node_list.begin() + begin,
                          m_geometry_node_list.begin() + end,
                          std::back_inserter(result_nodes));

        return result_nodes;
    }

    virtual std::vector<EdgeWeight>
    GetUncompressedForwardDurations(const EdgeID id) const override final
    {
        /*
         * EdgeWeights's for geometries are stored in one place for
         * both forward and reverse segments along the same bi-
         * directional edge. The m_geometry_indices stores
         * refences to where to find the beginning of the bi-
         * directional edge in the m_geometry_fwd_weight_list vector.
         * */
        const auto begin = m_geometry_indices.at(id) + 1;
        const auto end = m_geometry_indices.at(id + 1);

        std::vector<EdgeWeight> result_durations;
        result_durations.reserve(end - begin);

        std::copy(m_geometry_fwd_duration_list.begin() + begin,
                  m_geometry_fwd_duration_list.begin() + end,
                  std::back_inserter(result_durations));

        return result_durations;
    }

    virtual std::vector<EdgeWeight>
    GetUncompressedReverseDurations(const EdgeID id) const override final
    {
        /*
         * EdgeWeights for geometries are stored in one place for
         * both forward and reverse segments along the same bi-
         * directional edge. The m_geometry_indices stores
         * refences to where to find the beginning of the bi-
         * directional edge in the m_geometry_rev_weight_list vector. For
         * reverse durations of bi-directional edges, edges 1 to
         * n-1 of that edge need to be read in reverse.
         */
        const auto begin = m_geometry_indices.at(id);
        const auto end = m_geometry_indices.at(id + 1) - 1;

        std::vector<EdgeWeight> result_durations;
        result_durations.reserve(end - begin);

        std::reverse_copy(m_geometry_rev_duration_list.begin() + begin,
                          m_geometry_rev_duration_list.begin() + end,
                          std::back_inserter(result_durations));

        return result_durations;
    }

    virtual std::vector<EdgeWeight>
    GetUncompressedForwardWeights(const EdgeID id) const override final
    {
        /*
         * EdgeWeights's for geometries are stored in one place for
         * both forward and reverse segments along the same bi-
         * directional edge. The m_geometry_indices stores
         * refences to where to find the beginning of the bi-
         * directional edge in the m_geometry_fwd_weight_list vector.
         * */
        const auto begin = m_geometry_indices.at(id) + 1;
        const auto end = m_geometry_indices.at(id + 1);

        std::vector<EdgeWeight> result_weights;
        result_weights.reserve(end - begin);

        std::copy(m_geometry_fwd_weight_list.begin() + begin,
                  m_geometry_fwd_weight_list.begin() + end,
                  std::back_inserter(result_weights));

        return result_weights;
    }

    virtual std::vector<EdgeWeight>
    GetUncompressedReverseWeights(const EdgeID id) const override final
    {
        /*
         * EdgeWeights for geometries are stored in one place for
         * both forward and reverse segments along the same bi-
         * directional edge. The m_geometry_indices stores
         * refences to where to find the beginning of the bi-
         * directional edge in the m_geometry_rev_weight_list vector. For
         * reverse weights of bi-directional edges, edges 1 to
         * n-1 of that edge need to be read in reverse.
         */
        const auto begin = m_geometry_indices.at(id);
        const auto end = m_geometry_indices.at(id + 1) - 1;

        std::vector<EdgeWeight> result_weights;
        result_weights.reserve(end - begin);

        std::reverse_copy(m_geometry_rev_weight_list.begin() + begin,
                          m_geometry_rev_weight_list.begin() + end,
                          std::back_inserter(result_weights));

        return result_weights;
    }

    virtual GeometryID GetGeometryIndexForEdgeID(const EdgeID id) const override final
    {
        return m_via_geometry_list.at(id);
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
        return m_turn_instruction_list.at(id);
    }

    extractor::TravelMode GetTravelModeForEdgeID(const EdgeID id) const override final
    {
        return m_travel_mode_list.at(id);
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
        return m_name_ID_list.at(id);
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

    // Returns the data source ids that were used to supply the edge
    // weights.
    virtual std::vector<DatasourceID>
    GetUncompressedForwardDatasources(const EdgeID id) const override final
    {
        /*
         * Data sources for geometries are stored in one place for
         * both forward and reverse segments along the same bi-
         * directional edge. The m_geometry_indices stores
         * refences to where to find the beginning of the bi-
         * directional edge in the m_geometry_list vector. For
         * forward datasources of bi-directional edges, edges 2 to
         * n of that edge need to be read.
         */
        const auto begin = m_geometry_indices.at(id) + 1;
        const auto end = m_geometry_indices.at(id + 1);

        std::vector<DatasourceID> result_datasources;
        result_datasources.reserve(end - begin);

        // If there was no datasource info, return an array of 0's.
        if (m_datasource_list.empty())
        {
            result_datasources.resize(end - begin, 0);
        }
        else
        {
            std::copy(m_datasource_list.begin() + begin,
                      m_datasource_list.begin() + end,
                      std::back_inserter(result_datasources));
        }

        return result_datasources;
    }

    // Returns the data source ids that were used to supply the edge
    // weights.
    virtual std::vector<DatasourceID>
    GetUncompressedReverseDatasources(const EdgeID id) const override final
    {
        /*
         * Datasources for geometries are stored in one place for
         * both forward and reverse segments along the same bi-
         * directional edge. The m_geometry_indices stores
         * refences to where to find the beginning of the bi-
         * directional edge in the m_geometry_list vector. For
         * reverse datasources of bi-directional edges, edges 1 to
         * n-1 of that edge need to be read in reverse.
         */
        const unsigned begin = m_geometry_indices.at(id);
        const unsigned end = m_geometry_indices.at(id + 1) - 1;

        std::vector<DatasourceID> result_datasources;
        result_datasources.reserve(end - begin);

        // If there was no datasource info, return an array of 0's.
        if (m_datasource_list.empty())
        {
            result_datasources.resize(end - begin, 0);
        }
        else
        {
            std::reverse_copy(m_datasource_list.begin() + begin,
                              m_datasource_list.begin() + end,
                              std::back_inserter(result_datasources));
        }

        return result_datasources;
    }

    StringView GetDatasourceName(const DatasourceID id) const override final
    {
        BOOST_ASSERT(m_datasource_name_offsets.size() >= 1);
        BOOST_ASSERT(m_datasource_name_offsets.size() > id);

        auto first = m_datasource_name_data.begin() + m_datasource_name_offsets[id];
        auto last = m_datasource_name_data.begin() + m_datasource_name_offsets[id] +
                    m_datasource_name_lengths[id];
        // These iterators are useless: they're InputIterators onto a contiguous block of memory.
        // Deref to get to the first element, then Addressof to get the memory address of the it.
        const std::size_t len = &*last - &*first;

        return StringView{&*first, len};
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
        return m_entry_class_id_list.at(eid);
    }

    util::guidance::TurnBearing PreTurnBearing(const EdgeID eid) const override final
    {
        return m_pre_turn_bearing.at(eid);
    }
    util::guidance::TurnBearing PostTurnBearing(const EdgeID eid) const override final
    {
        return m_post_turn_bearing.at(eid);
    }

    util::guidance::EntryClass GetEntryClass(const EntryClassID entry_class_id) const override final
    {
        return m_entry_class_table.at(entry_class_id);
    }

    bool hasLaneData(const EdgeID id) const override final
    {
        return INVALID_LANE_DATAID != m_lane_data_id.at(id);
    }

    util::guidance::LaneTupleIdPair GetLaneData(const EdgeID id) const override final
    {
        BOOST_ASSERT(hasLaneData(id));
        return m_lane_tupel_id_pairs.at(m_lane_data_id.at(id));
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
class ContiguousInternalMemoryDataFacade<algorithm::CH>
    : public ContiguousInternalMemoryDataFacadeBase,
      public ContiguousInternalMemoryAlgorithmDataFacade<algorithm::CH>
{
  public:
    ContiguousInternalMemoryDataFacade(std::shared_ptr<ContiguousBlockAllocator> allocator)
        : ContiguousInternalMemoryDataFacadeBase(allocator),
          ContiguousInternalMemoryAlgorithmDataFacade<algorithm::CH>(allocator)

    {
    }
};

template <>
class ContiguousInternalMemoryDataFacade<algorithm::CoreCH> final
    : public ContiguousInternalMemoryDataFacade<algorithm::CH>,
      public ContiguousInternalMemoryAlgorithmDataFacade<algorithm::CoreCH>
{
  public:
    ContiguousInternalMemoryDataFacade(std::shared_ptr<ContiguousBlockAllocator> allocator)
        : ContiguousInternalMemoryDataFacade<algorithm::CH>(allocator),
          ContiguousInternalMemoryAlgorithmDataFacade<algorithm::CoreCH>(allocator)

    {
    }
};

template <>
class ContiguousInternalMemoryAlgorithmDataFacade<algorithm::MLD>
    : public datafacade::AlgorithmDataFacade<algorithm::MLD>
{
    // MLD data
    util::MultiLevelPartitionView mld_partition;
    util::CellStorageView mld_cell_storage;

    void InitializeInternalPointers(storage::DataLayout &data_layout, char *memory_block)
    {
        InitializeMLDDataPointers(data_layout, memory_block);
    }

    void InitializeMLDDataPointers(storage::DataLayout &data_layout, char *memory_block)
    {
        if (data_layout.GetBlockSize(storage::DataLayout::MLD_PARTITION) > 0)
        {
            BOOST_ASSERT(data_layout.GetBlockSize(storage::DataLayout::MLD_LEVEL_DATA) > 0);
            BOOST_ASSERT(data_layout.GetBlockSize(storage::DataLayout::MLD_CELL_TO_CHILDREN) > 0);

            auto level_data = *data_layout.GetBlockPtr<util::MultiLevelPartitionView::LevelData>(
                memory_block, storage::DataLayout::MLD_PARTITION);

            auto mld_partition_ptr = data_layout.GetBlockPtr<util::PartitionID>(
                memory_block, storage::DataLayout::MLD_PARTITION);
            auto partition_entries_count =
                data_layout.GetBlockEntries(storage::DataLayout::MLD_PARTITION);
            util::ShM<util::PartitionID, true>::vector partition(mld_partition_ptr,
                                                                 partition_entries_count);

            auto mld_chilren_ptr = data_layout.GetBlockPtr<util::CellID>(
                memory_block, storage::DataLayout::MLD_CELL_TO_CHILDREN);
            auto children_entries_count =
                data_layout.GetBlockEntries(storage::DataLayout::MLD_CELL_TO_CHILDREN);
            util::ShM<util::CellID, true>::vector cell_to_children(mld_chilren_ptr,
                                                                   children_entries_count);

            mld_partition = util::MultiLevelPartitionView{level_data, partition, cell_to_children};
        }

        if (data_layout.GetBlockSize(storage::DataLayout::MLD_CELL_WEIGHTS) > 0)
        {
            BOOST_ASSERT(data_layout.GetBlockSize(storage::DataLayout::MLD_CELL_SOURCE_BOUNDARY) >
                         0);
            BOOST_ASSERT(
                data_layout.GetBlockSize(storage::DataLayout::MLD_CELL_DESTINATION_BOUNDARY) > 0);
            BOOST_ASSERT(data_layout.GetBlockSize(storage::DataLayout::MLD_CELLS) > 0);
            BOOST_ASSERT(data_layout.GetBlockSize(storage::DataLayout::MLD_CELL_LEVEL_OFFSETS) > 0);

            auto mld_cell_weights_ptr = data_layout.GetBlockPtr<EdgeWeight>(
                memory_block, storage::DataLayout::MLD_CELL_WEIGHTS);
            auto mld_source_boundary_ptr = data_layout.GetBlockPtr<NodeID>(
                memory_block, storage::DataLayout::MLD_CELL_SOURCE_BOUNDARY);
            auto mld_destination_boundary_ptr = data_layout.GetBlockPtr<NodeID>(
                memory_block, storage::DataLayout::MLD_CELL_DESTINATION_BOUNDARY);
            auto mld_cells_ptr = data_layout.GetBlockPtr<util::CellStorageView::CellData>(
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

            util::ShM<EdgeWeight, true>::vector weights(mld_cell_weights_ptr, weight_entries_count);
            util::ShM<NodeID, true>::vector source_boundary(mld_source_boundary_ptr,
                                                            source_boundary_entries_count);
            util::ShM<NodeID, true>::vector destination_boundary(
                mld_destination_boundary_ptr, destination_boundary_entries_count);
            util::ShM<util::CellStorageView::CellData, true>::vector cells(mld_cells_ptr,
                                                                       cells_entries_counts);
            util::ShM<std::uint64_t, true>::vector level_offsets(mld_cell_level_offsets_ptr,
                                                                 cell_level_offsets_entries_count);

            mld_cell_storage = util::CellStorageView{std::move(weights),
                                                     std::move(source_boundary),
                                                     std::move(destination_boundary),
                                                     std::move(cells),
                                                     std::move(level_offsets)};
        }
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

    const util::MultiLevelPartitionView &GetMultiLevelPartition() const { return mld_partition; }

    const util::CellStorageView &GetCellStorage() const { return mld_cell_storage; }
};

template <>
class ContiguousInternalMemoryDataFacade<algorithm::MLD>
    : public ContiguousInternalMemoryDataFacadeBase,
      public ContiguousInternalMemoryAlgorithmDataFacade<algorithm::MLD>
{
  public:
    ContiguousInternalMemoryDataFacade(std::shared_ptr<ContiguousBlockAllocator> allocator)
        : ContiguousInternalMemoryDataFacadeBase(allocator),
          ContiguousInternalMemoryAlgorithmDataFacade<algorithm::MLD>(allocator)

    {
    }
};
}
}
}

#endif // CONTIGUOUS_INTERNALMEM_DATAFACADE_HPP

#ifndef CONTIGUOUS_INTERNALMEM_DATAFACADE_HPP
#define CONTIGUOUS_INTERNALMEM_DATAFACADE_HPP

#include "engine/datafacade/algorithm_datafacade.hpp"
#include "engine/datafacade/contiguous_block_allocator.hpp"
#include "engine/datafacade/datafacade_base.hpp"

#include "engine/algorithm.hpp"
#include "engine/approach.hpp"
#include "engine/geospatial_query.hpp"

#include "customizer/edge_based_graph.hpp"

#include "extractor/datasources.hpp"
#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/guidance/turn_lane_types.hpp"
#include "extractor/intersection_bearings_container.hpp"
#include "extractor/node_data_container.hpp"
#include "extractor/packed_osm_ids.hpp"
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
#include "util/filtered_graph.hpp"
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
    using QueryGraph = util::FilteredGraphView<contractor::QueryGraphView>;
    using GraphNode = QueryGraph::NodeArrayEntry;
    using GraphEdge = QueryGraph::EdgeArrayEntry;

    QueryGraph m_query_graph;

    // allocator that keeps the allocation data
    std::shared_ptr<ContiguousBlockAllocator> allocator;

    void InitializeGraphPointer(storage::DataLayout &data_layout,
                                char *memory_block,
                                const std::size_t exclude_index)
    {
        auto graph_nodes_ptr = data_layout.GetBlockPtr<GraphNode>(
            memory_block, storage::DataLayout::CH_GRAPH_NODE_LIST);

        auto graph_edges_ptr = data_layout.GetBlockPtr<GraphEdge>(
            memory_block, storage::DataLayout::CH_GRAPH_EDGE_LIST);

        auto filter_block_id = static_cast<storage::DataLayout::BlockID>(
            storage::DataLayout::CH_EDGE_FILTER_0 + exclude_index);

        auto edge_filter_ptr = data_layout.GetBlockPtr<unsigned>(memory_block, filter_block_id);

        util::vector_view<GraphNode> node_list(
            graph_nodes_ptr, data_layout.num_entries[storage::DataLayout::CH_GRAPH_NODE_LIST]);
        util::vector_view<GraphEdge> edge_list(
            graph_edges_ptr, data_layout.num_entries[storage::DataLayout::CH_GRAPH_EDGE_LIST]);

        util::vector_view<bool> edge_filter(edge_filter_ptr,
                                            data_layout.num_entries[filter_block_id]);
        m_query_graph = QueryGraph({node_list, edge_list}, edge_filter);
    }

  public:
    ContiguousInternalMemoryAlgorithmDataFacade(
        std::shared_ptr<ContiguousBlockAllocator> allocator_, std::size_t exclude_index)
        : allocator(std::move(allocator_))
    {
        InitializeInternalPointers(allocator->GetLayout(), allocator->GetMemory(), exclude_index);
    }

    void InitializeInternalPointers(storage::DataLayout &data_layout,
                                    char *memory_block,
                                    const std::size_t exclude_index)
    {
        InitializeGraphPointer(data_layout, memory_block, exclude_index);
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

    void InitializeCoreInformationPointer(storage::DataLayout &data_layout,
                                          char *memory_block,
                                          const std::size_t exclude_index)
    {
        auto core_block_id = static_cast<storage::DataLayout::BlockID>(
            storage::DataLayout::CH_CORE_MARKER_0 + exclude_index);
        auto core_marker_ptr = data_layout.GetBlockPtr<unsigned>(memory_block, core_block_id);
        util::vector_view<bool> is_core_node(core_marker_ptr,
                                             data_layout.num_entries[core_block_id]);
        m_is_core_node = std::move(is_core_node);
    }

  public:
    ContiguousInternalMemoryAlgorithmDataFacade(
        std::shared_ptr<ContiguousBlockAllocator> allocator_, const std::size_t exclude_index)
        : allocator(std::move(allocator_))
    {
        InitializeInternalPointers(allocator->GetLayout(), allocator->GetMemory(), exclude_index);
    }

    void InitializeInternalPointers(storage::DataLayout &data_layout,
                                    char *memory_block,
                                    const std::size_t exclude_index)
    {
        InitializeCoreInformationPointer(data_layout, memory_block, exclude_index);
    }

    bool IsCoreNode(const NodeID id) const override final
    {
        BOOST_ASSERT(m_is_core_node.empty() || id < m_is_core_node.size());
        return !m_is_core_node.empty() || m_is_core_node[id];
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

    extractor::ClassData exclude_mask;
    std::string m_timestamp;
    extractor::ProfileProperties *m_profile_properties;
    extractor::Datasources *m_datasources;

    unsigned m_check_sum;
    util::vector_view<util::Coordinate> m_coordinate_list;
    extractor::PackedOSMIDsView m_osmnodeid_list;
    util::vector_view<std::uint32_t> m_lane_description_offsets;
    util::vector_view<extractor::guidance::TurnLaneType::Mask> m_lane_description_masks;
    util::vector_view<TurnPenalty> m_turn_weight_penalties;
    util::vector_view<TurnPenalty> m_turn_duration_penalties;
    extractor::SegmentDataView segment_data;
    extractor::TurnDataView turn_data;
    extractor::EdgeBasedNodeDataView edge_based_node_data;

    util::vector_view<char> m_datasource_name_data;
    util::vector_view<std::size_t> m_datasource_name_offsets;
    util::vector_view<std::size_t> m_datasource_name_lengths;
    util::vector_view<util::guidance::LaneTupleIdPair> m_lane_tupel_id_pairs;

    std::unique_ptr<SharedRTree> m_static_rtree;
    std::unique_ptr<SharedGeospatialQuery> m_geospatial_query;
    boost::filesystem::path file_index_path;

    extractor::IntersectionBearingsView intersection_bearings_view;

    util::NameTable m_name_table;
    // the look-up table for entry classes. An entry class lists the possibility of entry for all
    // available turns. Such a class id is stored with every edge.
    util::vector_view<util::guidance::EntryClass> m_entry_class_table;

    // allocator that keeps the allocation data
    std::shared_ptr<ContiguousBlockAllocator> allocator;

    void InitializeProfilePropertiesPointer(storage::DataLayout &data_layout,
                                            char *memory_block,
                                            const std::size_t exclude_index)
    {
        m_profile_properties = data_layout.GetBlockPtr<extractor::ProfileProperties>(
            memory_block, storage::DataLayout::PROPERTIES);

        exclude_mask = m_profile_properties->excludable_classes[exclude_index];
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

        auto tree_nodes_ptr =
            data_layout.GetBlockPtr<RTreeNode>(memory_block, storage::DataLayout::R_SEARCH_TREE);
        auto tree_level_sizes_ptr = data_layout.GetBlockPtr<std::uint64_t>(
            memory_block, storage::DataLayout::R_SEARCH_TREE_LEVELS);
        m_static_rtree.reset(
            new SharedRTree(tree_nodes_ptr,
                            data_layout.num_entries[storage::DataLayout::R_SEARCH_TREE],
                            tree_level_sizes_ptr,
                            data_layout.num_entries[storage::DataLayout::R_SEARCH_TREE_LEVELS],
                            file_index_path,
                            m_coordinate_list));
        m_geospatial_query.reset(
            new SharedGeospatialQuery(*m_static_rtree, m_coordinate_list, *this));
    }

    void InitializeNodeInformationPointers(storage::DataLayout &layout, char *memory_ptr)
    {
        const auto coordinate_list_ptr =
            layout.GetBlockPtr<util::Coordinate>(memory_ptr, storage::DataLayout::COORDINATE_LIST);
        m_coordinate_list.reset(coordinate_list_ptr,
                                layout.num_entries[storage::DataLayout::COORDINATE_LIST]);

        const auto osmnodeid_ptr = layout.GetBlockPtr<extractor::PackedOSMIDsView::block_type>(
            memory_ptr, storage::DataLayout::OSM_NODE_ID_LIST);
        m_osmnodeid_list = extractor::PackedOSMIDsView(
            util::vector_view<extractor::PackedOSMIDsView::block_type>(
                osmnodeid_ptr, layout.num_entries[storage::DataLayout::OSM_NODE_ID_LIST]),
            // We (ab)use the number of coordinates here because we know we have the same amount of
            // ids
            layout.num_entries[storage::DataLayout::COORDINATE_LIST]);
    }

    void InitializeEdgeBasedNodeDataInformationPointers(storage::DataLayout &layout,
                                                        char *memory_ptr)
    {
        const auto via_geometry_list_ptr =
            layout.GetBlockPtr<GeometryID>(memory_ptr, storage::DataLayout::GEOMETRY_ID_LIST);
        util::vector_view<GeometryID> geometry_ids(
            via_geometry_list_ptr, layout.num_entries[storage::DataLayout::GEOMETRY_ID_LIST]);

        const auto name_id_list_ptr =
            layout.GetBlockPtr<NameID>(memory_ptr, storage::DataLayout::NAME_ID_LIST);
        util::vector_view<NameID> name_ids(name_id_list_ptr,
                                           layout.num_entries[storage::DataLayout::NAME_ID_LIST]);

        const auto component_id_list_ptr =
            layout.GetBlockPtr<ComponentID>(memory_ptr, storage::DataLayout::COMPONENT_ID_LIST);
        util::vector_view<ComponentID> component_ids(
            component_id_list_ptr, layout.num_entries[storage::DataLayout::COMPONENT_ID_LIST]);

        const auto travel_mode_list_ptr = layout.GetBlockPtr<extractor::TravelMode>(
            memory_ptr, storage::DataLayout::TRAVEL_MODE_LIST);
        util::vector_view<extractor::TravelMode> travel_modes(
            travel_mode_list_ptr, layout.num_entries[storage::DataLayout::TRAVEL_MODE_LIST]);

        const auto classes_list_ptr =
            layout.GetBlockPtr<extractor::ClassData>(memory_ptr, storage::DataLayout::CLASSES_LIST);
        util::vector_view<extractor::ClassData> classes(
            classes_list_ptr, layout.num_entries[storage::DataLayout::CLASSES_LIST]);

        auto is_left_hand_driving_ptr = layout.GetBlockPtr<unsigned>(
            memory_ptr, storage::DataLayout::IS_LEFT_HAND_DRIVING_LIST);
        util::vector_view<bool> is_left_hand_driving(
            is_left_hand_driving_ptr,
            layout.num_entries[storage::DataLayout::IS_LEFT_HAND_DRIVING_LIST]);

        edge_based_node_data = extractor::EdgeBasedNodeDataView(std::move(geometry_ids),
                                                                std::move(name_ids),
                                                                std::move(component_ids),
                                                                std::move(travel_modes),
                                                                std::move(classes),
                                                                std::move(is_left_hand_driving));
    }

    void InitializeEdgeInformationPointers(storage::DataLayout &layout, char *memory_ptr)
    {
        const auto lane_data_id_ptr =
            layout.GetBlockPtr<LaneDataID>(memory_ptr, storage::DataLayout::LANE_DATA_ID);
        util::vector_view<LaneDataID> lane_data_ids(
            lane_data_id_ptr, layout.num_entries[storage::DataLayout::LANE_DATA_ID]);

        const auto turn_instruction_list_ptr =
            layout.GetBlockPtr<extractor::guidance::TurnInstruction>(
                memory_ptr, storage::DataLayout::TURN_INSTRUCTION);
        util::vector_view<extractor::guidance::TurnInstruction> turn_instructions(
            turn_instruction_list_ptr, layout.num_entries[storage::DataLayout::TURN_INSTRUCTION]);

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

        turn_data = extractor::TurnDataView(std::move(turn_instructions),
                                            std::move(lane_data_ids),
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

        auto num_entries = data_layout.num_entries[storage::DataLayout::GEOMETRIES_NODE_LIST];

        auto geometries_node_list_ptr = data_layout.GetBlockPtr<NodeID>(
            memory_block, storage::DataLayout::GEOMETRIES_NODE_LIST);
        util::vector_view<NodeID> geometry_node_list(geometries_node_list_ptr, num_entries);

        auto geometries_fwd_weight_list_ptr =
            data_layout.GetBlockPtr<extractor::SegmentDataView::SegmentWeightVector::block_type>(
                memory_block, storage::DataLayout::GEOMETRIES_FWD_WEIGHT_LIST);
        extractor::SegmentDataView::SegmentWeightVector geometry_fwd_weight_list(
            util::vector_view<extractor::SegmentDataView::SegmentWeightVector::block_type>(
                geometries_fwd_weight_list_ptr,
                data_layout.num_entries[storage::DataLayout::GEOMETRIES_FWD_WEIGHT_LIST]),
            num_entries);

        auto geometries_rev_weight_list_ptr =
            data_layout.GetBlockPtr<extractor::SegmentDataView::SegmentWeightVector::block_type>(
                memory_block, storage::DataLayout::GEOMETRIES_REV_WEIGHT_LIST);
        extractor::SegmentDataView::SegmentWeightVector geometry_rev_weight_list(
            util::vector_view<extractor::SegmentDataView::SegmentWeightVector::block_type>(
                geometries_rev_weight_list_ptr,
                data_layout.num_entries[storage::DataLayout::GEOMETRIES_REV_WEIGHT_LIST]),
            num_entries);

        auto geometries_fwd_duration_list_ptr =
            data_layout.GetBlockPtr<extractor::SegmentDataView::SegmentDurationVector::block_type>(
                memory_block, storage::DataLayout::GEOMETRIES_FWD_DURATION_LIST);
        extractor::SegmentDataView::SegmentDurationVector geometry_fwd_duration_list(
            util::vector_view<extractor::SegmentDataView::SegmentDurationVector::block_type>(
                geometries_fwd_duration_list_ptr,
                data_layout.num_entries[storage::DataLayout::GEOMETRIES_FWD_DURATION_LIST]),
            num_entries);

        auto geometries_rev_duration_list_ptr =
            data_layout.GetBlockPtr<extractor::SegmentDataView::SegmentDurationVector::block_type>(
                memory_block, storage::DataLayout::GEOMETRIES_REV_DURATION_LIST);
        extractor::SegmentDataView::SegmentDurationVector geometry_rev_duration_list(
            util::vector_view<extractor::SegmentDataView::SegmentDurationVector::block_type>(
                geometries_rev_duration_list_ptr,
                data_layout.num_entries[storage::DataLayout::GEOMETRIES_REV_DURATION_LIST]),
            num_entries);

        auto geometries_fwd_datasources_list_ptr = data_layout.GetBlockPtr<DatasourceID>(
            memory_block, storage::DataLayout::GEOMETRIES_FWD_DATASOURCES_LIST);
        util::vector_view<DatasourceID> geometry_fwd_datasources_list(
            geometries_fwd_datasources_list_ptr,
            data_layout.num_entries[storage::DataLayout::GEOMETRIES_FWD_DATASOURCES_LIST]);

        auto geometries_rev_datasources_list_ptr = data_layout.GetBlockPtr<DatasourceID>(
            memory_block, storage::DataLayout::GEOMETRIES_REV_DATASOURCES_LIST);
        util::vector_view<DatasourceID> geometry_rev_datasources_list(
            geometries_rev_datasources_list_ptr,
            data_layout.num_entries[storage::DataLayout::GEOMETRIES_REV_DATASOURCES_LIST]);

        segment_data = extractor::SegmentDataView{std::move(geometry_begin_indices),
                                                  std::move(geometry_node_list),
                                                  std::move(geometry_fwd_weight_list),
                                                  std::move(geometry_rev_weight_list),
                                                  std::move(geometry_fwd_duration_list),
                                                  std::move(geometry_rev_duration_list),
                                                  std::move(geometry_fwd_datasources_list),
                                                  std::move(geometry_rev_datasources_list)};

        m_datasources = data_layout.GetBlockPtr<extractor::Datasources>(
            memory_block, storage::DataLayout::DATASOURCES_NAMES);
    }

    void InitializeIntersectionClassPointers(storage::DataLayout &data_layout, char *memory_block)
    {
        auto bearing_class_id_ptr = data_layout.GetBlockPtr<BearingClassID>(
            memory_block, storage::DataLayout::BEARING_CLASSID);
        util::vector_view<BearingClassID> bearing_class_id(
            bearing_class_id_ptr, data_layout.num_entries[storage::DataLayout::BEARING_CLASSID]);

        auto bearing_values_ptr = data_layout.GetBlockPtr<DiscreteBearing>(
            memory_block, storage::DataLayout::BEARING_VALUES);
        util::vector_view<DiscreteBearing> bearing_values(
            bearing_values_ptr, data_layout.num_entries[storage::DataLayout::BEARING_VALUES]);

        auto offsets_ptr =
            data_layout.GetBlockPtr<unsigned>(memory_block, storage::DataLayout::BEARING_OFFSETS);
        auto blocks_ptr =
            data_layout.GetBlockPtr<IndexBlock>(memory_block, storage::DataLayout::BEARING_BLOCKS);
        util::vector_view<unsigned> bearing_offsets(
            offsets_ptr, data_layout.num_entries[storage::DataLayout::BEARING_OFFSETS]);
        util::vector_view<IndexBlock> bearing_blocks(
            blocks_ptr, data_layout.num_entries[storage::DataLayout::BEARING_BLOCKS]);

        util::RangeTable<16, storage::Ownership::View> bearing_range_table(
            bearing_offsets, bearing_blocks, static_cast<unsigned>(bearing_values.size()));

        intersection_bearings_view = extractor::IntersectionBearingsView{
            std::move(bearing_values), std::move(bearing_class_id), std::move(bearing_range_table)};

        auto entry_class_ptr = data_layout.GetBlockPtr<util::guidance::EntryClass>(
            memory_block, storage::DataLayout::ENTRY_CLASS);
        util::vector_view<util::guidance::EntryClass> entry_class_table(
            entry_class_ptr, data_layout.num_entries[storage::DataLayout::ENTRY_CLASS]);
        m_entry_class_table = std::move(entry_class_table);
    }

    void InitializeInternalPointers(storage::DataLayout &data_layout,
                                    char *memory_block,
                                    const std::size_t exclude_index)
    {
        InitializeChecksumPointer(data_layout, memory_block);
        InitializeNodeInformationPointers(data_layout, memory_block);
        InitializeEdgeBasedNodeDataInformationPointers(data_layout, memory_block);
        InitializeEdgeInformationPointers(data_layout, memory_block);
        InitializeTurnPenalties(data_layout, memory_block);
        InitializeGeometryPointers(data_layout, memory_block);
        InitializeTimestampPointer(data_layout, memory_block);
        InitializeNamePointers(data_layout, memory_block);
        InitializeTurnLaneDescriptionsPointers(data_layout, memory_block);
        InitializeProfilePropertiesPointer(data_layout, memory_block, exclude_index);
        InitializeRTreePointers(data_layout, memory_block);
        InitializeIntersectionClassPointers(data_layout, memory_block);
    }

  public:
    // allows switching between process_memory/shared_memory datafacade, based on the type of
    // allocator
    ContiguousInternalMemoryDataFacadeBase(std::shared_ptr<ContiguousBlockAllocator> allocator_,
                                           const std::size_t exclude_index)
        : allocator(std::move(allocator_))
    {
        InitializeInternalPointers(allocator->GetLayout(), allocator->GetMemory(), exclude_index);
    }

    // node and edge information access
    util::Coordinate GetCoordinateOfNode(const NodeID id) const override final
    {
        return m_coordinate_list[id];
    }

    OSMNodeID GetOSMNodeIDOfNode(const NodeID id) const override final
    {
        return m_osmnodeid_list[id];
    }

    std::vector<NodeID> GetUncompressedForwardGeometry(const EdgeID id) const override final
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
                               const float max_distance,
                               const Approach approach) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodesInRange(
            input_coordinate, max_distance, approach);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate input_coordinate,
                               const float max_distance,
                               const int bearing,
                               const int bearing_range,
                               const Approach approach) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodesInRange(
            input_coordinate, max_distance, bearing, bearing_range, approach);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const Approach approach) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodes(input_coordinate, max_results, approach);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const double max_distance,
                        const Approach approach) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodes(
            input_coordinate, max_results, max_distance, approach);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const int bearing,
                        const int bearing_range,
                        const Approach approach) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodes(
            input_coordinate, max_results, bearing, bearing_range, approach);
    }

    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const double max_distance,
                        const int bearing,
                        const int bearing_range,
                        const Approach approach) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodes(
            input_coordinate, max_results, max_distance, bearing, bearing_range, approach);
    }

    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate input_coordinate,
                                                      const Approach approach) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodeWithAlternativeFromBigComponent(
            input_coordinate, approach);
    }

    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate input_coordinate,
                                                      const double max_distance,
                                                      const Approach approach) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodeWithAlternativeFromBigComponent(
            input_coordinate, max_distance, approach);
    }

    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate input_coordinate,
                                                      const double max_distance,
                                                      const int bearing,
                                                      const int bearing_range,
                                                      const Approach approach) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodeWithAlternativeFromBigComponent(
            input_coordinate, max_distance, bearing, bearing_range, approach);
    }

    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate input_coordinate,
                                                      const int bearing,
                                                      const int bearing_range,
                                                      const Approach approach) const override final
    {
        BOOST_ASSERT(m_geospatial_query.get());

        return m_geospatial_query->NearestPhantomNodeWithAlternativeFromBigComponent(
            input_coordinate, bearing, bearing_range, approach);
    }

    unsigned GetCheckSum() const override final { return m_check_sum; }

    GeometryID GetGeometryIndex(const NodeID id) const override final
    {
        return edge_based_node_data.GetGeometryID(id);
    }

    ComponentID GetComponentID(const NodeID id) const override final
    {
        return edge_based_node_data.GetComponentID(id);
    }

    extractor::TravelMode GetTravelMode(const NodeID id) const override final
    {
        return edge_based_node_data.GetTravelMode(id);
    }

    extractor::ClassData GetClassData(const NodeID id) const override final
    {
        return edge_based_node_data.GetClassData(id);
    }

    bool ExcludeNode(const NodeID id) const override final
    {
        return (edge_based_node_data.GetClassData(id) & exclude_mask) > 0;
    }

    std::vector<std::string> GetClasses(const extractor::ClassData class_data) const override final
    {
        auto indexes = extractor::getClassIndexes(class_data);
        std::vector<std::string> classes(indexes.size());
        std::transform(indexes.begin(), indexes.end(), classes.begin(), [this](const auto index) {
            return m_profile_properties->GetClassName(index);
        });

        return classes;
    }

    NameID GetNameIndex(const NodeID id) const override final
    {
        return edge_based_node_data.GetNameID(id);
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

    StringView GetExitsForID(const NameID id) const override final
    {
        return m_name_table.GetExitsForID(id);
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

    util::guidance::BearingClass GetBearingClass(const NodeID node) const override final
    {
        return intersection_bearings_view.GetBearingClass(node);
    }

    util::guidance::TurnBearing PreTurnBearing(const EdgeID eid) const override final
    {
        return turn_data.GetPreTurnBearing(eid);
    }
    util::guidance::TurnBearing PostTurnBearing(const EdgeID eid) const override final
    {
        return turn_data.GetPostTurnBearing(eid);
    }

    util::guidance::EntryClass GetEntryClass(const EdgeID turn_id) const override final
    {
        auto entry_class_id = turn_data.GetEntryClassID(turn_id);
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

    bool IsLeftHandDriving(const NodeID id) const override final
    {
        // TODO: can be moved to a data block indexed by GeometryID
        return edge_based_node_data.IsLeftHandDriving(id);
    }
};

template <typename AlgorithmT> class ContiguousInternalMemoryDataFacade;

template <>
class ContiguousInternalMemoryDataFacade<CH>
    : public ContiguousInternalMemoryDataFacadeBase,
      public ContiguousInternalMemoryAlgorithmDataFacade<CH>
{
  public:
    ContiguousInternalMemoryDataFacade(std::shared_ptr<ContiguousBlockAllocator> allocator,
                                       const std::size_t exclude_index)
        : ContiguousInternalMemoryDataFacadeBase(allocator, exclude_index),
          ContiguousInternalMemoryAlgorithmDataFacade<CH>(allocator, exclude_index)

    {
    }
};

template <>
class ContiguousInternalMemoryDataFacade<CoreCH> final
    : public ContiguousInternalMemoryDataFacade<CH>,
      public ContiguousInternalMemoryAlgorithmDataFacade<CoreCH>
{
  public:
    ContiguousInternalMemoryDataFacade(std::shared_ptr<ContiguousBlockAllocator> allocator,
                                       const std::size_t exclude_index)
        : ContiguousInternalMemoryDataFacade<CH>(allocator, exclude_index),
          ContiguousInternalMemoryAlgorithmDataFacade<CoreCH>(allocator, exclude_index)

    {
    }
};

template <> class ContiguousInternalMemoryAlgorithmDataFacade<MLD> : public AlgorithmDataFacade<MLD>
{
    // MLD data
    partition::MultiLevelPartitionView mld_partition;
    partition::CellStorageView mld_cell_storage;
    customizer::CellMetricView mld_cell_metric;
    using QueryGraph = customizer::MultiLevelEdgeBasedGraphView;
    using GraphNode = QueryGraph::NodeArrayEntry;
    using GraphEdge = QueryGraph::EdgeArrayEntry;

    QueryGraph query_graph;

    void InitializeInternalPointers(storage::DataLayout &data_layout,
                                    char *memory_block,
                                    const std::size_t exclude_index)
    {
        InitializeMLDDataPointers(data_layout, memory_block, exclude_index);
        InitializeGraphPointer(data_layout, memory_block);
    }

    void InitializeMLDDataPointers(storage::DataLayout &data_layout,
                                   char *memory_block,
                                   const std::size_t exclude_index)
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

        const auto weights_block_id = static_cast<storage::DataLayout::BlockID>(
            storage::DataLayout::MLD_CELL_WEIGHTS_0 + exclude_index);
        const auto durations_block_id = static_cast<storage::DataLayout::BlockID>(
            storage::DataLayout::MLD_CELL_DURATIONS_0 + exclude_index);

        if (data_layout.GetBlockSize(weights_block_id) > 0)
        {
            auto mld_cell_weights_ptr =
                data_layout.GetBlockPtr<EdgeWeight>(memory_block, weights_block_id);
            auto mld_cell_durations_ptr =
                data_layout.GetBlockPtr<EdgeDuration>(memory_block, durations_block_id);
            auto weight_entries_count =
                data_layout.GetBlockEntries(storage::DataLayout::MLD_CELL_WEIGHTS_0);
            auto duration_entries_count =
                data_layout.GetBlockEntries(storage::DataLayout::MLD_CELL_DURATIONS_0);
            BOOST_ASSERT(weight_entries_count == duration_entries_count);
            util::vector_view<EdgeWeight> weights(mld_cell_weights_ptr, weight_entries_count);
            util::vector_view<EdgeDuration> durations(mld_cell_durations_ptr,
                                                      duration_entries_count);

            mld_cell_metric = customizer::CellMetricView{std::move(weights), std::move(durations)};
        }

        if (data_layout.GetBlockSize(storage::DataLayout::MLD_CELLS) > 0)
        {

            auto mld_source_boundary_ptr = data_layout.GetBlockPtr<NodeID>(
                memory_block, storage::DataLayout::MLD_CELL_SOURCE_BOUNDARY);
            auto mld_destination_boundary_ptr = data_layout.GetBlockPtr<NodeID>(
                memory_block, storage::DataLayout::MLD_CELL_DESTINATION_BOUNDARY);
            auto mld_cells_ptr = data_layout.GetBlockPtr<partition::CellStorageView::CellData>(
                memory_block, storage::DataLayout::MLD_CELLS);
            auto mld_cell_level_offsets_ptr = data_layout.GetBlockPtr<std::uint64_t>(
                memory_block, storage::DataLayout::MLD_CELL_LEVEL_OFFSETS);

            auto source_boundary_entries_count =
                data_layout.GetBlockEntries(storage::DataLayout::MLD_CELL_SOURCE_BOUNDARY);
            auto destination_boundary_entries_count =
                data_layout.GetBlockEntries(storage::DataLayout::MLD_CELL_DESTINATION_BOUNDARY);
            auto cells_entries_counts = data_layout.GetBlockEntries(storage::DataLayout::MLD_CELLS);
            auto cell_level_offsets_entries_count =
                data_layout.GetBlockEntries(storage::DataLayout::MLD_CELL_LEVEL_OFFSETS);

            util::vector_view<NodeID> source_boundary(mld_source_boundary_ptr,
                                                      source_boundary_entries_count);
            util::vector_view<NodeID> destination_boundary(mld_destination_boundary_ptr,
                                                           destination_boundary_entries_count);
            util::vector_view<partition::CellStorageView::CellData> cells(mld_cells_ptr,
                                                                          cells_entries_counts);
            util::vector_view<std::uint64_t> level_offsets(mld_cell_level_offsets_ptr,
                                                           cell_level_offsets_entries_count);

            mld_cell_storage = partition::CellStorageView{std::move(source_boundary),
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
        std::shared_ptr<ContiguousBlockAllocator> allocator_, const std::size_t exclude_index)
        : allocator(std::move(allocator_))
    {
        InitializeInternalPointers(allocator->GetLayout(), allocator->GetMemory(), exclude_index);
    }

    const partition::MultiLevelPartitionView &GetMultiLevelPartition() const override
    {
        return mld_partition;
    }

    const partition::CellStorageView &GetCellStorage() const override { return mld_cell_storage; }

    const customizer::CellMetricView &GetCellMetric() const override { return mld_cell_metric; }

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
    ContiguousInternalMemoryDataFacade(std::shared_ptr<ContiguousBlockAllocator> allocator,
                                       const std::size_t exclude_index)
        : ContiguousInternalMemoryDataFacadeBase(allocator, exclude_index),
          ContiguousInternalMemoryAlgorithmDataFacade<MLD>(allocator, exclude_index)

    {
    }
};
}
}
}

#endif // CONTIGUOUS_INTERNALMEM_DATAFACADE_HPP

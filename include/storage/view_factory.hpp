#ifndef OSRM_STOARGE_VIEW_FACTORY_HPP
#define OSRM_STOARGE_VIEW_FACTORY_HPP

#include "storage/shared_datatype.hpp"

#include "contractor/contracted_metric.hpp"
#include "contractor/query_graph.hpp"

#include "customizer/edge_based_graph.hpp"

#include "extractor/class_data.hpp"
#include "extractor/compressed_edge_container.hpp"
#include "extractor/edge_based_edge.hpp"
#include "extractor/edge_based_node.hpp"
#include "extractor/edge_based_node_segment.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/name_table.hpp"
#include "extractor/packed_osm_ids.hpp"
#include "extractor/query_node.hpp"
#include "extractor/travel_mode.hpp"

#include "guidance/turn_bearing.hpp"
#include "guidance/turn_data_container.hpp"
#include "guidance/turn_instruction.hpp"

#include "partitioner/cell_storage.hpp"
#include "partitioner/edge_based_graph_reader.hpp"
#include "partitioner/multi_level_partition.hpp"

#include "util/coordinate.hpp"
#include "util/packed_vector.hpp"
#include "util/range_table.hpp"
#include "util/static_graph.hpp"
#include "util/static_rtree.hpp"
#include "util/typedefs.hpp"
#include "util/vector_view.hpp"

#include "util/filtered_graph.hpp"
#include "util/packed_vector.hpp"
#include "util/vector_view.hpp"

namespace osrm
{
namespace storage
{

template <typename T>
util::vector_view<T>
make_vector_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    return util::vector_view<T>(layout.GetBlockPtr<T>(memory_ptr, name),
                                layout.GetBlockEntries(name));
}

template <>
inline util::vector_view<bool>
make_vector_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    return util::vector_view<bool>(
        layout.GetBlockPtr<util::vector_view<bool>::Word>(memory_ptr, name),
        layout.GetBlockEntries(name));
}

template <typename T, std::size_t Bits>
util::PackedVectorView<T, Bits>
make_packed_vector_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    using V = util::PackedVectorView<T, Bits>;
    auto packed_internal = util::vector_view<typename V::block_type>(
        layout.GetBlockPtr<typename V::block_type>(memory_ptr, name + "/packed"),
        layout.GetBlockEntries(name + "/packed"));
    // the real size needs to come from an external source
    return V{packed_internal, std::numeric_limits<std::size_t>::max()};
}

inline auto
make_name_table_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    auto blocks = make_vector_view<extractor::NameTableView::IndexedData::BlockReference>(
        memory_ptr, layout, name + "/blocks");
    auto values = make_vector_view<extractor::NameTableView::IndexedData::ValueType>(
        memory_ptr, layout, name + "/values");

    extractor::NameTableView::IndexedData index_data_view{std::move(blocks), std::move(values)};
    return extractor::NameTableView{index_data_view};
}

inline auto make_lane_data_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    return make_vector_view<util::guidance::LaneTupleIdPair>(memory_ptr, layout, name + "/data");
}

inline auto make_turn_lane_description_views(char *memory_ptr,
                                             const DataLayout &layout,
                                             const std::string &name)
{
    auto offsets = make_vector_view<std::uint32_t>(memory_ptr, layout, name + "/offsets");
    auto masks =
        make_vector_view<extractor::TurnLaneType::Mask>(memory_ptr, layout, name + "/masks");

    return std::make_tuple(offsets, masks);
}

inline auto make_ebn_data_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    auto edge_based_node_data =
        make_vector_view<extractor::EdgeBasedNode>(memory_ptr, layout, name + "/nodes");
    auto annotation_data = make_vector_view<extractor::NodeBasedEdgeAnnotation>(
        memory_ptr, layout, name + "/annotations");

    return extractor::EdgeBasedNodeDataView(std::move(edge_based_node_data),
                                            std::move(annotation_data));
}

inline auto make_turn_data_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    auto lane_data_ids = make_vector_view<LaneDataID>(memory_ptr, layout, name + "/lane_data_ids");

    const auto turn_instructions = make_vector_view<guidance::TurnInstruction>(
        memory_ptr, layout, name + "/turn_instructions");

    const auto entry_class_ids =
        make_vector_view<EntryClassID>(memory_ptr, layout, name + "/entry_class_ids");

    const auto pre_turn_bearings =
        make_vector_view<guidance::TurnBearing>(memory_ptr, layout, name + "/pre_turn_bearings");

    const auto post_turn_bearings =
        make_vector_view<guidance::TurnBearing>(memory_ptr, layout, name + "/post_turn_bearings");

    return guidance::TurnDataView(std::move(turn_instructions),
                                  std::move(lane_data_ids),
                                  std::move(entry_class_ids),
                                  std::move(pre_turn_bearings),
                                  std::move(post_turn_bearings));
}

inline auto
make_segment_data_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    auto geometry_begin_indices = make_vector_view<unsigned>(memory_ptr, layout, name + "/index");

    auto node_list = make_vector_view<NodeID>(memory_ptr, layout, name + "/nodes");

    auto num_entries = layout.GetBlockEntries(name + "/nodes");

    extractor::SegmentDataView::SegmentWeightVector fwd_weight_list(
        make_vector_view<extractor::SegmentDataView::SegmentWeightVector::block_type>(
            memory_ptr, layout, name + "/forward_weights/packed"),
        num_entries);

    extractor::SegmentDataView::SegmentWeightVector rev_weight_list(
        make_vector_view<extractor::SegmentDataView::SegmentWeightVector::block_type>(
            memory_ptr, layout, name + "/reverse_weights/packed"),
        num_entries);

    extractor::SegmentDataView::SegmentDurationVector fwd_duration_list(
        make_vector_view<extractor::SegmentDataView::SegmentDurationVector::block_type>(
            memory_ptr, layout, name + "/forward_durations/packed"),
        num_entries);

    extractor::SegmentDataView::SegmentDurationVector rev_duration_list(
        make_vector_view<extractor::SegmentDataView::SegmentDurationVector::block_type>(
            memory_ptr, layout, name + "/reverse_durations/packed"),
        num_entries);

    auto fwd_datasources_list =
        make_vector_view<DatasourceID>(memory_ptr, layout, name + "/forward_data_sources");

    auto rev_datasources_list =
        make_vector_view<DatasourceID>(memory_ptr, layout, name + "/reverse_data_sources");

    return extractor::SegmentDataView{std::move(geometry_begin_indices),
                                      std::move(node_list),
                                      std::move(fwd_weight_list),
                                      std::move(rev_weight_list),
                                      std::move(fwd_duration_list),
                                      std::move(rev_duration_list),
                                      std::move(fwd_datasources_list),
                                      std::move(rev_datasources_list)};
}

inline auto
make_coordinates_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    return make_vector_view<util::Coordinate>(memory_ptr, layout, name);
}

inline auto make_osm_ids_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    return make_packed_vector_view<extractor::PackedOSMIDsView::value_type,
                                   extractor::PackedOSMIDsView::value_size>(
        memory_ptr, layout, name);
}

inline auto make_nbn_data_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    return std::make_tuple(make_coordinates_view(memory_ptr, layout, name + "/coordinates"),
                           make_osm_ids_view(memory_ptr, layout, name + "/osm_node_ids"));
}

inline auto
make_turn_weight_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    return make_vector_view<TurnPenalty>(memory_ptr, layout, name + "/weight");
}

inline auto
make_turn_duration_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    return make_vector_view<TurnPenalty>(memory_ptr, layout, name + "/duration");
}

inline auto
make_search_tree_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    using RTreeLeaf = extractor::EdgeBasedNodeSegment;
    using RTreeNode = util::StaticRTree<RTreeLeaf, storage::Ownership::View>::TreeNode;

    const auto search_tree = make_vector_view<RTreeNode>(memory_ptr, layout, name + "/search_tree");

    const auto rtree_level_starts =
        make_vector_view<std::uint64_t>(memory_ptr, layout, name + "/search_tree_level_starts");

    const auto coordinates =
        make_coordinates_view(memory_ptr, layout, "/common/nbn_data/coordinates");

    const char *path = layout.GetBlockPtr<char>(memory_ptr, name + "/file_index_path");

    if (!boost::filesystem::exists(boost::filesystem::path{path}))
    {
        throw util::exception("Could not load " + std::string(path) + "Does the leaf file exist?" +
                              SOURCE_REF);
    }

    return util::StaticRTree<RTreeLeaf, storage::Ownership::View>{
        std::move(search_tree), std::move(rtree_level_starts), path, std::move(coordinates)};
}

inline auto
make_intersection_bearings_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    auto bearing_offsets =
        make_vector_view<unsigned>(memory_ptr, layout, name + "/class_id_to_ranges/block_offsets");
    auto bearing_blocks = make_vector_view<util::RangeTable<16, storage::Ownership::View>::BlockT>(
        memory_ptr, layout, name + "/class_id_to_ranges/diff_blocks");
    auto bearing_values =
        make_vector_view<DiscreteBearing>(memory_ptr, layout, name + "/bearing_values");
    util::RangeTable<16, storage::Ownership::View> bearing_range_table(
        std::move(bearing_offsets),
        std::move(bearing_blocks),
        static_cast<unsigned>(bearing_values.size()));

    auto bearing_class_id =
        make_vector_view<BearingClassID>(memory_ptr, layout, name + "/node_to_class_id");
    return extractor::IntersectionBearingsView{
        std::move(bearing_values), std::move(bearing_class_id), std::move(bearing_range_table)};
}

inline auto
make_entry_classes_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    return make_vector_view<util::guidance::EntryClass>(memory_ptr, layout, name);
}

inline auto
make_contracted_metric_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    auto node_list = make_vector_view<contractor::QueryGraphView::NodeArrayEntry>(
        memory_ptr, layout, name + "/contracted_graph/node_array");
    auto edge_list = make_vector_view<contractor::QueryGraphView::EdgeArrayEntry>(
        memory_ptr, layout, name + "/contracted_graph/edge_array");

    std::vector<util::vector_view<bool>> edge_filter;
    layout.List(name + "/exclude",
                boost::make_function_output_iterator([&](const auto &filter_name) {
                    edge_filter.push_back(make_vector_view<bool>(memory_ptr, layout, filter_name));
                }));

    return contractor::ContractedMetricView{{std::move(node_list), std::move(edge_list)},
                                            std::move(edge_filter)};
}

inline auto make_partition_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    auto level_data_ptr = layout.GetBlockPtr<partitioner::MultiLevelPartitionView::LevelData>(
        memory_ptr, name + "/level_data");
    auto partition = make_vector_view<PartitionID>(memory_ptr, layout, name + "/partition");
    auto cell_to_children =
        make_vector_view<CellID>(memory_ptr, layout, name + "/cell_to_children");

    return partitioner::MultiLevelPartitionView{
        level_data_ptr, std::move(partition), std::move(cell_to_children)};
}

inline auto
make_cell_storage_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    auto source_boundary = make_vector_view<NodeID>(memory_ptr, layout, name + "/source_boundary");
    auto destination_boundary =
        make_vector_view<NodeID>(memory_ptr, layout, name + "/destination_boundary");
    auto cells = make_vector_view<partitioner::CellStorageView::CellData>(
        memory_ptr, layout, name + "/cells");
    auto level_offsets =
        make_vector_view<std::uint64_t>(memory_ptr, layout, name + "/level_to_cell_offset");

    return partitioner::CellStorageView{std::move(source_boundary),
                                        std::move(destination_boundary),
                                        std::move(cells),
                                        std::move(level_offsets)};
}

inline auto make_filtered_cell_metric_view(char *memory_ptr,
                                           const DataLayout &layout,
                                           const std::string &name,
                                           const std::size_t exclude_index)
{
    customizer::CellMetricView cell_metric;

    auto prefix = name + "/exclude/" + std::to_string(exclude_index);
    auto weights_block_id = prefix + "/weights";
    auto durations_block_id = prefix + "/durations";

    auto weights = make_vector_view<EdgeWeight>(memory_ptr, layout, weights_block_id);
    auto durations = make_vector_view<EdgeDuration>(memory_ptr, layout, durations_block_id);

    return customizer::CellMetricView{std::move(weights), std::move(durations)};
}

inline auto
make_cell_metric_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    std::vector<customizer::CellMetricView> cell_metric_excludes;

    std::vector<std::string> metric_prefix_names;
    layout.List(name + "/exclude/", std::back_inserter(metric_prefix_names));
    for (const auto &prefix : metric_prefix_names)
    {
        auto weights_block_id = prefix + "/weights";
        auto durations_block_id = prefix + "/durations";

        auto weights = make_vector_view<EdgeWeight>(memory_ptr, layout, weights_block_id);
        auto durations = make_vector_view<EdgeDuration>(memory_ptr, layout, durations_block_id);

        cell_metric_excludes.push_back(
            customizer::CellMetricView{std::move(weights), std::move(durations)});
    }

    return cell_metric_excludes;
}

inline auto
make_multi_level_graph_view(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    auto node_list = make_vector_view<customizer::MultiLevelEdgeBasedGraphView::NodeArrayEntry>(
        memory_ptr, layout, name + "/node_array");
    auto edge_list = make_vector_view<customizer::MultiLevelEdgeBasedGraphView::EdgeArrayEntry>(
        memory_ptr, layout, name + "/edge_array");
    auto node_to_offset = make_vector_view<customizer::MultiLevelEdgeBasedGraphView::EdgeOffset>(
        memory_ptr, layout, name + "/node_to_edge_offset");

    return customizer::MultiLevelEdgeBasedGraphView(
        std::move(node_list), std::move(edge_list), std::move(node_to_offset));
}

inline auto
make_maneuver_overrides_views(char *memory_ptr, const DataLayout &layout, const std::string &name)
{
    auto maneuver_overrides = make_vector_view<extractor::StorageManeuverOverride>(
        memory_ptr, layout, name + "/overrides");
    auto maneuver_override_node_sequences =
        make_vector_view<NodeID>(memory_ptr, layout, name + "/node_sequences");

    return std::make_tuple(maneuver_overrides, maneuver_override_node_sequences);
}

inline auto make_filtered_graph_view(char *memory_ptr,
                                     const DataLayout &layout,
                                     const std::string &name,
                                     const std::size_t exclude_index)
{
    auto exclude_prefix = name + "/exclude/" + std::to_string(exclude_index);
    auto edge_filter = make_vector_view<bool>(memory_ptr, layout, exclude_prefix + "/edge_filter");
    auto node_list = make_vector_view<contractor::QueryGraphView::NodeArrayEntry>(
        memory_ptr, layout, name + "/contracted_graph/node_array");
    auto edge_list = make_vector_view<contractor::QueryGraphView::EdgeArrayEntry>(
        memory_ptr, layout, name + "/contracted_graph/edge_array");

    return util::FilteredGraphView<contractor::QueryGraphView>({node_list, edge_list}, edge_filter);
}
}
}

#endif

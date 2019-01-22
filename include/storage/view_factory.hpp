#ifndef OSRM_STOARGE_VIEW_FACTORY_HPP
#define OSRM_STOARGE_VIEW_FACTORY_HPP

#include "storage/shared_data_index.hpp"

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
util::vector_view<T> make_vector_view(const SharedDataIndex &index, const std::string &name)
{
    return util::vector_view<T>(index.GetBlockPtr<T>(name), index.GetBlockEntries(name));
}

template <>
inline util::vector_view<bool> make_vector_view(const SharedDataIndex &index,
                                                const std::string &name)
{
    return util::vector_view<bool>(index.GetBlockPtr<util::vector_view<bool>::Word>(name),
                                   index.GetBlockEntries(name));
}

template <typename T, std::size_t Bits>
util::PackedVectorView<T, Bits> make_packed_vector_view(const SharedDataIndex &index,
                                                        const std::string &name)
{
    using V = util::PackedVectorView<T, Bits>;
    auto packed_internal = util::vector_view<typename V::block_type>(
        index.GetBlockPtr<typename V::block_type>(name + "/packed"),
        index.GetBlockEntries(name + "/packed"));
    // the real size needs to come from an external source
    return V{packed_internal, std::numeric_limits<std::size_t>::max()};
}

inline auto make_name_table_view(const SharedDataIndex &index, const std::string &name)
{
    auto blocks = make_vector_view<extractor::NameTableView::IndexedData::BlockReference>(
        index, name + "/blocks");
    auto values =
        make_vector_view<extractor::NameTableView::IndexedData::ValueType>(index, name + "/values");

    extractor::NameTableView::IndexedData index_data_view{std::move(blocks), std::move(values)};
    return extractor::NameTableView{index_data_view};
}

inline auto make_lane_data_view(const SharedDataIndex &index, const std::string &name)
{
    return make_vector_view<util::guidance::LaneTupleIdPair>(index, name + "/data");
}

inline auto make_turn_lane_description_views(const SharedDataIndex &index, const std::string &name)
{
    auto offsets = make_vector_view<std::uint32_t>(index, name + "/offsets");
    auto masks = make_vector_view<extractor::TurnLaneType::Mask>(index, name + "/masks");

    return std::make_tuple(offsets, masks);
}

inline auto make_ebn_data_view(const SharedDataIndex &index, const std::string &name)
{
    auto edge_based_node_data = make_vector_view<extractor::EdgeBasedNode>(index, name + "/nodes");
    auto annotation_data =
        make_vector_view<extractor::NodeBasedEdgeAnnotation>(index, name + "/annotations");

    return extractor::EdgeBasedNodeDataView(std::move(edge_based_node_data),
                                            std::move(annotation_data));
}

inline auto make_turn_data_view(const SharedDataIndex &index, const std::string &name)
{
    auto lane_data_ids = make_vector_view<LaneDataID>(index, name + "/lane_data_ids");

    const auto turn_instructions =
        make_vector_view<guidance::TurnInstruction>(index, name + "/turn_instructions");

    const auto entry_class_ids = make_vector_view<EntryClassID>(index, name + "/entry_class_ids");

    const auto pre_turn_bearings =
        make_vector_view<guidance::TurnBearing>(index, name + "/pre_turn_bearings");

    const auto post_turn_bearings =
        make_vector_view<guidance::TurnBearing>(index, name + "/post_turn_bearings");

    return guidance::TurnDataView(std::move(turn_instructions),
                                  std::move(lane_data_ids),
                                  std::move(entry_class_ids),
                                  std::move(pre_turn_bearings),
                                  std::move(post_turn_bearings));
}

inline auto make_segment_data_view(const SharedDataIndex &index, const std::string &name)
{
    auto geometry_begin_indices = make_vector_view<unsigned>(index, name + "/index");

    auto node_list = make_vector_view<NodeID>(index, name + "/nodes");

    auto num_entries = index.GetBlockEntries(name + "/nodes");

    extractor::SegmentDataView::SegmentWeightVector fwd_weight_list(
        make_vector_view<extractor::SegmentDataView::SegmentWeightVector::block_type>(
            index, name + "/forward_weights/packed"),
        num_entries);

    extractor::SegmentDataView::SegmentWeightVector rev_weight_list(
        make_vector_view<extractor::SegmentDataView::SegmentWeightVector::block_type>(
            index, name + "/reverse_weights/packed"),
        num_entries);

    extractor::SegmentDataView::SegmentDurationVector fwd_duration_list(
        make_vector_view<extractor::SegmentDataView::SegmentDurationVector::block_type>(
            index, name + "/forward_durations/packed"),
        num_entries);

    extractor::SegmentDataView::SegmentDurationVector rev_duration_list(
        make_vector_view<extractor::SegmentDataView::SegmentDurationVector::block_type>(
            index, name + "/reverse_durations/packed"),
        num_entries);

    auto fwd_datasources_list =
        make_vector_view<DatasourceID>(index, name + "/forward_data_sources");

    auto rev_datasources_list =
        make_vector_view<DatasourceID>(index, name + "/reverse_data_sources");

    return extractor::SegmentDataView{std::move(geometry_begin_indices),
                                      std::move(node_list),
                                      std::move(fwd_weight_list),
                                      std::move(rev_weight_list),
                                      std::move(fwd_duration_list),
                                      std::move(rev_duration_list),
                                      std::move(fwd_datasources_list),
                                      std::move(rev_datasources_list)};
}

inline auto make_coordinates_view(const SharedDataIndex &index, const std::string &name)
{
    return make_vector_view<util::Coordinate>(index, name);
}

inline auto make_osm_ids_view(const SharedDataIndex &index, const std::string &name)
{
    return make_packed_vector_view<extractor::PackedOSMIDsView::value_type,
                                   extractor::PackedOSMIDsView::value_size>(index, name);
}

inline auto make_nbn_data_view(const SharedDataIndex &index, const std::string &name)
{
    return std::make_tuple(make_coordinates_view(index, name + "/coordinates"),
                           make_osm_ids_view(index, name + "/osm_node_ids"));
}

inline auto make_turn_weight_view(const SharedDataIndex &index, const std::string &name)
{
    return make_vector_view<TurnPenalty>(index, name + "/weight");
}

inline auto make_turn_duration_view(const SharedDataIndex &index, const std::string &name)
{
    return make_vector_view<TurnPenalty>(index, name + "/duration");
}

inline auto make_search_tree_view(const SharedDataIndex &index, const std::string &name)
{
    using RTreeLeaf = extractor::EdgeBasedNodeSegment;
    using RTreeNode = util::StaticRTree<RTreeLeaf, storage::Ownership::View>::TreeNode;

    const auto search_tree = make_vector_view<RTreeNode>(index, name + "/search_tree");

    const auto rtree_level_starts =
        make_vector_view<std::uint64_t>(index, name + "/search_tree_level_starts");

    const auto coordinates = make_coordinates_view(index, "/common/nbn_data/coordinates");

    const char *path = index.template GetBlockPtr<char>(name + "/file_index_path");

    if (!boost::filesystem::exists(boost::filesystem::path{path}))
    {
        throw util::exception("Could not load " + std::string(path) + "Does the leaf file exist?" +
                              SOURCE_REF);
    }

    return util::StaticRTree<RTreeLeaf, storage::Ownership::View>{
        std::move(search_tree), std::move(rtree_level_starts), path, std::move(coordinates)};
}

inline auto make_intersection_bearings_view(const SharedDataIndex &index, const std::string &name)
{
    auto bearing_offsets =
        make_vector_view<unsigned>(index, name + "/class_id_to_ranges/block_offsets");
    auto bearing_blocks = make_vector_view<util::RangeTable<16, storage::Ownership::View>::BlockT>(
        index, name + "/class_id_to_ranges/diff_blocks");
    auto bearing_values = make_vector_view<DiscreteBearing>(index, name + "/bearing_values");
    util::RangeTable<16, storage::Ownership::View> bearing_range_table(
        std::move(bearing_offsets),
        std::move(bearing_blocks),
        static_cast<unsigned>(bearing_values.size()));

    auto bearing_class_id = make_vector_view<BearingClassID>(index, name + "/node_to_class_id");
    return extractor::IntersectionBearingsView{
        std::move(bearing_values), std::move(bearing_class_id), std::move(bearing_range_table)};
}

inline auto make_entry_classes_view(const SharedDataIndex &index, const std::string &name)
{
    return make_vector_view<util::guidance::EntryClass>(index, name);
}

inline auto make_contracted_metric_view(const SharedDataIndex &index, const std::string &name)
{
    auto node_list = make_vector_view<contractor::QueryGraphView::NodeArrayEntry>(
        index, name + "/contracted_graph/node_array");
    auto edge_list = make_vector_view<contractor::QueryGraphView::EdgeArrayEntry>(
        index, name + "/contracted_graph/edge_array");

    std::vector<util::vector_view<bool>> edge_filter;
    index.List(name + "/exclude",
               boost::make_function_output_iterator([&](const auto &filter_name) {
                   edge_filter.push_back(make_vector_view<bool>(index, filter_name));
               }));

    return contractor::ContractedMetricView{{std::move(node_list), std::move(edge_list)},
                                            std::move(edge_filter)};
}

inline auto make_partition_view(const SharedDataIndex &index, const std::string &name)
{
    auto level_data_ptr =
        index.template GetBlockPtr<partitioner::MultiLevelPartitionView::LevelData>(name +
                                                                                    "/level_data");
    auto partition = make_vector_view<PartitionID>(index, name + "/partition");
    auto cell_to_children = make_vector_view<CellID>(index, name + "/cell_to_children");

    return partitioner::MultiLevelPartitionView{
        level_data_ptr, std::move(partition), std::move(cell_to_children)};
}

inline auto make_timestamp_view(const SharedDataIndex &index, const std::string &name)
{
    return util::StringView(index.GetBlockPtr<char>(name), index.GetBlockEntries(name));
}

inline auto make_cell_storage_view(const SharedDataIndex &index, const std::string &name)
{
    auto source_boundary = make_vector_view<NodeID>(index, name + "/source_boundary");
    auto destination_boundary = make_vector_view<NodeID>(index, name + "/destination_boundary");
    auto cells = make_vector_view<partitioner::CellStorageView::CellData>(index, name + "/cells");
    auto level_offsets = make_vector_view<std::uint64_t>(index, name + "/level_to_cell_offset");

    return partitioner::CellStorageView{std::move(source_boundary),
                                        std::move(destination_boundary),
                                        std::move(cells),
                                        std::move(level_offsets)};
}

inline auto make_filtered_cell_metric_view(const SharedDataIndex &index,
                                           const std::string &name,
                                           const std::size_t exclude_index)
{
    customizer::CellMetricView cell_metric;

    auto prefix = name + "/exclude/" + std::to_string(exclude_index);
    auto weights_block_id = prefix + "/weights";
    auto durations_block_id = prefix + "/durations";
    auto distances_block_id = prefix + "/distances";

    auto weights = make_vector_view<EdgeWeight>(index, weights_block_id);
    auto durations = make_vector_view<EdgeDuration>(index, durations_block_id);
    auto distances = make_vector_view<EdgeDistance>(index, distances_block_id);

    return customizer::CellMetricView{
        std::move(weights), std::move(durations), std::move(distances)};
}

inline auto make_cell_metric_view(const SharedDataIndex &index, const std::string &name)
{
    std::vector<customizer::CellMetricView> cell_metric_excludes;

    std::vector<std::string> metric_prefix_names;
    index.List(name + "/exclude/", std::back_inserter(metric_prefix_names));
    for (const auto &prefix : metric_prefix_names)
    {
        auto weights_block_id = prefix + "/weights";
        auto durations_block_id = prefix + "/durations";
        auto distances_block_id = prefix + "/distances";

        auto weights = make_vector_view<EdgeWeight>(index, weights_block_id);
        auto durations = make_vector_view<EdgeDuration>(index, durations_block_id);
        auto distances = make_vector_view<EdgeDistance>(index, distances_block_id);

        cell_metric_excludes.push_back(customizer::CellMetricView{
            std::move(weights), std::move(durations), std::move(distances)});
    }

    return cell_metric_excludes;
}

inline auto make_multi_level_graph_view(const SharedDataIndex &index, const std::string &name)
{
    auto node_list = make_vector_view<customizer::MultiLevelEdgeBasedGraphView::NodeArrayEntry>(
        index, name + "/node_array");
    auto edge_list = make_vector_view<customizer::MultiLevelEdgeBasedGraphView::EdgeArrayEntry>(
        index, name + "/edge_array");
    auto node_to_offset = make_vector_view<customizer::MultiLevelEdgeBasedGraphView::EdgeOffset>(
        index, name + "/node_to_edge_offset");
    auto node_weights = make_vector_view<EdgeWeight>(index, name + "/node_weights");
    auto node_durations = make_vector_view<EdgeDuration>(index, name + "/node_durations");
    auto node_distances = make_vector_view<EdgeDistance>(index, name + "/node_distances");
    auto is_forward_edge = make_vector_view<bool>(index, name + "/is_forward_edge");
    auto is_backward_edge = make_vector_view<bool>(index, name + "/is_backward_edge");

    return customizer::MultiLevelEdgeBasedGraphView(std::move(node_list),
                                                    std::move(edge_list),
                                                    std::move(node_to_offset),
                                                    std::move(node_weights),
                                                    std::move(node_durations),
                                                    std::move(node_distances),
                                                    std::move(is_forward_edge),
                                                    std::move(is_backward_edge));
}

inline auto make_maneuver_overrides_views(const SharedDataIndex &index, const std::string &name)
{
    auto maneuver_overrides =
        make_vector_view<extractor::StorageManeuverOverride>(index, name + "/overrides");
    auto maneuver_override_node_sequences =
        make_vector_view<NodeID>(index, name + "/node_sequences");

    return std::make_tuple(maneuver_overrides, maneuver_override_node_sequences);
}

inline auto make_filtered_graph_view(const SharedDataIndex &index,
                                     const std::string &name,
                                     const std::size_t exclude_index)
{
    auto exclude_prefix = name + "/exclude/" + std::to_string(exclude_index);
    auto edge_filter = make_vector_view<bool>(index, exclude_prefix + "/edge_filter");
    auto node_list = make_vector_view<contractor::QueryGraphView::NodeArrayEntry>(
        index, name + "/contracted_graph/node_array");
    auto edge_list = make_vector_view<contractor::QueryGraphView::EdgeArrayEntry>(
        index, name + "/contracted_graph/edge_array");

    return util::FilteredGraphView<contractor::QueryGraphView>({node_list, edge_list}, edge_filter);
}
}
}

#endif

#include "storage/storage.hpp"

#include "storage/io.hpp"
#include "storage/shared_datatype.hpp"
#include "storage/shared_memory.hpp"
#include "storage/shared_memory_ownership.hpp"
#include "storage/shared_monitor.hpp"

#include "contractor/files.hpp"
#include "contractor/query_graph.hpp"

#include "customizer/edge_based_graph.hpp"
#include "customizer/files.hpp"

#include "extractor/class_data.hpp"
#include "extractor/compressed_edge_container.hpp"
#include "extractor/edge_based_edge.hpp"
#include "extractor/edge_based_node.hpp"
#include "extractor/files.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/name_table.hpp"
#include "extractor/packed_osm_ids.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/query_node.hpp"
#include "extractor/travel_mode.hpp"

#include "guidance/files.hpp"
#include "guidance/turn_instruction.hpp"

#include "partitioner/cell_storage.hpp"
#include "partitioner/edge_based_graph_reader.hpp"
#include "partitioner/files.hpp"
#include "partitioner/multi_level_partition.hpp"

#include "engine/datafacade/datafacade_base.hpp"

#include "util/coordinate.hpp"
#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/fingerprint.hpp"
#include "util/log.hpp"
#include "util/packed_vector.hpp"
#include "util/range_table.hpp"
#include "util/static_graph.hpp"
#include "util/static_rtree.hpp"
#include "util/typedefs.hpp"
#include "util/vector_view.hpp"

#ifdef __linux__
#include <sys/mman.h>
#endif

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <cstdint>

#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>

namespace osrm
{
namespace storage
{
namespace
{
inline void readBlocks(const boost::filesystem::path &path, DataLayout &layout)
{
    tar::FileReader reader(path, tar::FileReader::VerifyFingerprint);

    std::vector<tar::FileReader::FileEntry> entries;
    reader.List(std::back_inserter(entries));

    for (const auto &entry : entries)
    {
        const auto name_end = entry.name.rfind(".meta");
        if (name_end == std::string::npos)
        {
            auto number_of_elements = reader.ReadElementCount64(entry.name);
            layout.SetBlock(entry.name, Block{number_of_elements, entry.size});
        }
    }
}
}

using RTreeLeaf = engine::datafacade::BaseDataFacade::RTreeLeaf;
using RTreeNode = util::StaticRTree<RTreeLeaf, storage::Ownership::View>::TreeNode;
using QueryGraph = util::StaticGraph<contractor::QueryEdge::EdgeData>;
using EdgeBasedGraph = util::StaticGraph<extractor::EdgeBasedEdge::EdgeData>;

using Monitor = SharedMonitor<SharedDataTimestamp>;

Storage::Storage(StorageConfig config_) : config(std::move(config_)) {}

int Storage::Run(int max_wait)
{
    BOOST_ASSERT_MSG(config.IsValid(), "Invalid storage config");

    util::LogPolicy::GetInstance().Unmute();

    boost::filesystem::path lock_path =
        boost::filesystem::temp_directory_path() / "osrm-datastore.lock";
    if (!boost::filesystem::exists(lock_path))
    {
        boost::filesystem::ofstream ofs(lock_path);
    }

    boost::interprocess::file_lock file_lock(lock_path.string().c_str());
    boost::interprocess::scoped_lock<boost::interprocess::file_lock> datastore_lock(
        file_lock, boost::interprocess::defer_lock);

    if (!datastore_lock.try_lock())
    {
        util::UnbufferedLog(logWARNING) << "Data update in progress, waiting until it finishes... ";
        datastore_lock.lock();
        util::UnbufferedLog(logWARNING) << "ok.";
    }

#ifdef __linux__
    // try to disable swapping on Linux
    const bool lock_flags = MCL_CURRENT | MCL_FUTURE;
    if (-1 == mlockall(lock_flags))
    {
        util::Log(logWARNING) << "Could not request RAM lock";
    }
#endif

    // Get the next region ID and time stamp without locking shared barriers.
    // Because of datastore_lock the only write operation can occur sequentially later.
    Monitor monitor(SharedDataTimestamp{REGION_NONE, 0});
    auto in_use_region = monitor.data().region;
    auto next_timestamp = monitor.data().timestamp + 1;
    auto next_region =
        in_use_region == REGION_2 || in_use_region == REGION_NONE ? REGION_1 : REGION_2;

    // ensure that the shared memory region we want to write to is really removed
    // this is only needef for failure recovery because we actually wait for all clients
    // to detach at the end of the function
    if (storage::SharedMemory::RegionExists(next_region))
    {
        util::Log(logWARNING) << "Old shared memory region " << regionToString(next_region)
                              << " still exists.";
        util::UnbufferedLog() << "Retrying removal... ";
        storage::SharedMemory::Remove(next_region);
        util::UnbufferedLog() << "ok.";
    }

    util::Log() << "Loading data into " << regionToString(next_region);

    // Populate a memory layout into stack memory
    DataLayout layout;
    PopulateLayout(layout);

    io::BufferWriter writer;
    serialization::write(writer, layout);
    auto encoded_layout = writer.GetBuffer();

    // Allocate shared memory block
    auto regions_size = encoded_layout.size() + layout.GetSizeOfLayout();
    util::Log() << "Data layout has a size of " << encoded_layout.size() << " bytes";
    util::Log() << "Allocating shared memory of " << regions_size << " bytes";
    auto data_memory = makeSharedMemory(next_region, regions_size);

    // Copy memory layout to shared memory and populate data
    char *shared_memory_ptr = static_cast<char *>(data_memory->Ptr());
    std::copy_n(encoded_layout.data(), encoded_layout.size(), shared_memory_ptr);
    PopulateData(layout, shared_memory_ptr + encoded_layout.size());

    { // Lock for write access shared region mutex
        boost::interprocess::scoped_lock<Monitor::mutex_type> lock(monitor.get_mutex(),
                                                                   boost::interprocess::defer_lock);

        if (max_wait >= 0)
        {
            if (!lock.timed_lock(boost::posix_time::microsec_clock::universal_time() +
                                 boost::posix_time::seconds(max_wait)))
            {
                util::Log(logWARNING)
                    << "Could not aquire current region lock after " << max_wait
                    << " seconds. Removing locked block and creating a new one. All currently "
                       "attached processes will not receive notifications and must be restarted";
                Monitor::remove();
                in_use_region = REGION_NONE;
                monitor = Monitor(SharedDataTimestamp{REGION_NONE, 0});
            }
        }
        else
        {
            lock.lock();
        }

        // Update the current region ID and timestamp
        monitor.data().region = next_region;
        monitor.data().timestamp = next_timestamp;
    }

    util::Log() << "All data loaded. Notify all client about new data in "
                << regionToString(next_region) << " with timestamp " << next_timestamp;
    monitor.notify_all();

    // SHMCTL(2): Mark the segment to be destroyed. The segment will actually be destroyed
    // only after the last process detaches it.
    if (in_use_region != REGION_NONE && storage::SharedMemory::RegionExists(in_use_region))
    {
        util::UnbufferedLog() << "Marking old shared memory region "
                              << regionToString(in_use_region) << " for removal... ";

        // aquire a handle for the old shared memory region before we mark it for deletion
        // we will need this to wait for all users to detach
        auto in_use_shared_memory = makeSharedMemory(in_use_region);

        storage::SharedMemory::Remove(in_use_region);
        util::UnbufferedLog() << "ok.";

        util::UnbufferedLog() << "Waiting for clients to detach... ";
        in_use_shared_memory->WaitForDetach();
        util::UnbufferedLog() << " ok.";
    }

    util::Log() << "All clients switched.";

    return EXIT_SUCCESS;
}

/**
 * This function examines all our data files and figures out how much
 * memory needs to be allocated, and the position of each data structure
 * in that big block.  It updates the fields in the DataLayout parameter.
 */
void Storage::PopulateLayout(DataLayout &layout)
{
    {
        auto absolute_file_index_path =
            boost::filesystem::absolute(config.GetPath(".osrm.fileIndex"));

        layout.SetBlock("/common/rtree/file_index_path",
                        make_block<char>(absolute_file_index_path.string().length() + 1));
    }

    constexpr bool REQUIRED = true;
    constexpr bool OPTIONAL = false;
    std::vector<std::pair<bool, boost::filesystem::path>> tar_files = {
        {OPTIONAL, config.GetPath(".osrm.mldgr")},
        {OPTIONAL, config.GetPath(".osrm.cells")},
        {OPTIONAL, config.GetPath(".osrm.partition")},
        {OPTIONAL, config.GetPath(".osrm.cell_metrics")},
        {OPTIONAL, config.GetPath(".osrm.hsgr")},
        {REQUIRED, config.GetPath(".osrm.icd")},
        {REQUIRED, config.GetPath(".osrm.properties")},
        {REQUIRED, config.GetPath(".osrm.nbg_nodes")},
        {REQUIRED, config.GetPath(".osrm.datasource_names")},
        {REQUIRED, config.GetPath(".osrm.geometry")},
        {REQUIRED, config.GetPath(".osrm.ebg_nodes")},
        {REQUIRED, config.GetPath(".osrm.tls")},
        {REQUIRED, config.GetPath(".osrm.tld")},
        {REQUIRED, config.GetPath(".osrm.maneuver_overrides")},
        {REQUIRED, config.GetPath(".osrm.turn_weight_penalties")},
        {REQUIRED, config.GetPath(".osrm.turn_duration_penalties")},
        {REQUIRED, config.GetPath(".osrm.edges")},
        {REQUIRED, config.GetPath(".osrm.names")},
        {REQUIRED, config.GetPath(".osrm.ramIndex")},
    };

    for (const auto &file : tar_files)
    {
        if (boost::filesystem::exists(file.second))
        {
            readBlocks(file.second, layout);
        }
        else
        {
            if (file.first == REQUIRED)
            {
                throw util::exception("Could not find required filed: " +
                                      std::get<1>(file).string());
            }
        }
    }
}

void Storage::PopulateData(const DataLayout &layout, char *memory_ptr)
{
    BOOST_ASSERT(memory_ptr != nullptr);

    // Connectivity matrix checksum
    std::uint32_t turns_connectivity_checksum = 0;

    // read actual data into shared memory object //

    // store the filename of the on-disk portion of the RTree
    {
        const auto file_index_path_ptr =
            layout.GetBlockPtr<char, true>(memory_ptr, "/common/rtree/file_index_path");
        // make sure we have 0 ending
        std::fill(file_index_path_ptr,
                  file_index_path_ptr + layout.GetBlockSize("/common/rtree/file_index_path"),
                  0);
        const auto absolute_file_index_path =
            boost::filesystem::absolute(config.GetPath(".osrm.fileIndex")).string();
        BOOST_ASSERT(static_cast<std::size_t>(layout.GetBlockSize(
                         "/common/rtree/file_index_path")) >= absolute_file_index_path.size());
        std::copy(
            absolute_file_index_path.begin(), absolute_file_index_path.end(), file_index_path_ptr);
    }

    // Name data
    {
        auto blocks =
            layout.GetWritableVector<extractor::NameTableView::IndexedData::BlockReference>(
                memory_ptr, "/common/names/blocks");
        auto values = layout.GetWritableVector<extractor::NameTableView::IndexedData::ValueType>(
            memory_ptr, "/common/names/values");

        extractor::NameTableView::IndexedData index_data_view{std::move(blocks), std::move(values)};
        extractor::NameTableView name_table{index_data_view};
        extractor::files::readNames(config.GetPath(".osrm.names"), name_table);
    }

    // Turn lane data
    {
        auto turn_lane_data = layout.GetWritableVector<util::guidance::LaneTupleIdPair>(
            memory_ptr, "/common/turn_lanes/data");

        extractor::files::readTurnLaneData(config.GetPath(".osrm.tld"), turn_lane_data);
    }

    // Turn lane descriptions
    {
        auto offsets =
            layout.GetWritableVector<std::uint32_t>(memory_ptr, "/common/turn_lanes/offsets");
        auto masks = layout.GetWritableVector<extractor::TurnLaneType::Mask>(
            memory_ptr, "/common/turn_lanes/masks");

        extractor::files::readTurnLaneDescriptions(config.GetPath(".osrm.tls"), offsets, masks);
    }

    // Load edge-based nodes data
    {
        auto edge_based_node_data = layout.GetWritableVector<extractor::EdgeBasedNode>(
            memory_ptr, "/common/ebg_node_data/nodes");
        auto annotation_data = layout.GetWritableVector<extractor::NodeBasedEdgeAnnotation>(
            memory_ptr, "/common/ebg_node_data/annotations");

        extractor::EdgeBasedNodeDataView node_data(std::move(edge_based_node_data),
                                                   std::move(annotation_data));

        extractor::files::readNodeData(config.GetPath(".osrm.ebg_nodes"), node_data);
    }

    // Load original edge data
    {
        auto lane_data_ids =
            layout.GetWritableVector<LaneDataID>(memory_ptr, "/common/turn_data/lane_data_ids");

        const auto turn_instructions = layout.GetWritableVector<guidance::TurnInstruction>(
            memory_ptr, "/common/turn_data/turn_instructions");

        const auto entry_class_ids =
            layout.GetWritableVector<EntryClassID>(memory_ptr, "/common/turn_data/entry_class_ids");

        const auto pre_turn_bearings = layout.GetWritableVector<guidance::TurnBearing>(
            memory_ptr, "/common/turn_data/pre_turn_bearings");

        const auto post_turn_bearings = layout.GetWritableVector<guidance::TurnBearing>(
            memory_ptr, "/common/turn_data/post_turn_bearings");

        guidance::TurnDataView turn_data(std::move(turn_instructions),
                                         std::move(lane_data_ids),
                                         std::move(entry_class_ids),
                                         std::move(pre_turn_bearings),
                                         std::move(post_turn_bearings));
        auto connectivity_checksum_ptr =
            layout.GetBlockPtr<std::uint32_t, true>(memory_ptr, "/common/connectivity_checksum");

        guidance::files::readTurnData(
            config.GetPath(".osrm.edges"), turn_data, *connectivity_checksum_ptr);

        turns_connectivity_checksum = *connectivity_checksum_ptr;
    }

    // load compressed geometry
    {
        auto geometry_begin_indices =
            layout.GetWritableVector<unsigned>(memory_ptr, "/common/segment_data/index");

        auto node_list = layout.GetWritableVector<NodeID>(memory_ptr, "/common/segment_data/nodes");

        auto num_entries = layout.GetBlockEntries("/common/segment_data/nodes");

        extractor::SegmentDataView::SegmentWeightVector fwd_weight_list(
            layout.GetWritableVector<extractor::SegmentDataView::SegmentWeightVector::block_type>(
                memory_ptr, "/common/segment_data/forward_weights/packed"),
            num_entries);

        extractor::SegmentDataView::SegmentWeightVector rev_weight_list(
            layout.GetWritableVector<extractor::SegmentDataView::SegmentWeightVector::block_type>(
                memory_ptr, "/common/segment_data/reverse_weights/packed"),
            num_entries);

        extractor::SegmentDataView::SegmentDurationVector fwd_duration_list(
            layout.GetWritableVector<extractor::SegmentDataView::SegmentDurationVector::block_type>(
                memory_ptr, "/common/segment_data/forward_durations/packed"),
            num_entries);

        extractor::SegmentDataView::SegmentDurationVector rev_duration_list(
            layout.GetWritableVector<extractor::SegmentDataView::SegmentDurationVector::block_type>(
                memory_ptr, "/common/segment_data/reverse_durations/packed"),
            num_entries);

        auto fwd_datasources_list = layout.GetWritableVector<DatasourceID>(
            memory_ptr, "/common/segment_data/forward_data_sources");

        auto rev_datasources_list = layout.GetWritableVector<DatasourceID>(
            memory_ptr, "/common/segment_data/reverse_data_sources");

        extractor::SegmentDataView segment_data{std::move(geometry_begin_indices),
                                                std::move(node_list),
                                                std::move(fwd_weight_list),
                                                std::move(rev_weight_list),
                                                std::move(fwd_duration_list),
                                                std::move(rev_duration_list),
                                                std::move(fwd_datasources_list),
                                                std::move(rev_datasources_list)};

        extractor::files::readSegmentData(config.GetPath(".osrm.geometry"), segment_data);
    }

    {
        const auto datasources_names_ptr = layout.GetBlockPtr<extractor::Datasources, true>(
            memory_ptr, "/common/data_sources_names");
        extractor::files::readDatasources(config.GetPath(".osrm.datasource_names"),
                                          *datasources_names_ptr);
    }

    // Loading list of coordinates
    {
        auto coordinates =
            layout.GetWritableVector<util::Coordinate>(memory_ptr, "/common/coordinates");

        auto osm_node_ids = extractor::PackedOSMIDsView{
            layout.GetWritableVector<extractor::PackedOSMIDsView::block_type>(
                memory_ptr, "/common/osm_node_ids/packed"),
            coordinates.size()};

        extractor::files::readNodes(config.GetPath(".osrm.nbg_nodes"), coordinates, osm_node_ids);
    }

    // load turn weight penalties
    {
        auto turn_duration_penalties =
            layout.GetWritableVector<TurnPenalty>(memory_ptr, "/common/turn_penalty/weight");
        extractor::files::readTurnWeightPenalty(config.GetPath(".osrm.turn_weight_penalties"),
                                                turn_duration_penalties);
    }

    // load turn duration penalties
    {
        auto turn_duration_penalties =
            layout.GetWritableVector<TurnPenalty>(memory_ptr, "/common/turn_penalty/duration");
        extractor::files::readTurnDurationPenalty(config.GetPath(".osrm.turn_duration_penalties"),
                                                  turn_duration_penalties);
    }

    // store search tree portion of rtree
    {

        const auto search_tree =
            layout.GetWritableVector<RTreeNode>(memory_ptr, "/common/rtree/search_tree");

        const auto rtree_level_starts = layout.GetWritableVector<std::uint64_t>(
            memory_ptr, "/common/rtree/search_tree_level_starts");

        // we need this purely for the interface
        util::vector_view<util::Coordinate> empty_coords;

        util::StaticRTree<RTreeLeaf, storage::Ownership::View> rtree{
            std::move(search_tree),
            std::move(rtree_level_starts),
            config.GetPath(".osrm.fileIndex"),
            empty_coords};
        extractor::files::readRamIndex(config.GetPath(".osrm.ramIndex"), rtree);
    }

    // FIXME we only need to get the weight name
    std::string metric_name;
    // load profile properties
    {
        const auto profile_properties_ptr = layout.GetBlockPtr<extractor::ProfileProperties, true>(
            memory_ptr, "/common/properties");
        extractor::files::readProfileProperties(config.GetPath(".osrm.properties"),
                                                *profile_properties_ptr);

        metric_name = profile_properties_ptr->GetWeightName();
    }

    // Load intersection data
    {
        auto bearing_offsets = layout.GetWritableVector<unsigned>(
            memory_ptr, "/common/intersection_bearings/class_id_to_ranges/block_offsets");
        auto bearing_blocks =
            layout.GetWritableVector<util::RangeTable<16, storage::Ownership::View>::BlockT>(
                memory_ptr, "/common/intersection_bearings/class_id_to_ranges/diff_blocks");
        auto bearing_values = layout.GetWritableVector<DiscreteBearing>(
            memory_ptr, "/common/intersection_bearings/bearing_values");
        util::RangeTable<16, storage::Ownership::View> bearing_range_table(
            std::move(bearing_offsets),
            std::move(bearing_blocks),
            static_cast<unsigned>(bearing_values.size()));

        auto bearing_class_id = layout.GetWritableVector<BearingClassID>(
            memory_ptr, "/common/intersection_bearings/node_to_class_id");
        extractor::IntersectionBearingsView intersection_bearings_view{
            std::move(bearing_values), std::move(bearing_class_id), std::move(bearing_range_table)};

        auto entry_classes = layout.GetWritableVector<util::guidance::EntryClass>(
            memory_ptr, "/common/entry_classes");
        extractor::files::readIntersections(
            config.GetPath(".osrm.icd"), intersection_bearings_view, entry_classes);
    }

    if (boost::filesystem::exists(config.GetPath(".osrm.hsgr")))
    {
        const std::string metric_prefix = "/ch/metrics/" + metric_name;

        auto node_list = layout.GetWritableVector<contractor::QueryGraphView::NodeArrayEntry>(
            memory_ptr, metric_prefix + "/contracted_graph/node_array");
        auto edge_list = layout.GetWritableVector<contractor::QueryGraphView::EdgeArrayEntry>(
            memory_ptr, metric_prefix + "/contracted_graph/edge_array");

        std::vector<util::vector_view<bool>> edge_filter;
        layout.List(metric_prefix + "/exclude",
                    boost::make_function_output_iterator([&](const auto &name) {
                        edge_filter.push_back(layout.GetWritableVector<bool>(memory_ptr, name));
                    }));

        std::uint32_t graph_connectivity_checksum = 0;
        std::unordered_map<std::string, contractor::ContractedMetricView> metrics = {
            {metric_name, {{std::move(node_list), std::move(edge_list)}, std::move(edge_filter)}}};

        contractor::files::readGraph(
            config.GetPath(".osrm.hsgr"), metrics, graph_connectivity_checksum);
        if (turns_connectivity_checksum != graph_connectivity_checksum)
        {
            throw util::exception(
                "Connectivity checksum " + std::to_string(graph_connectivity_checksum) + " in " +
                config.GetPath(".osrm.hsgr").string() + " does not equal to checksum " +
                std::to_string(turns_connectivity_checksum) + " in " +
                config.GetPath(".osrm.edges").string());
        }
    }

    if (boost::filesystem::exists(config.GetPath(".osrm.partition")))
    {
        BOOST_ASSERT(layout.GetBlockSize("/mld/multilevelpartition/level_data") > 0);
        BOOST_ASSERT(layout.GetBlockSize("/mld/multilevelpartition/cell_to_children") > 0);
        BOOST_ASSERT(layout.GetBlockSize("/mld/multilevelpartition/partition") > 0);

        auto level_data_ptr =
            layout.GetBlockPtr<partitioner::MultiLevelPartitionView::LevelData, true>(
                memory_ptr, "/mld/multilevelpartition/level_data");
        auto partition =
            layout.GetWritableVector<PartitionID>(memory_ptr, "/mld/multilevelpartition/partition");
        auto cell_to_children = layout.GetWritableVector<CellID>(
            memory_ptr, "/mld/multilevelpartition/cell_to_children");

        partitioner::MultiLevelPartitionView mlp{
            level_data_ptr, std::move(partition), std::move(cell_to_children)};
        partitioner::files::readPartition(config.GetPath(".osrm.partition"), mlp);
    }

    if (boost::filesystem::exists(config.GetPath(".osrm.cells")))
    {
        BOOST_ASSERT(layout.GetBlockSize("/mld/cellstorage/cells") > 0);
        BOOST_ASSERT(layout.GetBlockSize("/mld/cellstorage/level_to_cell_offset") > 0);

        auto source_boundary =
            layout.GetWritableVector<NodeID>(memory_ptr, "/mld/cellstorage/source_boundary");
        auto destination_boundary =
            layout.GetWritableVector<NodeID>(memory_ptr, "/mld/cellstorage/destination_boundary");
        auto cells = layout.GetWritableVector<partitioner::CellStorageView::CellData>(
            memory_ptr, "/mld/cellstorage/cells");
        auto level_offsets = layout.GetWritableVector<std::uint64_t>(
            memory_ptr, "/mld/cellstorage/level_to_cell_offset");

        partitioner::CellStorageView storage{std::move(source_boundary),
                                             std::move(destination_boundary),
                                             std::move(cells),
                                             std::move(level_offsets)};
        partitioner::files::readCells(config.GetPath(".osrm.cells"), storage);
    }

    if (boost::filesystem::exists(config.GetPath(".osrm.cell_metrics")))
    {
        BOOST_ASSERT(layout.GetBlockSize("/mld/cellstorage/cells") > 0);
        BOOST_ASSERT(layout.GetBlockSize("/mld/cellstorage/level_to_cell_offset") > 0);

        std::unordered_map<std::string, std::vector<customizer::CellMetricView>> metrics = {
            {metric_name, {}},
        };

        std::vector<std::string> metric_prefix_names;
        layout.List("/mld/metrics/" + metric_name + "/exclude/",
                    std::back_inserter(metric_prefix_names));
        for (const auto &prefix : metric_prefix_names)
        {
            auto weights_block_id = prefix + "/weights";
            auto durations_block_id = prefix + "/durations";

            auto weights = layout.GetWritableVector<EdgeWeight>(memory_ptr, weights_block_id);
            auto durations = layout.GetWritableVector<EdgeDuration>(memory_ptr, durations_block_id);

            metrics[metric_name].push_back(
                customizer::CellMetricView{std::move(weights), std::move(durations)});
        }

        customizer::files::readCellMetrics(config.GetPath(".osrm.cell_metrics"), metrics);
    }

    if (boost::filesystem::exists(config.GetPath(".osrm.mldgr")))
    {
        auto node_list =
            layout.GetWritableVector<customizer::MultiLevelEdgeBasedGraphView::NodeArrayEntry>(
                memory_ptr, "/mld/multilevelgraph/node_array");
        auto edge_list =
            layout.GetWritableVector<customizer::MultiLevelEdgeBasedGraphView::EdgeArrayEntry>(
                memory_ptr, "/mld/multilevelgraph/edge_array");
        auto node_to_offset =
            layout.GetWritableVector<customizer::MultiLevelEdgeBasedGraphView::EdgeOffset>(
                memory_ptr, "/mld/multilevelgraph/node_to_edge_offset");

        std::uint32_t graph_connectivity_checksum = 0;
        customizer::MultiLevelEdgeBasedGraphView graph_view(
            std::move(node_list), std::move(edge_list), std::move(node_to_offset));
        partitioner::files::readGraph(
            config.GetPath(".osrm.mldgr"), graph_view, graph_connectivity_checksum);

        if (turns_connectivity_checksum != graph_connectivity_checksum)
        {
            throw util::exception(
                "Connectivity checksum " + std::to_string(graph_connectivity_checksum) + " in " +
                config.GetPath(".osrm.mldgr").string() + " does not equal to checksum " +
                std::to_string(turns_connectivity_checksum) + " in " +
                config.GetPath(".osrm.edges").string());
        }
    }

    // load maneuver overrides
    {
        auto maneuver_overrides = layout.GetWritableVector<extractor::StorageManeuverOverride>(
            memory_ptr, "/common/maneuver_overrides/overrides");
        auto maneuver_override_node_sequences = layout.GetWritableVector<NodeID>(
            memory_ptr, "/common/maneuver_overrides/node_sequences");

        extractor::files::readManeuverOverrides(config.GetPath(".osrm.maneuver_overrides"),
                                                maneuver_overrides,
                                                maneuver_override_node_sequences);
    }
}
}
}

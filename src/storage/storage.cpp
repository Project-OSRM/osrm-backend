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
template <typename OutIter> void readBlocks(const boost::filesystem::path &path, OutIter out)
{
    tar::FileReader reader(path, tar::FileReader::VerifyFingerprint);

    std::vector<tar::FileReader::TarEntry> entries;
    reader.List(std::back_inserter(entries));

    for (const auto &entry : entries)
    {
        std::string name;
        std::uint64_t size;
        std::tie(name, size) = entry;

        const auto name_end = name.rfind(".meta");
        if (name_end == std::string::npos)
        {
            auto number_of_elements = reader.ReadElementCount64(name);
            *out++ = NamedBlock{name, Block{number_of_elements, size}};
        }
    }
}
}

static constexpr std::size_t NUM_METRICS = 8;

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

    // Allocate shared memory block
    auto regions_size = sizeof(layout) + layout.GetSizeOfLayout();
    util::Log() << "Allocating shared memory of " << regions_size << " bytes";
    auto data_memory = makeSharedMemory(next_region, regions_size);

    // Copy memory layout to shared memory and populate data
    char *shared_memory_ptr = static_cast<char *>(data_memory->Ptr());
    memcpy(shared_memory_ptr, &layout, sizeof(layout));
    PopulateData(layout, shared_memory_ptr + sizeof(layout));

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

        layout.SetBlock(DataLayout::FILE_INDEX_PATH,
                        make_block<char>(absolute_file_index_path.string().length() + 1));
    }

    {
        util::Log() << "load names from: " << config.GetPath(".osrm.names");
        // number of entries in name index
        io::FileReader name_file(config.GetPath(".osrm.names"), io::FileReader::VerifyFingerprint);
        layout.SetBlock(DataLayout::NAME_CHAR_DATA, make_block<char>(name_file.GetSize()));
    }

    // Loading information for original edges
    {
        io::FileReader edges_file(config.GetPath(".osrm.edges"), io::FileReader::VerifyFingerprint);
        const auto number_of_original_edges = edges_file.ReadElementCount64();

        // note: settings this all to the same size is correct, we extract them from the same struct
        layout.SetBlock(DataLayout::PRE_TURN_BEARING,
                        make_block<guidance::TurnBearing>(number_of_original_edges));
        layout.SetBlock(DataLayout::POST_TURN_BEARING,
                        make_block<guidance::TurnBearing>(number_of_original_edges));
        layout.SetBlock(DataLayout::TURN_INSTRUCTION,
                        make_block<guidance::TurnInstruction>(number_of_original_edges));
        layout.SetBlock(DataLayout::LANE_DATA_ID, make_block<LaneDataID>(number_of_original_edges));
        layout.SetBlock(DataLayout::ENTRY_CLASSID,
                        make_block<EntryClassID>(number_of_original_edges));
    }

    // load rsearch tree size
    {
        io::FileReader tree_node_file(config.GetPath(".osrm.ramIndex"),
                                      io::FileReader::VerifyFingerprint);

        const auto tree_size = tree_node_file.ReadElementCount64();
        layout.SetBlock(DataLayout::R_SEARCH_TREE, make_block<RTreeNode>(tree_size));
        tree_node_file.Skip<RTreeNode>(tree_size);
        const auto tree_levels_size = tree_node_file.ReadElementCount64();
        layout.SetBlock(DataLayout::R_SEARCH_TREE_LEVELS,
                        make_block<std::uint64_t>(tree_levels_size));
    }

    // read timestampsize
    {
        io::FileReader timestamp_file(config.GetPath(".osrm.timestamp"),
                                      io::FileReader::VerifyFingerprint);
        const auto timestamp_size = timestamp_file.GetSize();
        layout.SetBlock(DataLayout::TIMESTAMP, make_block<char>(timestamp_size));
    }

    std::unordered_map<std::string, DataLayout::BlockID> name_to_block_id = {
        {"/mld/multilevelgraph/node_array", DataLayout::MLD_GRAPH_NODE_LIST},
        {"/mld/multilevelgraph/edge_array", DataLayout::MLD_GRAPH_EDGE_LIST},
        {"/mld/multilevelgraph/node_to_edge_offset", DataLayout::MLD_GRAPH_NODE_TO_OFFSET},
        {"/mld/multilevelgraph/connectivity_checksum", DataLayout::IGNORE_BLOCK},
        {"/mld/multilevelpartition/level_data", DataLayout::MLD_LEVEL_DATA},
        {"/mld/multilevelpartition/partition", DataLayout::MLD_PARTITION},
        {"/mld/multilevelpartition/cell_to_children", DataLayout::MLD_CELL_TO_CHILDREN},
        {"/mld/cellstorage/source_boundary", DataLayout::MLD_CELL_SOURCE_BOUNDARY},
        {"/mld/cellstorage/destination_boundary", DataLayout::MLD_CELL_DESTINATION_BOUNDARY},
        {"/mld/cellstorage/cells", DataLayout::MLD_CELLS},
        {"/mld/cellstorage/level_to_cell_offset", DataLayout::MLD_CELL_LEVEL_OFFSETS},
        {"/mld/metrics/0/weights", DataLayout::MLD_CELL_WEIGHTS_0},
        {"/mld/metrics/1/weights", DataLayout::MLD_CELL_WEIGHTS_1},
        {"/mld/metrics/2/weights", DataLayout::MLD_CELL_WEIGHTS_2},
        {"/mld/metrics/3/weights", DataLayout::MLD_CELL_WEIGHTS_3},
        {"/mld/metrics/4/weights", DataLayout::MLD_CELL_WEIGHTS_4},
        {"/mld/metrics/5/weights", DataLayout::MLD_CELL_WEIGHTS_5},
        {"/mld/metrics/6/weights", DataLayout::MLD_CELL_WEIGHTS_6},
        {"/mld/metrics/7/weights", DataLayout::MLD_CELL_WEIGHTS_7},
        {"/mld/metrics/0/durations", DataLayout::MLD_CELL_DURATIONS_0},
        {"/mld/metrics/1/durations", DataLayout::MLD_CELL_DURATIONS_1},
        {"/mld/metrics/2/durations", DataLayout::MLD_CELL_DURATIONS_2},
        {"/mld/metrics/3/durations", DataLayout::MLD_CELL_DURATIONS_3},
        {"/mld/metrics/4/durations", DataLayout::MLD_CELL_DURATIONS_4},
        {"/mld/metrics/5/durations", DataLayout::MLD_CELL_DURATIONS_5},
        {"/mld/metrics/6/durations", DataLayout::MLD_CELL_DURATIONS_6},
        {"/mld/metrics/7/durations", DataLayout::MLD_CELL_DURATIONS_7},
        {"/ch/checksum", DataLayout::HSGR_CHECKSUM},
        {"/ch/contracted_graph/node_array", DataLayout::CH_GRAPH_NODE_LIST},
        {"/ch/contracted_graph/edge_array", DataLayout::CH_GRAPH_EDGE_LIST},
        {"/ch/connectivity_checksum", DataLayout::IGNORE_BLOCK},
        {"/ch/edge_filter/0", DataLayout::CH_EDGE_FILTER_0},
        {"/ch/edge_filter/1", DataLayout::CH_EDGE_FILTER_1},
        {"/ch/edge_filter/2", DataLayout::CH_EDGE_FILTER_2},
        {"/ch/edge_filter/3", DataLayout::CH_EDGE_FILTER_3},
        {"/ch/edge_filter/4", DataLayout::CH_EDGE_FILTER_4},
        {"/ch/edge_filter/5", DataLayout::CH_EDGE_FILTER_5},
        {"/ch/edge_filter/6", DataLayout::CH_EDGE_FILTER_6},
        {"/ch/edge_filter/7", DataLayout::CH_EDGE_FILTER_7},
        {"/common/intersection_bearings/bearing_values", DataLayout::BEARING_VALUES},
        {"/common/intersection_bearings/node_to_class_id", DataLayout::BEARING_CLASSID},
        {"/common/intersection_bearings/class_id_to_ranges/block_offsets",
         DataLayout::BEARING_OFFSETS},
        {"/common/intersection_bearings/class_id_to_ranges/diff_blocks",
         DataLayout::BEARING_BLOCKS},
        {"/common/entry_classes", DataLayout::ENTRY_CLASS},
        {"/common/properties", DataLayout::PROPERTIES},
        {"/common/coordinates", DataLayout::COORDINATE_LIST},
        {"/common/osm_node_ids/packed", DataLayout::OSM_NODE_ID_LIST},
        {"/common/data_sources_names", DataLayout::DATASOURCES_NAMES},
        {"/common/segment_data/index", DataLayout::GEOMETRIES_INDEX},
        {"/common/segment_data/nodes", DataLayout::GEOMETRIES_NODE_LIST},
        {"/common/segment_data/forward_weights/packed", DataLayout::GEOMETRIES_FWD_WEIGHT_LIST},
        {"/common/segment_data/reverse_weights/packed", DataLayout::GEOMETRIES_REV_WEIGHT_LIST},
        {"/common/segment_data/forward_durations/packed", DataLayout::GEOMETRIES_FWD_DURATION_LIST},
        {"/common/segment_data/reverse_durations/packed", DataLayout::GEOMETRIES_REV_DURATION_LIST},
        {"/common/segment_data/forward_data_sources", DataLayout::GEOMETRIES_FWD_DATASOURCES_LIST},
        {"/common/segment_data/reverse_data_sources", DataLayout::GEOMETRIES_REV_DATASOURCES_LIST},
        {"/common/ebg_node_data/nodes", DataLayout::EDGE_BASED_NODE_DATA_LIST},
        {"/common/ebg_node_data/annotations", DataLayout::ANNOTATION_DATA_LIST},
        {"/common/turn_lanes/offsets", DataLayout::LANE_DESCRIPTION_OFFSETS},
        {"/common/turn_lanes/masks", DataLayout::LANE_DESCRIPTION_MASKS},
        {"/common/turn_lanes/data", DataLayout::TURN_LANE_DATA},
        {"/common/maneuver_overrides/overrides", DataLayout::MANEUVER_OVERRIDES},
        {"/common/maneuver_overrides/node_sequences", DataLayout::MANEUVER_OVERRIDE_NODE_SEQUENCES},
        {"/common/turn_penalty/weight", DataLayout::TURN_WEIGHT_PENALTIES},
        {"/common/turn_penalty/duration", DataLayout::TURN_DURATION_PENALTIES},
    };
    std::vector<NamedBlock> blocks;

    constexpr bool REQUIRED = true;
    constexpr bool OPTIONAL = false;
    std::vector<std::tuple<bool, boost::filesystem::path>> tar_files = {
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
    };

    for (const auto &file : tar_files)
    {
        if (boost::filesystem::exists(std::get<1>(file)))
        {
            readBlocks(std::get<1>(file), std::back_inserter(blocks));
        }
        else
        {
            if (std::get<0>(file) == REQUIRED)
            {
                throw util::exception("Could not find required filed: " +
                                      std::get<1>(file).string());
            }
        }
    }

    for (const auto &block : blocks)
    {
        auto id_iter = name_to_block_id.find(std::get<0>(block));
        if (id_iter == name_to_block_id.end())
        {
            throw util::exception("Could not map " + std::get<0>(block) +
                                  " to a region in memory.");
        }
        layout.SetBlock(id_iter->second, std::get<1>(block));
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
            layout.GetBlockPtr<char, true>(memory_ptr, DataLayout::FILE_INDEX_PATH);
        // make sure we have 0 ending
        std::fill(file_index_path_ptr,
                  file_index_path_ptr + layout.GetBlockSize(DataLayout::FILE_INDEX_PATH),
                  0);
        const auto absolute_file_index_path =
            boost::filesystem::absolute(config.GetPath(".osrm.fileIndex")).string();
        BOOST_ASSERT(static_cast<std::size_t>(layout.GetBlockSize(DataLayout::FILE_INDEX_PATH)) >=
                     absolute_file_index_path.size());
        std::copy(
            absolute_file_index_path.begin(), absolute_file_index_path.end(), file_index_path_ptr);
    }

    // Name data
    {
        io::FileReader name_file(config.GetPath(".osrm.names"), io::FileReader::VerifyFingerprint);
        std::size_t name_file_size = name_file.GetSize();

        BOOST_ASSERT(name_file_size == layout.GetBlockSize(DataLayout::NAME_CHAR_DATA));
        const auto name_char_ptr =
            layout.GetBlockPtr<char, true>(memory_ptr, DataLayout::NAME_CHAR_DATA);

        name_file.ReadInto<char>(name_char_ptr, name_file_size);
    }

    // Turn lane data
    {
        const auto turn_lane_data_ptr = layout.GetBlockPtr<util::guidance::LaneTupleIdPair, true>(
            memory_ptr, DataLayout::TURN_LANE_DATA);
        util::vector_view<util::guidance::LaneTupleIdPair> turn_lane_data(
            turn_lane_data_ptr, layout.GetBlockEntries(storage::DataLayout::TURN_LANE_DATA));

        extractor::files::readTurnLaneData(config.GetPath(".osrm.tld"), turn_lane_data);
    }

    // Turn lane descriptions
    {
        auto offsets_ptr = layout.GetBlockPtr<std::uint32_t, true>(
            memory_ptr, storage::DataLayout::LANE_DESCRIPTION_OFFSETS);
        util::vector_view<std::uint32_t> offsets(
            offsets_ptr, layout.GetBlockEntries(storage::DataLayout::LANE_DESCRIPTION_OFFSETS));

        auto masks_ptr = layout.GetBlockPtr<extractor::TurnLaneType::Mask, true>(
            memory_ptr, storage::DataLayout::LANE_DESCRIPTION_MASKS);
        util::vector_view<extractor::TurnLaneType::Mask> masks(
            masks_ptr, layout.GetBlockEntries(storage::DataLayout::LANE_DESCRIPTION_MASKS));

        extractor::files::readTurnLaneDescriptions(config.GetPath(".osrm.tls"), offsets, masks);
    }

    // Load edge-based nodes data
    {
        auto edge_based_node_data_list_ptr = layout.GetBlockPtr<extractor::EdgeBasedNode, true>(
            memory_ptr, storage::DataLayout::EDGE_BASED_NODE_DATA_LIST);
        util::vector_view<extractor::EdgeBasedNode> edge_based_node_data(
            edge_based_node_data_list_ptr,
            layout.GetBlockEntries(storage::DataLayout::EDGE_BASED_NODE_DATA_LIST));

        auto annotation_data_list_ptr =
            layout.GetBlockPtr<extractor::NodeBasedEdgeAnnotation, true>(
                memory_ptr, storage::DataLayout::ANNOTATION_DATA_LIST);
        util::vector_view<extractor::NodeBasedEdgeAnnotation> annotation_data(
            annotation_data_list_ptr,
            layout.GetBlockEntries(storage::DataLayout::ANNOTATION_DATA_LIST));

        extractor::EdgeBasedNodeDataView node_data(std::move(edge_based_node_data),
                                                   std::move(annotation_data));

        extractor::files::readNodeData(config.GetPath(".osrm.ebg_nodes"), node_data);
    }

    // Load original edge data
    {
        const auto lane_data_id_ptr =
            layout.GetBlockPtr<LaneDataID, true>(memory_ptr, storage::DataLayout::LANE_DATA_ID);
        util::vector_view<LaneDataID> lane_data_ids(
            lane_data_id_ptr, layout.GetBlockEntries(storage::DataLayout::LANE_DATA_ID));

        const auto turn_instruction_list_ptr = layout.GetBlockPtr<guidance::TurnInstruction, true>(
            memory_ptr, storage::DataLayout::TURN_INSTRUCTION);
        util::vector_view<guidance::TurnInstruction> turn_instructions(
            turn_instruction_list_ptr,
            layout.GetBlockEntries(storage::DataLayout::TURN_INSTRUCTION));

        const auto entry_class_id_list_ptr =
            layout.GetBlockPtr<EntryClassID, true>(memory_ptr, storage::DataLayout::ENTRY_CLASSID);
        util::vector_view<EntryClassID> entry_class_ids(
            entry_class_id_list_ptr, layout.GetBlockEntries(storage::DataLayout::ENTRY_CLASSID));

        const auto pre_turn_bearing_ptr = layout.GetBlockPtr<guidance::TurnBearing, true>(
            memory_ptr, storage::DataLayout::PRE_TURN_BEARING);
        util::vector_view<guidance::TurnBearing> pre_turn_bearings(
            pre_turn_bearing_ptr, layout.GetBlockEntries(storage::DataLayout::PRE_TURN_BEARING));

        const auto post_turn_bearing_ptr = layout.GetBlockPtr<guidance::TurnBearing, true>(
            memory_ptr, storage::DataLayout::POST_TURN_BEARING);
        util::vector_view<guidance::TurnBearing> post_turn_bearings(
            post_turn_bearing_ptr, layout.GetBlockEntries(storage::DataLayout::POST_TURN_BEARING));

        guidance::TurnDataView turn_data(std::move(turn_instructions),
                                         std::move(lane_data_ids),
                                         std::move(entry_class_ids),
                                         std::move(pre_turn_bearings),
                                         std::move(post_turn_bearings));

        guidance::files::readTurnData(
            config.GetPath(".osrm.edges"), turn_data, turns_connectivity_checksum);
    }

    // load compressed geometry
    {
        auto geometries_index_ptr =
            layout.GetBlockPtr<unsigned, true>(memory_ptr, storage::DataLayout::GEOMETRIES_INDEX);
        util::vector_view<unsigned> geometry_begin_indices(
            geometries_index_ptr, layout.GetBlockEntries(storage::DataLayout::GEOMETRIES_INDEX));

        auto num_entries = layout.GetBlockEntries(storage::DataLayout::GEOMETRIES_NODE_LIST);

        auto geometries_node_list_ptr =
            layout.GetBlockPtr<NodeID, true>(memory_ptr, storage::DataLayout::GEOMETRIES_NODE_LIST);
        util::vector_view<NodeID> geometry_node_list(geometries_node_list_ptr, num_entries);

        auto geometries_fwd_weight_list_ptr =
            layout.GetBlockPtr<extractor::SegmentDataView::SegmentWeightVector::block_type, true>(
                memory_ptr, storage::DataLayout::GEOMETRIES_FWD_WEIGHT_LIST);
        extractor::SegmentDataView::SegmentWeightVector geometry_fwd_weight_list(
            util::vector_view<extractor::SegmentDataView::SegmentWeightVector::block_type>(
                geometries_fwd_weight_list_ptr,
                layout.GetBlockEntries(storage::DataLayout::GEOMETRIES_FWD_WEIGHT_LIST)),
            num_entries);

        auto geometries_rev_weight_list_ptr =
            layout.GetBlockPtr<extractor::SegmentDataView::SegmentWeightVector::block_type, true>(
                memory_ptr, storage::DataLayout::GEOMETRIES_REV_WEIGHT_LIST);
        extractor::SegmentDataView::SegmentWeightVector geometry_rev_weight_list(
            util::vector_view<extractor::SegmentDataView::SegmentWeightVector::block_type>(
                geometries_rev_weight_list_ptr,
                layout.GetBlockEntries(storage::DataLayout::GEOMETRIES_REV_WEIGHT_LIST)),
            num_entries);

        auto geometries_fwd_duration_list_ptr =
            layout.GetBlockPtr<extractor::SegmentDataView::SegmentDurationVector::block_type, true>(
                memory_ptr, storage::DataLayout::GEOMETRIES_FWD_DURATION_LIST);
        extractor::SegmentDataView::SegmentDurationVector geometry_fwd_duration_list(
            util::vector_view<extractor::SegmentDataView::SegmentDurationVector::block_type>(
                geometries_fwd_duration_list_ptr,
                layout.GetBlockEntries(storage::DataLayout::GEOMETRIES_FWD_DURATION_LIST)),
            num_entries);

        auto geometries_rev_duration_list_ptr =
            layout.GetBlockPtr<extractor::SegmentDataView::SegmentDurationVector::block_type, true>(
                memory_ptr, storage::DataLayout::GEOMETRIES_REV_DURATION_LIST);
        extractor::SegmentDataView::SegmentDurationVector geometry_rev_duration_list(
            util::vector_view<extractor::SegmentDataView::SegmentDurationVector::block_type>(
                geometries_rev_duration_list_ptr,
                layout.GetBlockEntries(storage::DataLayout::GEOMETRIES_REV_DURATION_LIST)),
            num_entries);

        auto geometries_fwd_datasources_list_ptr = layout.GetBlockPtr<DatasourceID, true>(
            memory_ptr, storage::DataLayout::GEOMETRIES_FWD_DATASOURCES_LIST);
        util::vector_view<DatasourceID> geometry_fwd_datasources_list(
            geometries_fwd_datasources_list_ptr,
            layout.GetBlockEntries(storage::DataLayout::GEOMETRIES_FWD_DATASOURCES_LIST));

        auto geometries_rev_datasources_list_ptr = layout.GetBlockPtr<DatasourceID, true>(
            memory_ptr, storage::DataLayout::GEOMETRIES_REV_DATASOURCES_LIST);
        util::vector_view<DatasourceID> geometry_rev_datasources_list(
            geometries_rev_datasources_list_ptr,
            layout.GetBlockEntries(storage::DataLayout::GEOMETRIES_REV_DATASOURCES_LIST));

        extractor::SegmentDataView segment_data{std::move(geometry_begin_indices),
                                                std::move(geometry_node_list),
                                                std::move(geometry_fwd_weight_list),
                                                std::move(geometry_rev_weight_list),
                                                std::move(geometry_fwd_duration_list),
                                                std::move(geometry_rev_duration_list),
                                                std::move(geometry_fwd_datasources_list),
                                                std::move(geometry_rev_datasources_list)};

        extractor::files::readSegmentData(config.GetPath(".osrm.geometry"), segment_data);
    }

    {
        const auto datasources_names_ptr = layout.GetBlockPtr<extractor::Datasources, true>(
            memory_ptr, DataLayout::DATASOURCES_NAMES);
        extractor::files::readDatasources(config.GetPath(".osrm.datasource_names"),
                                          *datasources_names_ptr);
    }

    // Loading list of coordinates
    {
        const auto coordinates_ptr =
            layout.GetBlockPtr<util::Coordinate, true>(memory_ptr, DataLayout::COORDINATE_LIST);
        const auto osmnodeid_ptr =
            layout.GetBlockPtr<extractor::PackedOSMIDsView::block_type, true>(
                memory_ptr, DataLayout::OSM_NODE_ID_LIST);
        util::vector_view<util::Coordinate> coordinates(
            coordinates_ptr, layout.GetBlockEntries(DataLayout::COORDINATE_LIST));
        extractor::PackedOSMIDsView osm_node_ids(
            util::vector_view<extractor::PackedOSMIDsView::block_type>(
                osmnodeid_ptr, layout.GetBlockEntries(DataLayout::OSM_NODE_ID_LIST)),
            layout.GetBlockEntries(DataLayout::COORDINATE_LIST));

        extractor::files::readNodes(config.GetPath(".osrm.nbg_nodes"), coordinates, osm_node_ids);
    }

    // load turn weight penalties
    {
        auto turn_duration_penalties_ptr = layout.GetBlockPtr<TurnPenalty, true>(
            memory_ptr, storage::DataLayout::TURN_WEIGHT_PENALTIES);
        util::vector_view<TurnPenalty> turn_duration_penalties(
            turn_duration_penalties_ptr,
            layout.GetBlockEntries(storage::DataLayout::TURN_WEIGHT_PENALTIES));
        extractor::files::readTurnWeightPenalty(config.GetPath(".osrm.turn_weight_penalties"),
                                                  turn_duration_penalties);
    }

    // load turn duration penalties
    {
        auto turn_duration_penalties_ptr = layout.GetBlockPtr<TurnPenalty, true>(
            memory_ptr, storage::DataLayout::TURN_DURATION_PENALTIES);
        util::vector_view<TurnPenalty> turn_duration_penalties(
            turn_duration_penalties_ptr,
            layout.GetBlockEntries(storage::DataLayout::TURN_DURATION_PENALTIES));
        extractor::files::readTurnDurationPenalty(config.GetPath(".osrm.turn_duration_penalties"),
                                                  turn_duration_penalties);
    }

    // store timestamp
    {
        io::FileReader timestamp_file(config.GetPath(".osrm.timestamp"),
                                      io::FileReader::VerifyFingerprint);
        const auto timestamp_size = timestamp_file.GetSize();

        const auto timestamp_ptr =
            layout.GetBlockPtr<char, true>(memory_ptr, DataLayout::TIMESTAMP);
        BOOST_ASSERT(timestamp_size == layout.GetBlockEntries(DataLayout::TIMESTAMP));
        timestamp_file.ReadInto(timestamp_ptr, timestamp_size);
    }

    // store search tree portion of rtree
    {
        io::FileReader tree_node_file(config.GetPath(".osrm.ramIndex"),
                                      io::FileReader::VerifyFingerprint);
        // perform this read so that we're at the right stream position for the next
        // read.
        tree_node_file.Skip<std::uint64_t>(1);
        const auto rtree_ptr =
            layout.GetBlockPtr<RTreeNode, true>(memory_ptr, DataLayout::R_SEARCH_TREE);

        tree_node_file.ReadInto(rtree_ptr, layout.GetBlockEntries(DataLayout::R_SEARCH_TREE));

        tree_node_file.Skip<std::uint64_t>(1);
        const auto rtree_levelsizes_ptr =
            layout.GetBlockPtr<std::uint64_t, true>(memory_ptr, DataLayout::R_SEARCH_TREE_LEVELS);

        tree_node_file.ReadInto(rtree_levelsizes_ptr,
                                layout.GetBlockEntries(DataLayout::R_SEARCH_TREE_LEVELS));
    }

    // load profile properties
    {
        const auto profile_properties_ptr = layout.GetBlockPtr<extractor::ProfileProperties, true>(
            memory_ptr, DataLayout::PROPERTIES);
        extractor::files::readProfileProperties(config.GetPath(".osrm.properties"),
                                                *profile_properties_ptr);
    }

    // Load intersection data
    {
        auto bearing_class_id_ptr = layout.GetBlockPtr<BearingClassID, true>(
            memory_ptr, storage::DataLayout::BEARING_CLASSID);
        util::vector_view<BearingClassID> bearing_class_id(
            bearing_class_id_ptr, layout.GetBlockEntries(storage::DataLayout::BEARING_CLASSID));

        auto bearing_values_ptr = layout.GetBlockPtr<DiscreteBearing, true>(
            memory_ptr, storage::DataLayout::BEARING_VALUES);
        util::vector_view<DiscreteBearing> bearing_values(
            bearing_values_ptr, layout.GetBlockEntries(storage::DataLayout::BEARING_VALUES));

        auto offsets_ptr =
            layout.GetBlockPtr<unsigned, true>(memory_ptr, storage::DataLayout::BEARING_OFFSETS);
        auto blocks_ptr =
            layout.GetBlockPtr<util::RangeTable<16, storage::Ownership::View>::BlockT, true>(
                memory_ptr, storage::DataLayout::BEARING_BLOCKS);
        util::vector_view<unsigned> bearing_offsets(
            offsets_ptr, layout.GetBlockEntries(storage::DataLayout::BEARING_OFFSETS));
        util::vector_view<util::RangeTable<16, storage::Ownership::View>::BlockT> bearing_blocks(
            blocks_ptr, layout.GetBlockEntries(storage::DataLayout::BEARING_BLOCKS));

        util::RangeTable<16, storage::Ownership::View> bearing_range_table(
            bearing_offsets, bearing_blocks, static_cast<unsigned>(bearing_values.size()));

        extractor::IntersectionBearingsView intersection_bearings_view{
            std::move(bearing_values), std::move(bearing_class_id), std::move(bearing_range_table)};

        auto entry_class_ptr = layout.GetBlockPtr<util::guidance::EntryClass, true>(
            memory_ptr, storage::DataLayout::ENTRY_CLASS);
        util::vector_view<util::guidance::EntryClass> entry_classes(
            entry_class_ptr, layout.GetBlockEntries(storage::DataLayout::ENTRY_CLASS));

        extractor::files::readIntersections(
            config.GetPath(".osrm.icd"), intersection_bearings_view, entry_classes);
    }

    { // Load the HSGR file
        if (boost::filesystem::exists(config.GetPath(".osrm.hsgr")))
        {
            auto graph_nodes_ptr =
                layout.GetBlockPtr<contractor::QueryGraphView::NodeArrayEntry, true>(
                    memory_ptr, storage::DataLayout::CH_GRAPH_NODE_LIST);
            auto graph_edges_ptr =
                layout.GetBlockPtr<contractor::QueryGraphView::EdgeArrayEntry, true>(
                    memory_ptr, storage::DataLayout::CH_GRAPH_EDGE_LIST);
            auto checksum =
                layout.GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::HSGR_CHECKSUM);

            util::vector_view<contractor::QueryGraphView::NodeArrayEntry> node_list(
                graph_nodes_ptr, layout.GetBlockEntries(storage::DataLayout::CH_GRAPH_NODE_LIST));
            util::vector_view<contractor::QueryGraphView::EdgeArrayEntry> edge_list(
                graph_edges_ptr, layout.GetBlockEntries(storage::DataLayout::CH_GRAPH_EDGE_LIST));

            std::vector<util::vector_view<bool>> edge_filter;
            for (auto index : util::irange<std::size_t>(0, NUM_METRICS))
            {
                auto block_id =
                    static_cast<DataLayout::BlockID>(storage::DataLayout::CH_EDGE_FILTER_0 + index);
                auto data_ptr = layout.GetBlockPtr<unsigned, true>(memory_ptr, block_id);
                auto num_entries = layout.GetBlockEntries(block_id);
                edge_filter.emplace_back(data_ptr, num_entries);
            }

            std::uint32_t graph_connectivity_checksum = 0;
            contractor::QueryGraphView graph_view(std::move(node_list), std::move(edge_list));
            contractor::files::readGraph(config.GetPath(".osrm.hsgr"),
                                         *checksum,
                                         graph_view,
                                         edge_filter,
                                         graph_connectivity_checksum);
            if (turns_connectivity_checksum != graph_connectivity_checksum)
            {
                throw util::exception(
                    "Connectivity checksum " + std::to_string(graph_connectivity_checksum) +
                    " in " + config.GetPath(".osrm.hsgr").string() +
                    " does not equal to checksum " + std::to_string(turns_connectivity_checksum) +
                    " in " + config.GetPath(".osrm.edges").string());
            }
        }
        else
        {
            layout.GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::HSGR_CHECKSUM);
            layout.GetBlockPtr<contractor::QueryGraphView::NodeArrayEntry, true>(
                memory_ptr, DataLayout::CH_GRAPH_NODE_LIST);
            layout.GetBlockPtr<contractor::QueryGraphView::EdgeArrayEntry, true>(
                memory_ptr, DataLayout::CH_GRAPH_EDGE_LIST);
        }
    }

    { // Loading MLD Data
        if (boost::filesystem::exists(config.GetPath(".osrm.partition")))
        {
            BOOST_ASSERT(layout.GetBlockSize(storage::DataLayout::MLD_LEVEL_DATA) > 0);
            BOOST_ASSERT(layout.GetBlockSize(storage::DataLayout::MLD_CELL_TO_CHILDREN) > 0);
            BOOST_ASSERT(layout.GetBlockSize(storage::DataLayout::MLD_PARTITION) > 0);

            auto level_data =
                layout.GetBlockPtr<partitioner::MultiLevelPartitionView::LevelData, true>(
                    memory_ptr, storage::DataLayout::MLD_LEVEL_DATA);

            auto mld_partition_ptr = layout.GetBlockPtr<PartitionID, true>(
                memory_ptr, storage::DataLayout::MLD_PARTITION);
            auto partition_entries_count =
                layout.GetBlockEntries(storage::DataLayout::MLD_PARTITION);
            util::vector_view<PartitionID> partition(mld_partition_ptr, partition_entries_count);

            auto mld_chilren_ptr = layout.GetBlockPtr<CellID, true>(
                memory_ptr, storage::DataLayout::MLD_CELL_TO_CHILDREN);
            auto children_entries_count =
                layout.GetBlockEntries(storage::DataLayout::MLD_CELL_TO_CHILDREN);
            util::vector_view<CellID> cell_to_children(mld_chilren_ptr, children_entries_count);

            partitioner::MultiLevelPartitionView mlp{
                std::move(level_data), std::move(partition), std::move(cell_to_children)};
            partitioner::files::readPartition(config.GetPath(".osrm.partition"), mlp);
        }

        if (boost::filesystem::exists(config.GetPath(".osrm.cells")))
        {
            BOOST_ASSERT(layout.GetBlockSize(storage::DataLayout::MLD_CELLS) > 0);
            BOOST_ASSERT(layout.GetBlockSize(storage::DataLayout::MLD_CELL_LEVEL_OFFSETS) > 0);

            auto mld_source_boundary_ptr = layout.GetBlockPtr<NodeID, true>(
                memory_ptr, storage::DataLayout::MLD_CELL_SOURCE_BOUNDARY);
            auto mld_destination_boundary_ptr = layout.GetBlockPtr<NodeID, true>(
                memory_ptr, storage::DataLayout::MLD_CELL_DESTINATION_BOUNDARY);
            auto mld_cells_ptr = layout.GetBlockPtr<partitioner::CellStorageView::CellData, true>(
                memory_ptr, storage::DataLayout::MLD_CELLS);
            auto mld_cell_level_offsets_ptr = layout.GetBlockPtr<std::uint64_t, true>(
                memory_ptr, storage::DataLayout::MLD_CELL_LEVEL_OFFSETS);

            auto source_boundary_entries_count =
                layout.GetBlockEntries(storage::DataLayout::MLD_CELL_SOURCE_BOUNDARY);
            auto destination_boundary_entries_count =
                layout.GetBlockEntries(storage::DataLayout::MLD_CELL_DESTINATION_BOUNDARY);
            auto cells_entries_counts = layout.GetBlockEntries(storage::DataLayout::MLD_CELLS);
            auto cell_level_offsets_entries_count =
                layout.GetBlockEntries(storage::DataLayout::MLD_CELL_LEVEL_OFFSETS);

            util::vector_view<NodeID> source_boundary(mld_source_boundary_ptr,
                                                      source_boundary_entries_count);
            util::vector_view<NodeID> destination_boundary(mld_destination_boundary_ptr,
                                                           destination_boundary_entries_count);
            util::vector_view<partitioner::CellStorageView::CellData> cells(mld_cells_ptr,
                                                                            cells_entries_counts);
            util::vector_view<std::uint64_t> level_offsets(mld_cell_level_offsets_ptr,
                                                           cell_level_offsets_entries_count);

            partitioner::CellStorageView storage{std::move(source_boundary),
                                                 std::move(destination_boundary),
                                                 std::move(cells),
                                                 std::move(level_offsets)};
            partitioner::files::readCells(config.GetPath(".osrm.cells"), storage);
        }

        if (boost::filesystem::exists(config.GetPath(".osrm.cell_metrics")))
        {
            BOOST_ASSERT(layout.GetBlockSize(storage::DataLayout::MLD_CELLS) > 0);
            BOOST_ASSERT(layout.GetBlockSize(storage::DataLayout::MLD_CELL_LEVEL_OFFSETS) > 0);

            std::vector<customizer::CellMetricView> metrics;

            for (auto index : util::irange<std::size_t>(0, NUM_METRICS))
            {
                auto weights_block_id = static_cast<DataLayout::BlockID>(
                    storage::DataLayout::MLD_CELL_WEIGHTS_0 + index);
                auto durations_block_id = static_cast<DataLayout::BlockID>(
                    storage::DataLayout::MLD_CELL_DURATIONS_0 + index);

                auto weight_entries_count = layout.GetBlockEntries(weights_block_id);
                auto duration_entries_count = layout.GetBlockEntries(durations_block_id);
                auto mld_cell_weights_ptr =
                    layout.GetBlockPtr<EdgeWeight, true>(memory_ptr, weights_block_id);
                auto mld_cell_duration_ptr =
                    layout.GetBlockPtr<EdgeDuration, true>(memory_ptr, durations_block_id);
                util::vector_view<EdgeWeight> weights(mld_cell_weights_ptr, weight_entries_count);
                util::vector_view<EdgeDuration> durations(mld_cell_duration_ptr,
                                                          duration_entries_count);

                metrics.push_back(
                    customizer::CellMetricView{std::move(weights), std::move(durations)});
            }

            customizer::files::readCellMetrics(config.GetPath(".osrm.cell_metrics"), metrics);
        }

        if (boost::filesystem::exists(config.GetPath(".osrm.mldgr")))
        {

            auto graph_nodes_ptr =
                layout.GetBlockPtr<customizer::MultiLevelEdgeBasedGraphView::NodeArrayEntry, true>(
                    memory_ptr, storage::DataLayout::MLD_GRAPH_NODE_LIST);
            auto graph_edges_ptr =
                layout.GetBlockPtr<customizer::MultiLevelEdgeBasedGraphView::EdgeArrayEntry, true>(
                    memory_ptr, storage::DataLayout::MLD_GRAPH_EDGE_LIST);
            auto graph_node_to_offset_ptr =
                layout.GetBlockPtr<customizer::MultiLevelEdgeBasedGraphView::EdgeOffset, true>(
                    memory_ptr, storage::DataLayout::MLD_GRAPH_NODE_TO_OFFSET);

            util::vector_view<customizer::MultiLevelEdgeBasedGraphView::NodeArrayEntry> node_list(
                graph_nodes_ptr, layout.GetBlockEntries(storage::DataLayout::MLD_GRAPH_NODE_LIST));
            util::vector_view<customizer::MultiLevelEdgeBasedGraphView::EdgeArrayEntry> edge_list(
                graph_edges_ptr, layout.GetBlockEntries(storage::DataLayout::MLD_GRAPH_EDGE_LIST));
            util::vector_view<customizer::MultiLevelEdgeBasedGraphView::EdgeOffset> node_to_offset(
                graph_node_to_offset_ptr,
                layout.GetBlockEntries(storage::DataLayout::MLD_GRAPH_NODE_TO_OFFSET));

            std::uint32_t graph_connectivity_checksum = 0;
            customizer::MultiLevelEdgeBasedGraphView graph_view(
                std::move(node_list), std::move(edge_list), std::move(node_to_offset));
            partitioner::files::readGraph(
                config.GetPath(".osrm.mldgr"), graph_view, graph_connectivity_checksum);

            if (turns_connectivity_checksum != graph_connectivity_checksum)
            {
                throw util::exception(
                    "Connectivity checksum " + std::to_string(graph_connectivity_checksum) +
                    " in " + config.GetPath(".osrm.mldgr").string() +
                    " does not equal to checksum " + std::to_string(turns_connectivity_checksum) +
                    " in " + config.GetPath(".osrm.edges").string());
            }
        }

        // load maneuver overrides
        {
            const auto maneuver_overrides_ptr =
                layout.GetBlockPtr<extractor::StorageManeuverOverride, true>(
                    memory_ptr, DataLayout::MANEUVER_OVERRIDES);
            const auto maneuver_override_node_sequences_ptr = layout.GetBlockPtr<NodeID, true>(
                memory_ptr, DataLayout::MANEUVER_OVERRIDE_NODE_SEQUENCES);

            util::vector_view<extractor::StorageManeuverOverride> maneuver_overrides(
                maneuver_overrides_ptr, layout.GetBlockEntries(DataLayout::MANEUVER_OVERRIDES));
            util::vector_view<NodeID> maneuver_override_node_sequences(
                maneuver_override_node_sequences_ptr,
                layout.GetBlockEntries(DataLayout::MANEUVER_OVERRIDE_NODE_SEQUENCES));

            extractor::files::readManeuverOverrides(config.GetPath(".osrm.maneuver_overrides"),
                                                    maneuver_overrides,
                                                    maneuver_override_node_sequences);
        }
    }
}
}
}

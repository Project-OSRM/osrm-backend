#include "storage/storage.hpp"

#include "storage/io.hpp"
#include "storage/shared_datatype.hpp"
#include "storage/shared_memory.hpp"
#include "storage/shared_memory_ownership.hpp"
#include "storage/shared_monitor.hpp"

#include "contractor/files.hpp"
#include "contractor/query_graph.hpp"

#include "customizer/edge_based_graph.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/edge_based_edge.hpp"
#include "extractor/files.hpp"
#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/original_edge_data.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/query_node.hpp"
#include "extractor/travel_mode.hpp"

#include "partition/cell_storage.hpp"
#include "partition/edge_based_graph_reader.hpp"
#include "partition/files.hpp"
#include "partition/multi_level_partition.hpp"

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
        auto absolute_file_index_path = boost::filesystem::absolute(config.file_index_path);

        layout.SetBlockSize<char>(DataLayout::FILE_INDEX_PATH,
                                  absolute_file_index_path.string().length() + 1);
    }

    {
        util::Log() << "load names from: " << config.names_data_path;
        // number of entries in name index
        io::FileReader name_file(config.names_data_path, io::FileReader::HasNoFingerprint);
        layout.SetBlockSize<char>(DataLayout::NAME_CHAR_DATA, name_file.GetSize());
    }

    {
        io::FileReader reader(config.turn_lane_description_path, io::FileReader::HasNoFingerprint);
        auto num_offsets = reader.ReadVectorSize<std::uint32_t>();
        auto num_masks = reader.ReadVectorSize<extractor::guidance::TurnLaneType::Mask>();

        layout.SetBlockSize<std::uint32_t>(DataLayout::LANE_DESCRIPTION_OFFSETS, num_offsets);
        layout.SetBlockSize<extractor::guidance::TurnLaneType::Mask>(
            DataLayout::LANE_DESCRIPTION_MASKS, num_masks);
    }

    // Loading information for original edges
    {
        io::FileReader edges_file(config.edges_data_path, io::FileReader::HasNoFingerprint);
        const auto number_of_original_edges = edges_file.ReadElementCount64();

        // note: settings this all to the same size is correct, we extract them from the same struct
        layout.SetBlockSize<NodeID>(DataLayout::VIA_NODE_LIST, number_of_original_edges);
        layout.SetBlockSize<unsigned>(DataLayout::NAME_ID_LIST, number_of_original_edges);
        layout.SetBlockSize<extractor::TravelMode>(DataLayout::TRAVEL_MODE,
                                                   number_of_original_edges);
        layout.SetBlockSize<util::guidance::TurnBearing>(DataLayout::PRE_TURN_BEARING,
                                                         number_of_original_edges);
        layout.SetBlockSize<util::guidance::TurnBearing>(DataLayout::POST_TURN_BEARING,
                                                         number_of_original_edges);
        layout.SetBlockSize<extractor::guidance::TurnInstruction>(DataLayout::TURN_INSTRUCTION,
                                                                  number_of_original_edges);
        layout.SetBlockSize<LaneDataID>(DataLayout::LANE_DATA_ID, number_of_original_edges);
        layout.SetBlockSize<EntryClassID>(DataLayout::ENTRY_CLASSID, number_of_original_edges);
    }

    if (boost::filesystem::exists(config.hsgr_data_path))
    {
        io::FileReader reader(config.hsgr_data_path, io::FileReader::VerifyFingerprint);

        reader.Skip<std::uint32_t>(1); // checksum
        auto num_nodes = reader.ReadVectorSize<contractor::QueryGraph::NodeArrayEntry>();
        auto num_edges = reader.ReadVectorSize<contractor::QueryGraph::EdgeArrayEntry>();

        layout.SetBlockSize<unsigned>(DataLayout::HSGR_CHECKSUM, 1);
        layout.SetBlockSize<contractor::QueryGraph::NodeArrayEntry>(DataLayout::CH_GRAPH_NODE_LIST,
                                                                    num_nodes);
        layout.SetBlockSize<contractor::QueryGraph::EdgeArrayEntry>(DataLayout::CH_GRAPH_EDGE_LIST,
                                                                    num_edges);
    }
    else
    {
        layout.SetBlockSize<unsigned>(DataLayout::HSGR_CHECKSUM, 0);
        layout.SetBlockSize<contractor::QueryGraph::NodeArrayEntry>(DataLayout::CH_GRAPH_NODE_LIST,
                                                                    0);
        layout.SetBlockSize<contractor::QueryGraph::EdgeArrayEntry>(DataLayout::CH_GRAPH_EDGE_LIST,
                                                                    0);
    }

    // load rsearch tree size
    {
        io::FileReader tree_node_file(config.ram_index_path, io::FileReader::HasNoFingerprint);

        const auto tree_size = tree_node_file.ReadElementCount64();
        layout.SetBlockSize<RTreeNode>(DataLayout::R_SEARCH_TREE, tree_size);
    }

    {
        layout.SetBlockSize<extractor::ProfileProperties>(DataLayout::PROPERTIES, 1);
    }

    // read timestampsize
    {
        io::FileReader timestamp_file(config.timestamp_path, io::FileReader::HasNoFingerprint);
        const auto timestamp_size = timestamp_file.Size();
        layout.SetBlockSize<char>(DataLayout::TIMESTAMP, timestamp_size);
    }

    // load core marker size
    if (boost::filesystem::exists(config.core_data_path))
    {
        io::FileReader core_marker_file(config.core_data_path, io::FileReader::HasNoFingerprint);
        const auto number_of_core_markers = core_marker_file.ReadElementCount32();
        layout.SetBlockSize<unsigned>(DataLayout::CH_CORE_MARKER, number_of_core_markers);
    }
    else
    {
        layout.SetBlockSize<unsigned>(DataLayout::CH_CORE_MARKER, 0);
    }

    // load turn weight penalties
    {
        io::FileReader turn_weight_penalties_file(config.turn_weight_penalties_path,
                                                  io::FileReader::HasNoFingerprint);
        const auto number_of_penalties = turn_weight_penalties_file.ReadElementCount64();
        layout.SetBlockSize<TurnPenalty>(DataLayout::TURN_WEIGHT_PENALTIES, number_of_penalties);
    }

    // load turn duration penalties
    {
        io::FileReader turn_duration_penalties_file(config.turn_duration_penalties_path,
                                                    io::FileReader::HasNoFingerprint);
        const auto number_of_penalties = turn_duration_penalties_file.ReadElementCount64();
        layout.SetBlockSize<TurnPenalty>(DataLayout::TURN_DURATION_PENALTIES, number_of_penalties);
    }

    // load coordinate size
    {
        io::FileReader node_file(config.nodes_data_path, io::FileReader::VerifyFingerprint);
        const auto coordinate_list_size = node_file.ReadElementCount64();
        layout.SetBlockSize<util::Coordinate>(DataLayout::COORDINATE_LIST, coordinate_list_size);
        // we'll read a list of OSM node IDs from the same data, so set the block size for the same
        // number of items:
        layout.SetBlockSize<std::uint64_t>(
            DataLayout::OSM_NODE_ID_LIST,
            util::PackedVectorView<OSMNodeID>::elements_to_blocks(coordinate_list_size));
    }

    // load geometries sizes
    {
        io::FileReader reader(config.geometries_path, io::FileReader::HasNoFingerprint);

        const auto number_of_geometries_indices = reader.ReadVectorSize<unsigned>();
        layout.SetBlockSize<unsigned>(DataLayout::GEOMETRIES_INDEX, number_of_geometries_indices);

        const auto number_of_compressed_geometries = reader.ReadVectorSize<NodeID>();
        layout.SetBlockSize<NodeID>(DataLayout::GEOMETRIES_NODE_LIST,
                                    number_of_compressed_geometries);
        layout.SetBlockSize<EdgeWeight>(DataLayout::GEOMETRIES_FWD_WEIGHT_LIST,
                                        number_of_compressed_geometries);
        layout.SetBlockSize<EdgeWeight>(DataLayout::GEOMETRIES_REV_WEIGHT_LIST,
                                        number_of_compressed_geometries);
        layout.SetBlockSize<EdgeWeight>(DataLayout::GEOMETRIES_FWD_DURATION_LIST,
                                        number_of_compressed_geometries);
        layout.SetBlockSize<EdgeWeight>(DataLayout::GEOMETRIES_REV_DURATION_LIST,
                                        number_of_compressed_geometries);
        layout.SetBlockSize<DatasourceID>(DataLayout::DATASOURCES_LIST,
                                          number_of_compressed_geometries);
    }

    // Load datasource name sizes.
    {
        layout.SetBlockSize<extractor::Datasources>(DataLayout::DATASOURCES_NAMES, 1);
    }

    {
        io::FileReader intersection_file(config.intersection_class_path,
                                         io::FileReader::VerifyFingerprint);

        std::vector<BearingClassID> bearing_class_id_table;
        serialization::read(intersection_file, bearing_class_id_table);

        layout.SetBlockSize<BearingClassID>(DataLayout::BEARING_CLASSID,
                                            bearing_class_id_table.size());

        const auto bearing_blocks = intersection_file.ReadElementCount32();
        intersection_file.Skip<std::uint32_t>(1); // sum_lengths

        layout.SetBlockSize<unsigned>(DataLayout::BEARING_OFFSETS, bearing_blocks);
        layout.SetBlockSize<typename util::RangeTable<16, storage::Ownership::View>::BlockT>(
            DataLayout::BEARING_BLOCKS, bearing_blocks);

        // No need to read the data
        intersection_file.Skip<unsigned>(bearing_blocks);
        intersection_file.Skip<typename util::RangeTable<16, storage::Ownership::View>::BlockT>(
            bearing_blocks);

        const auto num_bearings = intersection_file.ReadElementCount64();

        // Skip over the actual data
        intersection_file.Skip<DiscreteBearing>(num_bearings);

        layout.SetBlockSize<DiscreteBearing>(DataLayout::BEARING_VALUES, num_bearings);

        std::vector<util::guidance::EntryClass> entry_class_table;
        serialization::read(intersection_file, entry_class_table);

        layout.SetBlockSize<util::guidance::EntryClass>(DataLayout::ENTRY_CLASS,
                                                        entry_class_table.size());
    }

    {
        // Loading turn lane data
        io::FileReader lane_data_file(config.turn_lane_data_path, io::FileReader::HasNoFingerprint);
        const auto lane_tuple_count = lane_data_file.ReadElementCount64();
        layout.SetBlockSize<util::guidance::LaneTupleIdPair>(DataLayout::TURN_LANE_DATA,
                                                             lane_tuple_count);
    }

    {
        // Loading MLD Data
        if (boost::filesystem::exists(config.mld_partition_path))
        {
            io::FileReader reader(config.mld_partition_path, io::FileReader::VerifyFingerprint);

            reader.Skip<partition::MultiLevelPartition::LevelData>(1);
            layout.SetBlockSize<partition::MultiLevelPartition::LevelData>(
                DataLayout::MLD_LEVEL_DATA, 1);
            const auto partition_entries_count = reader.ReadVectorSize<PartitionID>();
            layout.SetBlockSize<PartitionID>(DataLayout::MLD_PARTITION, partition_entries_count);
            const auto children_entries_count = reader.ReadVectorSize<CellID>();
            layout.SetBlockSize<CellID>(DataLayout::MLD_CELL_TO_CHILDREN, children_entries_count);
        }
        else
        {
            layout.SetBlockSize<partition::MultiLevelPartition::LevelData>(
                DataLayout::MLD_LEVEL_DATA, 0);
            layout.SetBlockSize<PartitionID>(DataLayout::MLD_PARTITION, 0);
            layout.SetBlockSize<CellID>(DataLayout::MLD_CELL_TO_CHILDREN, 0);
        }

        if (boost::filesystem::exists(config.mld_storage_path))
        {
            io::FileReader reader(config.mld_storage_path, io::FileReader::VerifyFingerprint);

            const auto weights_count = reader.ReadVectorSize<EdgeWeight>();
            layout.SetBlockSize<EdgeWeight>(DataLayout::MLD_CELL_WEIGHTS, weights_count);
            const auto source_node_count = reader.ReadVectorSize<NodeID>();
            layout.SetBlockSize<NodeID>(DataLayout::MLD_CELL_SOURCE_BOUNDARY, source_node_count);
            const auto destination_node_count = reader.ReadVectorSize<NodeID>();
            layout.SetBlockSize<NodeID>(DataLayout::MLD_CELL_DESTINATION_BOUNDARY,
                                        destination_node_count);
            const auto cell_count = reader.ReadVectorSize<partition::CellStorage::CellData>();
            layout.SetBlockSize<partition::CellStorage::CellData>(DataLayout::MLD_CELLS,
                                                                  cell_count);
            const auto level_offsets_count = reader.ReadVectorSize<std::uint64_t>();
            layout.SetBlockSize<std::uint64_t>(DataLayout::MLD_CELL_LEVEL_OFFSETS,
                                               level_offsets_count);
        }
        else
        {
            layout.SetBlockSize<char>(DataLayout::MLD_CELL_WEIGHTS, 0);
            layout.SetBlockSize<char>(DataLayout::MLD_CELL_SOURCE_BOUNDARY, 0);
            layout.SetBlockSize<char>(DataLayout::MLD_CELL_DESTINATION_BOUNDARY, 0);
            layout.SetBlockSize<char>(DataLayout::MLD_CELLS, 0);
            layout.SetBlockSize<char>(DataLayout::MLD_CELL_LEVEL_OFFSETS, 0);
        }

        if (boost::filesystem::exists(config.mld_graph_path))
        {
            io::FileReader reader(config.mld_graph_path, io::FileReader::VerifyFingerprint);

            const auto num_nodes =
                reader.ReadVectorSize<customizer::MultiLevelEdgeBasedGraph::NodeArrayEntry>();
            const auto num_edges =
                reader.ReadVectorSize<customizer::MultiLevelEdgeBasedGraph::EdgeArrayEntry>();
            const auto num_node_offsets =
                reader.ReadVectorSize<customizer::MultiLevelEdgeBasedGraph::EdgeOffset>();

            layout.SetBlockSize<customizer::MultiLevelEdgeBasedGraph::NodeArrayEntry>(
                DataLayout::MLD_GRAPH_NODE_LIST, num_nodes);
            layout.SetBlockSize<customizer::MultiLevelEdgeBasedGraph::EdgeArrayEntry>(
                DataLayout::MLD_GRAPH_EDGE_LIST, num_edges);
            layout.SetBlockSize<customizer::MultiLevelEdgeBasedGraph::EdgeOffset>(
                DataLayout::MLD_GRAPH_NODE_TO_OFFSET, num_node_offsets);
        }
        else
        {
            layout.SetBlockSize<customizer::MultiLevelEdgeBasedGraph::NodeArrayEntry>(
                DataLayout::MLD_GRAPH_NODE_LIST, 0);
            layout.SetBlockSize<customizer::MultiLevelEdgeBasedGraph::EdgeArrayEntry>(
                DataLayout::MLD_GRAPH_EDGE_LIST, 0);
            layout.SetBlockSize<customizer::MultiLevelEdgeBasedGraph::EdgeOffset>(
                DataLayout::MLD_GRAPH_NODE_TO_OFFSET, 0);
        }
    }
}

void Storage::PopulateData(const DataLayout &layout, char *memory_ptr)
{
    BOOST_ASSERT(memory_ptr != nullptr);

    // read actual data into shared memory object //

    // Load the HSGR file
    if (boost::filesystem::exists(config.hsgr_data_path))
    {
        auto graph_nodes_ptr = layout.GetBlockPtr<contractor::QueryGraphView::NodeArrayEntry, true>(
            memory_ptr, storage::DataLayout::CH_GRAPH_NODE_LIST);
        auto graph_edges_ptr = layout.GetBlockPtr<contractor::QueryGraphView::EdgeArrayEntry, true>(
            memory_ptr, storage::DataLayout::CH_GRAPH_EDGE_LIST);
        auto checksum = layout.GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::HSGR_CHECKSUM);

        util::vector_view<contractor::QueryGraphView::NodeArrayEntry> node_list(
            graph_nodes_ptr, layout.num_entries[storage::DataLayout::CH_GRAPH_NODE_LIST]);
        util::vector_view<contractor::QueryGraphView::EdgeArrayEntry> edge_list(
            graph_edges_ptr, layout.num_entries[storage::DataLayout::CH_GRAPH_EDGE_LIST]);

        contractor::QueryGraphView graph_view(std::move(node_list), std::move(edge_list));
        contractor::files::readGraph(config.hsgr_data_path, *checksum, graph_view);
    }
    else
    {
        layout.GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::HSGR_CHECKSUM);
        layout.GetBlockPtr<contractor::QueryGraphView::NodeArrayEntry, true>(
            memory_ptr, DataLayout::CH_GRAPH_NODE_LIST);
        layout.GetBlockPtr<contractor::QueryGraphView::EdgeArrayEntry, true>(
            memory_ptr, DataLayout::CH_GRAPH_EDGE_LIST);
    }

    // store the filename of the on-disk portion of the RTree
    {
        const auto file_index_path_ptr =
            layout.GetBlockPtr<char, true>(memory_ptr, DataLayout::FILE_INDEX_PATH);
        // make sure we have 0 ending
        std::fill(file_index_path_ptr,
                  file_index_path_ptr + layout.GetBlockSize(DataLayout::FILE_INDEX_PATH),
                  0);
        const auto absolute_file_index_path =
            boost::filesystem::absolute(config.file_index_path).string();
        BOOST_ASSERT(static_cast<std::size_t>(layout.GetBlockSize(DataLayout::FILE_INDEX_PATH)) >=
                     absolute_file_index_path.size());
        std::copy(
            absolute_file_index_path.begin(), absolute_file_index_path.end(), file_index_path_ptr);
    }

    // Name data
    {
        io::FileReader name_file(config.names_data_path, io::FileReader::HasNoFingerprint);
        std::size_t name_file_size = name_file.GetSize();

        BOOST_ASSERT(name_file_size == layout.GetBlockSize(DataLayout::NAME_CHAR_DATA));
        const auto name_char_ptr =
            layout.GetBlockPtr<char, true>(memory_ptr, DataLayout::NAME_CHAR_DATA);

        name_file.ReadInto<char>(name_char_ptr, name_file_size);
    }

    // Turn lane data
    {
        io::FileReader lane_data_file(config.turn_lane_data_path, io::FileReader::HasNoFingerprint);

        const auto lane_tuple_count = lane_data_file.ReadElementCount64();

        // Need to call GetBlockPtr -> it write the memory canary, even if no data needs to be
        // loaded.
        const auto turn_lane_data_ptr = layout.GetBlockPtr<util::guidance::LaneTupleIdPair, true>(
            memory_ptr, DataLayout::TURN_LANE_DATA);
        BOOST_ASSERT(lane_tuple_count * sizeof(util::guidance::LaneTupleIdPair) ==
                     layout.GetBlockSize(DataLayout::TURN_LANE_DATA));
        lane_data_file.ReadInto(turn_lane_data_ptr, lane_tuple_count);
    }

    // Turn lane descriptions
    {
        auto offsets_ptr = layout.GetBlockPtr<std::uint32_t, true>(
            memory_ptr, storage::DataLayout::LANE_DESCRIPTION_OFFSETS);
        util::vector_view<std::uint32_t> offsets(
            offsets_ptr, layout.num_entries[storage::DataLayout::LANE_DESCRIPTION_OFFSETS]);

        auto masks_ptr = layout.GetBlockPtr<extractor::guidance::TurnLaneType::Mask, true>(
            memory_ptr, storage::DataLayout::LANE_DESCRIPTION_MASKS);
        util::vector_view<extractor::guidance::TurnLaneType::Mask> masks(
            masks_ptr, layout.num_entries[storage::DataLayout::LANE_DESCRIPTION_MASKS]);

        extractor::files::readTurnLaneDescriptions(
            config.turn_lane_description_path, offsets, masks);
    }

    // Load original edge data
    {
        auto via_geometry_list_ptr =
            layout.GetBlockPtr<GeometryID, true>(memory_ptr, storage::DataLayout::VIA_NODE_LIST);
        util::vector_view<GeometryID> geometry_ids(
            via_geometry_list_ptr, layout.num_entries[storage::DataLayout::VIA_NODE_LIST]);

        const auto travel_mode_list_ptr = layout.GetBlockPtr<extractor::TravelMode, true>(
            memory_ptr, storage::DataLayout::TRAVEL_MODE);
        util::vector_view<extractor::TravelMode> travel_modes(
            travel_mode_list_ptr, layout.num_entries[storage::DataLayout::TRAVEL_MODE]);

        const auto lane_data_id_ptr =
            layout.GetBlockPtr<LaneDataID, true>(memory_ptr, storage::DataLayout::LANE_DATA_ID);
        util::vector_view<LaneDataID> lane_data_ids(
            lane_data_id_ptr, layout.num_entries[storage::DataLayout::LANE_DATA_ID]);

        const auto turn_instruction_list_ptr =
            layout.GetBlockPtr<extractor::guidance::TurnInstruction, true>(
                memory_ptr, storage::DataLayout::TURN_INSTRUCTION);
        util::vector_view<extractor::guidance::TurnInstruction> turn_instructions(
            turn_instruction_list_ptr, layout.num_entries[storage::DataLayout::TURN_INSTRUCTION]);

        const auto name_id_list_ptr =
            layout.GetBlockPtr<NameID, true>(memory_ptr, storage::DataLayout::NAME_ID_LIST);
        util::vector_view<NameID> name_ids(name_id_list_ptr,
                                           layout.num_entries[storage::DataLayout::NAME_ID_LIST]);

        const auto entry_class_id_list_ptr =
            layout.GetBlockPtr<EntryClassID, true>(memory_ptr, storage::DataLayout::ENTRY_CLASSID);
        util::vector_view<EntryClassID> entry_class_ids(
            entry_class_id_list_ptr, layout.num_entries[storage::DataLayout::ENTRY_CLASSID]);

        const auto pre_turn_bearing_ptr = layout.GetBlockPtr<util::guidance::TurnBearing, true>(
            memory_ptr, storage::DataLayout::PRE_TURN_BEARING);
        util::vector_view<util::guidance::TurnBearing> pre_turn_bearings(
            pre_turn_bearing_ptr, layout.num_entries[storage::DataLayout::PRE_TURN_BEARING]);

        const auto post_turn_bearing_ptr = layout.GetBlockPtr<util::guidance::TurnBearing, true>(
            memory_ptr, storage::DataLayout::POST_TURN_BEARING);
        util::vector_view<util::guidance::TurnBearing> post_turn_bearings(
            post_turn_bearing_ptr, layout.num_entries[storage::DataLayout::POST_TURN_BEARING]);

        extractor::TurnDataView turn_data(std::move(geometry_ids),
                                          std::move(name_ids),
                                          std::move(turn_instructions),
                                          std::move(lane_data_ids),
                                          std::move(travel_modes),
                                          std::move(entry_class_ids),
                                          std::move(pre_turn_bearings),
                                          std::move(post_turn_bearings));

        extractor::files::readTurnData(config.edges_data_path, turn_data);
    }

    // load compressed geometry
    {
        auto geometries_index_ptr =
            layout.GetBlockPtr<unsigned, true>(memory_ptr, storage::DataLayout::GEOMETRIES_INDEX);
        util::vector_view<unsigned> geometry_begin_indices(
            geometries_index_ptr, layout.num_entries[storage::DataLayout::GEOMETRIES_INDEX]);

        auto geometries_node_list_ptr =
            layout.GetBlockPtr<NodeID, true>(memory_ptr, storage::DataLayout::GEOMETRIES_NODE_LIST);
        util::vector_view<NodeID> geometry_node_list(
            geometries_node_list_ptr,
            layout.num_entries[storage::DataLayout::GEOMETRIES_NODE_LIST]);

        auto geometries_fwd_weight_list_ptr = layout.GetBlockPtr<EdgeWeight, true>(
            memory_ptr, storage::DataLayout::GEOMETRIES_FWD_WEIGHT_LIST);
        util::vector_view<EdgeWeight> geometry_fwd_weight_list(
            geometries_fwd_weight_list_ptr,
            layout.num_entries[storage::DataLayout::GEOMETRIES_FWD_WEIGHT_LIST]);

        auto geometries_rev_weight_list_ptr = layout.GetBlockPtr<EdgeWeight, true>(
            memory_ptr, storage::DataLayout::GEOMETRIES_REV_WEIGHT_LIST);
        util::vector_view<EdgeWeight> geometry_rev_weight_list(
            geometries_rev_weight_list_ptr,
            layout.num_entries[storage::DataLayout::GEOMETRIES_REV_WEIGHT_LIST]);

        auto geometries_fwd_duration_list_ptr = layout.GetBlockPtr<EdgeWeight, true>(
            memory_ptr, storage::DataLayout::GEOMETRIES_FWD_DURATION_LIST);
        util::vector_view<EdgeWeight> geometry_fwd_duration_list(
            geometries_fwd_duration_list_ptr,
            layout.num_entries[storage::DataLayout::GEOMETRIES_FWD_DURATION_LIST]);

        auto geometries_rev_duration_list_ptr = layout.GetBlockPtr<EdgeWeight, true>(
            memory_ptr, storage::DataLayout::GEOMETRIES_REV_DURATION_LIST);
        util::vector_view<EdgeWeight> geometry_rev_duration_list(
            geometries_rev_duration_list_ptr,
            layout.num_entries[storage::DataLayout::GEOMETRIES_REV_DURATION_LIST]);

        auto datasources_list_ptr = layout.GetBlockPtr<DatasourceID, true>(
            memory_ptr, storage::DataLayout::DATASOURCES_LIST);
        util::vector_view<DatasourceID> datasources_list(
            datasources_list_ptr, layout.num_entries[storage::DataLayout::DATASOURCES_LIST]);

        extractor::SegmentDataView segment_data{std::move(geometry_begin_indices),
                                                std::move(geometry_node_list),
                                                std::move(geometry_fwd_weight_list),
                                                std::move(geometry_rev_weight_list),
                                                std::move(geometry_fwd_duration_list),
                                                std::move(geometry_rev_duration_list),
                                                std::move(datasources_list)};

        extractor::files::readSegmentData(config.geometries_path, segment_data);
    }

    {
        const auto datasources_names_ptr = layout.GetBlockPtr<extractor::Datasources, true>(
            memory_ptr, DataLayout::DATASOURCES_NAMES);
        extractor::files::readDatasources(config.datasource_names_path, *datasources_names_ptr);
    }

    // Loading list of coordinates
    {
        const auto coordinates_ptr =
            layout.GetBlockPtr<util::Coordinate, true>(memory_ptr, DataLayout::COORDINATE_LIST);
        const auto osmnodeid_ptr =
            layout.GetBlockPtr<std::uint64_t, true>(memory_ptr, DataLayout::OSM_NODE_ID_LIST);
        util::vector_view<util::Coordinate> coordinates(
            coordinates_ptr, layout.num_entries[DataLayout::COORDINATE_LIST]);
        util::PackedVectorView<OSMNodeID> osm_node_ids;
        osm_node_ids.reset(osmnodeid_ptr, layout.num_entries[DataLayout::OSM_NODE_ID_LIST]);

        extractor::files::readNodes(config.nodes_data_path, coordinates, osm_node_ids);
    }

    // load turn weight penalties
    {
        io::FileReader turn_weight_penalties_file(config.turn_weight_penalties_path,
                                                  io::FileReader::HasNoFingerprint);
        const auto number_of_penalties = turn_weight_penalties_file.ReadElementCount64();
        const auto turn_weight_penalties_ptr =
            layout.GetBlockPtr<TurnPenalty, true>(memory_ptr, DataLayout::TURN_WEIGHT_PENALTIES);
        turn_weight_penalties_file.ReadInto(turn_weight_penalties_ptr, number_of_penalties);
    }

    // load turn duration penalties
    {
        io::FileReader turn_duration_penalties_file(config.turn_duration_penalties_path,
                                                    io::FileReader::HasNoFingerprint);
        const auto number_of_penalties = turn_duration_penalties_file.ReadElementCount64();
        const auto turn_duration_penalties_ptr =
            layout.GetBlockPtr<TurnPenalty, true>(memory_ptr, DataLayout::TURN_DURATION_PENALTIES);
        turn_duration_penalties_file.ReadInto(turn_duration_penalties_ptr, number_of_penalties);
    }

    // store timestamp
    {
        io::FileReader timestamp_file(config.timestamp_path, io::FileReader::HasNoFingerprint);
        const auto timestamp_size = timestamp_file.Size();

        const auto timestamp_ptr =
            layout.GetBlockPtr<char, true>(memory_ptr, DataLayout::TIMESTAMP);
        BOOST_ASSERT(timestamp_size == layout.num_entries[DataLayout::TIMESTAMP]);
        timestamp_file.ReadInto(timestamp_ptr, timestamp_size);
    }

    // store search tree portion of rtree
    {
        io::FileReader tree_node_file(config.ram_index_path, io::FileReader::HasNoFingerprint);
        // perform this read so that we're at the right stream position for the next
        // read.
        tree_node_file.Skip<std::uint64_t>(1);
        const auto rtree_ptr =
            layout.GetBlockPtr<RTreeNode, true>(memory_ptr, DataLayout::R_SEARCH_TREE);

        tree_node_file.ReadInto(rtree_ptr, layout.num_entries[DataLayout::R_SEARCH_TREE]);
    }

    if (boost::filesystem::exists(config.core_data_path))
    {
        io::FileReader core_marker_file(config.core_data_path, io::FileReader::HasNoFingerprint);
        const auto number_of_core_markers = core_marker_file.ReadElementCount32();

        // load core markers
        std::vector<char> unpacked_core_markers(number_of_core_markers);
        core_marker_file.ReadInto(unpacked_core_markers.data(), number_of_core_markers);

        const auto core_marker_ptr =
            layout.GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::CH_CORE_MARKER);

        for (auto i = 0u; i < number_of_core_markers; ++i)
        {
            BOOST_ASSERT(unpacked_core_markers[i] == 0 || unpacked_core_markers[i] == 1);

            if (unpacked_core_markers[i] == 1)
            {
                const unsigned bucket = i / 32;
                const unsigned offset = i % 32;
                const unsigned value = [&] {
                    unsigned return_value = 0;
                    if (0 != offset)
                    {
                        return_value = core_marker_ptr[bucket];
                    }
                    return return_value;
                }();

                core_marker_ptr[bucket] = (value | (1u << offset));
            }
        }
    }

    // load profile properties
    {
        io::FileReader profile_properties_file(config.properties_path,
                                               io::FileReader::HasNoFingerprint);
        const auto profile_properties_ptr = layout.GetBlockPtr<extractor::ProfileProperties, true>(
            memory_ptr, DataLayout::PROPERTIES);
        profile_properties_file.ReadInto(profile_properties_ptr,
                                         layout.num_entries[DataLayout::PROPERTIES]);
    }

    // Load intersection data
    {
        io::FileReader intersection_file(config.intersection_class_path,
                                         io::FileReader::VerifyFingerprint);

        std::vector<BearingClassID> bearing_class_id_table;
        serialization::read(intersection_file, bearing_class_id_table);

        const auto bearing_blocks = intersection_file.ReadElementCount32();
        intersection_file.Skip<std::uint32_t>(1); // sum_lengths

        std::vector<unsigned> bearing_offsets_data(bearing_blocks);
        std::vector<typename util::RangeTable<16, storage::Ownership::View>::BlockT>
            bearing_blocks_data(bearing_blocks);

        intersection_file.ReadInto(bearing_offsets_data.data(), bearing_blocks);
        intersection_file.ReadInto(bearing_blocks_data.data(), bearing_blocks);

        const auto num_bearings = intersection_file.ReadElementCount64();

        std::vector<DiscreteBearing> bearing_class_table(num_bearings);
        intersection_file.ReadInto(bearing_class_table.data(), num_bearings);

        std::vector<util::guidance::EntryClass> entry_class_table;
        serialization::read(intersection_file, entry_class_table);

        // load intersection classes
        if (!bearing_class_id_table.empty())
        {
            const auto bearing_id_ptr =
                layout.GetBlockPtr<BearingClassID, true>(memory_ptr, DataLayout::BEARING_CLASSID);
            BOOST_ASSERT(
                static_cast<std::size_t>(layout.GetBlockSize(DataLayout::BEARING_CLASSID)) >=
                std::distance(bearing_class_id_table.begin(), bearing_class_id_table.end()) *
                    sizeof(decltype(bearing_class_id_table)::value_type));
            std::copy(bearing_class_id_table.begin(), bearing_class_id_table.end(), bearing_id_ptr);
        }

        if (layout.GetBlockSize(DataLayout::BEARING_OFFSETS) > 0)
        {
            const auto bearing_offsets_ptr =
                layout.GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::BEARING_OFFSETS);
            BOOST_ASSERT(
                static_cast<std::size_t>(layout.GetBlockSize(DataLayout::BEARING_OFFSETS)) >=
                std::distance(bearing_offsets_data.begin(), bearing_offsets_data.end()) *
                    sizeof(decltype(bearing_offsets_data)::value_type));
            std::copy(
                bearing_offsets_data.begin(), bearing_offsets_data.end(), bearing_offsets_ptr);
        }

        if (layout.GetBlockSize(DataLayout::BEARING_BLOCKS) > 0)
        {
            const auto bearing_blocks_ptr =
                layout.GetBlockPtr<typename util::RangeTable<16, storage::Ownership::View>::BlockT,
                                   true>(memory_ptr, DataLayout::BEARING_BLOCKS);
            BOOST_ASSERT(
                static_cast<std::size_t>(layout.GetBlockSize(DataLayout::BEARING_BLOCKS)) >=
                std::distance(bearing_blocks_data.begin(), bearing_blocks_data.end()) *
                    sizeof(decltype(bearing_blocks_data)::value_type));
            std::copy(bearing_blocks_data.begin(), bearing_blocks_data.end(), bearing_blocks_ptr);
        }

        if (!bearing_class_table.empty())
        {
            const auto bearing_class_ptr =
                layout.GetBlockPtr<DiscreteBearing, true>(memory_ptr, DataLayout::BEARING_VALUES);
            BOOST_ASSERT(
                static_cast<std::size_t>(layout.GetBlockSize(DataLayout::BEARING_VALUES)) >=
                std::distance(bearing_class_table.begin(), bearing_class_table.end()) *
                    sizeof(decltype(bearing_class_table)::value_type));
            std::copy(bearing_class_table.begin(), bearing_class_table.end(), bearing_class_ptr);
        }

        if (!entry_class_table.empty())
        {
            const auto entry_class_ptr = layout.GetBlockPtr<util::guidance::EntryClass, true>(
                memory_ptr, DataLayout::ENTRY_CLASS);
            BOOST_ASSERT(static_cast<std::size_t>(layout.GetBlockSize(DataLayout::ENTRY_CLASS)) >=
                         std::distance(entry_class_table.begin(), entry_class_table.end()) *
                             sizeof(decltype(entry_class_table)::value_type));
            std::copy(entry_class_table.begin(), entry_class_table.end(), entry_class_ptr);
        }
    }

    {
        // Loading MLD Data
        if (boost::filesystem::exists(config.mld_partition_path))
        {
            BOOST_ASSERT(layout.GetBlockSize(storage::DataLayout::MLD_LEVEL_DATA) > 0);
            BOOST_ASSERT(layout.GetBlockSize(storage::DataLayout::MLD_CELL_TO_CHILDREN) > 0);
            BOOST_ASSERT(layout.GetBlockSize(storage::DataLayout::MLD_PARTITION) > 0);

            auto level_data =
                layout.GetBlockPtr<partition::MultiLevelPartitionView::LevelData, true>(
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

            partition::MultiLevelPartitionView mlp{
                std::move(level_data), std::move(partition), std::move(cell_to_children)};
            partition::files::readPartition(config.mld_partition_path, mlp);
        }

        if (boost::filesystem::exists(config.mld_storage_path))
        {
            BOOST_ASSERT(layout.GetBlockSize(storage::DataLayout::MLD_CELLS) > 0);
            BOOST_ASSERT(layout.GetBlockSize(storage::DataLayout::MLD_CELL_LEVEL_OFFSETS) > 0);

            auto mld_cell_weights_ptr = layout.GetBlockPtr<EdgeWeight, true>(
                memory_ptr, storage::DataLayout::MLD_CELL_WEIGHTS);
            auto mld_source_boundary_ptr = layout.GetBlockPtr<NodeID, true>(
                memory_ptr, storage::DataLayout::MLD_CELL_SOURCE_BOUNDARY);
            auto mld_destination_boundary_ptr = layout.GetBlockPtr<NodeID, true>(
                memory_ptr, storage::DataLayout::MLD_CELL_DESTINATION_BOUNDARY);
            auto mld_cells_ptr = layout.GetBlockPtr<partition::CellStorageView::CellData, true>(
                memory_ptr, storage::DataLayout::MLD_CELLS);
            auto mld_cell_level_offsets_ptr = layout.GetBlockPtr<std::uint64_t, true>(
                memory_ptr, storage::DataLayout::MLD_CELL_LEVEL_OFFSETS);

            auto weight_entries_count =
                layout.GetBlockEntries(storage::DataLayout::MLD_CELL_WEIGHTS);
            auto source_boundary_entries_count =
                layout.GetBlockEntries(storage::DataLayout::MLD_CELL_SOURCE_BOUNDARY);
            auto destination_boundary_entries_count =
                layout.GetBlockEntries(storage::DataLayout::MLD_CELL_DESTINATION_BOUNDARY);
            auto cells_entries_counts = layout.GetBlockEntries(storage::DataLayout::MLD_CELLS);
            auto cell_level_offsets_entries_count =
                layout.GetBlockEntries(storage::DataLayout::MLD_CELL_LEVEL_OFFSETS);

            util::vector_view<EdgeWeight> weights(mld_cell_weights_ptr, weight_entries_count);
            util::vector_view<NodeID> source_boundary(mld_source_boundary_ptr,
                                                      source_boundary_entries_count);
            util::vector_view<NodeID> destination_boundary(mld_destination_boundary_ptr,
                                                           destination_boundary_entries_count);
            util::vector_view<partition::CellStorageView::CellData> cells(mld_cells_ptr,
                                                                          cells_entries_counts);
            util::vector_view<std::uint64_t> level_offsets(mld_cell_level_offsets_ptr,
                                                           cell_level_offsets_entries_count);

            partition::CellStorageView storage{std::move(weights),
                                               std::move(source_boundary),
                                               std::move(destination_boundary),
                                               std::move(cells),
                                               std::move(level_offsets)};
            partition::files::readCells(config.mld_storage_path, storage);
        }

        if (boost::filesystem::exists(config.mld_graph_path))
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
                graph_nodes_ptr, layout.num_entries[storage::DataLayout::MLD_GRAPH_NODE_LIST]);
            util::vector_view<customizer::MultiLevelEdgeBasedGraphView::EdgeArrayEntry> edge_list(
                graph_edges_ptr, layout.num_entries[storage::DataLayout::MLD_GRAPH_EDGE_LIST]);
            util::vector_view<customizer::MultiLevelEdgeBasedGraphView::EdgeOffset> node_to_offset(
                graph_node_to_offset_ptr,
                layout.num_entries[storage::DataLayout::MLD_GRAPH_NODE_TO_OFFSET]);

            customizer::MultiLevelEdgeBasedGraphView graph_view(
                std::move(node_list), std::move(edge_list), std::move(node_to_offset));
            partition::files::readGraph(config.mld_graph_path, graph_view);
        }
    }
}
}
}

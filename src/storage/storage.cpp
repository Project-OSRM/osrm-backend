#include "storage/storage.hpp"
#include "contractor/query_edge.hpp"
#include "extractor/compressed_edge_container.hpp"
#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/original_edge_data.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/query_node.hpp"
#include "extractor/travel_mode.hpp"
#include "storage/io.hpp"
#include "storage/serialization.hpp"
#include "storage/shared_datatype.hpp"
#include "storage/shared_memory.hpp"
#include "storage/shared_monitor.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "util/cell_storage.hpp"
#include "util/coordinate.hpp"
#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/fingerprint.hpp"
#include "util/io.hpp"
#include "util/log.hpp"
#include "util/multi_level_partition.hpp"
#include "util/packed_vector.hpp"
#include "util/range_table.hpp"
#include "util/shared_memory_vector_wrapper.hpp"
#include "util/static_graph.hpp"
#include "util/static_rtree.hpp"
#include "util/typedefs.hpp"

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
using RTreeNode =
    util::StaticRTree<RTreeLeaf, util::ShM<util::Coordinate, true>::vector, true>::TreeNode;
using QueryGraph = util::StaticGraph<contractor::QueryEdge::EdgeData>;

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
        std::vector<std::uint32_t> lane_description_offsets;
        std::vector<extractor::guidance::TurnLaneType::Mask> lane_description_masks;
        util::deserializeAdjacencyArray(config.turn_lane_description_path.string(),
                                        lane_description_offsets,
                                        lane_description_masks);
        layout.SetBlockSize<std::uint32_t>(DataLayout::LANE_DESCRIPTION_OFFSETS,
                                           lane_description_offsets.size());
        layout.SetBlockSize<extractor::guidance::TurnLaneType::Mask>(
            DataLayout::LANE_DESCRIPTION_MASKS, lane_description_masks.size());
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

    {
        io::FileReader hsgr_file(config.hsgr_data_path, io::FileReader::VerifyFingerprint);

        const auto hsgr_header = serialization::readHSGRHeader(hsgr_file);
        layout.SetBlockSize<unsigned>(DataLayout::HSGR_CHECKSUM, 1);
        layout.SetBlockSize<QueryGraph::NodeArrayEntry>(DataLayout::CH_GRAPH_NODE_LIST,
                                                        hsgr_header.number_of_nodes);
        layout.SetBlockSize<QueryGraph::EdgeArrayEntry>(DataLayout::CH_GRAPH_EDGE_LIST,
                                                        hsgr_header.number_of_edges);
    }

    // load rsearch tree size
    {
        io::FileReader tree_node_file(config.ram_index_path, io::FileReader::HasNoFingerprint);

        const auto tree_size = tree_node_file.ReadElementCount64();
        layout.SetBlockSize<RTreeNode>(DataLayout::R_SEARCH_TREE, tree_size);
    }

    {
        // allocate space in shared memory for profile properties
        const auto properties_size = serialization::readPropertiesCount();
        layout.SetBlockSize<extractor::ProfileProperties>(DataLayout::PROPERTIES, properties_size);
    }

    // read timestampsize
    {
        io::FileReader timestamp_file(config.timestamp_path, io::FileReader::HasNoFingerprint);
        const auto timestamp_size = timestamp_file.Size();
        layout.SetBlockSize<char>(DataLayout::TIMESTAMP, timestamp_size);
    }

    // load core marker size
    {
        io::FileReader core_marker_file(config.core_data_path, io::FileReader::HasNoFingerprint);
        const auto number_of_core_markers = core_marker_file.ReadElementCount32();
        layout.SetBlockSize<unsigned>(DataLayout::CH_CORE_MARKER, number_of_core_markers);
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
        io::FileReader node_file(config.nodes_data_path, io::FileReader::HasNoFingerprint);
        const auto coordinate_list_size = node_file.ReadElementCount64();
        layout.SetBlockSize<util::Coordinate>(DataLayout::COORDINATE_LIST, coordinate_list_size);
        // we'll read a list of OSM node IDs from the same data, so set the block size for the same
        // number of items:
        layout.SetBlockSize<std::uint64_t>(
            DataLayout::OSM_NODE_ID_LIST,
            util::PackedVector<OSMNodeID>::elements_to_blocks(coordinate_list_size));
    }

    // load geometries sizes
    {
        io::FileReader geometry_file(config.geometries_path, io::FileReader::HasNoFingerprint);

        const auto number_of_geometries_indices = geometry_file.ReadElementCount32();
        layout.SetBlockSize<unsigned>(DataLayout::GEOMETRIES_INDEX, number_of_geometries_indices);

        geometry_file.Skip<unsigned>(number_of_geometries_indices);

        const auto number_of_compressed_geometries = geometry_file.ReadElementCount32();
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
    }

    // load datasource sizes.  This file is optional, and it's non-fatal if it doesn't
    // exist.
    {
        io::FileReader geometry_datasource_file(config.datasource_indexes_path,
                                                io::FileReader::HasNoFingerprint);
        const auto number_of_compressed_datasources = geometry_datasource_file.ReadElementCount64();
        layout.SetBlockSize<uint8_t>(DataLayout::DATASOURCES_LIST,
                                     number_of_compressed_datasources);
    }

    // Load datasource name sizes.  This file is optional, and it's non-fatal if it doesn't
    // exist
    {
        io::FileReader datasource_names_file(config.datasource_names_path,
                                             io::FileReader::HasNoFingerprint);

        const serialization::DatasourceNamesData datasource_names_data =
            serialization::readDatasourceNames(datasource_names_file);

        layout.SetBlockSize<char>(DataLayout::DATASOURCE_NAME_DATA,
                                  datasource_names_data.names.size());
        layout.SetBlockSize<std::size_t>(DataLayout::DATASOURCE_NAME_OFFSETS,
                                         datasource_names_data.offsets.size());
        layout.SetBlockSize<std::size_t>(DataLayout::DATASOURCE_NAME_LENGTHS,
                                         datasource_names_data.lengths.size());
    }

    {
        io::FileReader intersection_file(config.intersection_class_path,
                                         io::FileReader::VerifyFingerprint);

        std::vector<BearingClassID> bearing_class_id_table;
        intersection_file.DeserializeVector(bearing_class_id_table);

        layout.SetBlockSize<BearingClassID>(DataLayout::BEARING_CLASSID,
                                            bearing_class_id_table.size());

        const auto bearing_blocks = intersection_file.ReadElementCount32();
        intersection_file.Skip<std::uint32_t>(1); // sum_lengths

        layout.SetBlockSize<unsigned>(DataLayout::BEARING_OFFSETS, bearing_blocks);
        layout.SetBlockSize<typename util::RangeTable<16, true>::BlockT>(DataLayout::BEARING_BLOCKS,
                                                                         bearing_blocks);

        // No need to read the data
        intersection_file.Skip<unsigned>(bearing_blocks);
        intersection_file.Skip<typename util::RangeTable<16, true>::BlockT>(bearing_blocks);

        const auto num_bearings = intersection_file.ReadElementCount64();

        // Skip over the actual data
        intersection_file.Skip<DiscreteBearing>(num_bearings);

        layout.SetBlockSize<DiscreteBearing>(DataLayout::BEARING_VALUES, num_bearings);

        std::vector<util::guidance::EntryClass> entry_class_table;
        intersection_file.DeserializeVector(entry_class_table);

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

            reader.Skip<util::MultiLevelPartition::LevelData>(1);
            layout.SetBlockSize<util::MultiLevelPartition::LevelData>(DataLayout::MLD_LEVEL_DATA,
                                                                      1);
            const auto partition_entries_count = reader.ReadVectorSize<util::PartitionID>();
            layout.SetBlockSize<util::PartitionID>(DataLayout::MLD_PARTITION,
                                                   partition_entries_count);
            const auto children_entries_count = reader.ReadVectorSize<util::CellID>();
            layout.SetBlockSize<util::CellID>(DataLayout::MLD_CELL_TO_CHILDREN,
                                              children_entries_count);
        }
        else
        {
            layout.SetBlockSize<util::MultiLevelPartition::LevelData>(DataLayout::MLD_LEVEL_DATA,
                                                                      0);
            layout.SetBlockSize<util::PartitionID>(DataLayout::MLD_PARTITION, 0);
            layout.SetBlockSize<util::CellID>(DataLayout::MLD_CELL_TO_CHILDREN, 0);
        }

        if (boost::filesystem::exists(config.mld_storage_path))
        {
            auto mld_cell_storage_size =
                util::CellStorage<true>().GetRequiredMemorySize(config.mld_storage_path);
            layout.SetBlockSize<char>(DataLayout::MLD_CELL_STORAGE, mld_cell_storage_size);
        }
        else
            layout.SetBlockSize<char>(DataLayout::MLD_CELL_STORAGE, 0);
    }
}

void Storage::PopulateData(const DataLayout &layout, char *memory_ptr)
{
    BOOST_ASSERT(memory_ptr != nullptr);

    // read actual data into shared memory object //

    // Load the HSGR file
    {
        io::FileReader hsgr_file(config.hsgr_data_path, io::FileReader::VerifyFingerprint);
        auto hsgr_header = serialization::readHSGRHeader(hsgr_file);
        unsigned *checksum_ptr =
            layout.GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::HSGR_CHECKSUM);
        *checksum_ptr = hsgr_header.checksum;

        // load the nodes of the search graph
        QueryGraph::NodeArrayEntry *graph_node_list_ptr =
            layout.GetBlockPtr<QueryGraph::NodeArrayEntry, true>(memory_ptr,
                                                                 DataLayout::CH_GRAPH_NODE_LIST);

        // load the edges of the search graph
        QueryGraph::EdgeArrayEntry *graph_edge_list_ptr =
            layout.GetBlockPtr<QueryGraph::EdgeArrayEntry, true>(memory_ptr,
                                                                 DataLayout::CH_GRAPH_EDGE_LIST);

        serialization::readHSGR(hsgr_file,
                                graph_node_list_ptr,
                                hsgr_header.number_of_nodes,
                                graph_edge_list_ptr,
                                hsgr_header.number_of_edges);
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
        std::vector<std::uint32_t> lane_description_offsets;
        std::vector<extractor::guidance::TurnLaneType::Mask> lane_description_masks;
        util::deserializeAdjacencyArray(config.turn_lane_description_path.string(),
                                        lane_description_offsets,
                                        lane_description_masks);

        const auto turn_lane_offset_ptr = layout.GetBlockPtr<std::uint32_t, true>(
            memory_ptr, DataLayout::LANE_DESCRIPTION_OFFSETS);
        if (!lane_description_offsets.empty())
        {
            BOOST_ASSERT(
                static_cast<std::size_t>(
                    layout.GetBlockSize(DataLayout::LANE_DESCRIPTION_OFFSETS)) >=
                std::distance(lane_description_offsets.begin(), lane_description_offsets.end()) *
                    sizeof(decltype(lane_description_offsets)::value_type));
            std::copy(lane_description_offsets.begin(),
                      lane_description_offsets.end(),
                      turn_lane_offset_ptr);
        }

        const auto turn_lane_mask_ptr =
            layout.GetBlockPtr<extractor::guidance::TurnLaneType::Mask, true>(
                memory_ptr, DataLayout::LANE_DESCRIPTION_MASKS);
        if (!lane_description_masks.empty())
        {
            BOOST_ASSERT(
                static_cast<std::size_t>(layout.GetBlockSize(DataLayout::LANE_DESCRIPTION_MASKS)) >=
                std::distance(lane_description_masks.begin(), lane_description_masks.end()) *
                    sizeof(decltype(lane_description_masks)::value_type));
            std::copy(
                lane_description_masks.begin(), lane_description_masks.end(), turn_lane_mask_ptr);
        }
    }

    // Load original edge data
    {
        io::FileReader edges_input_file(config.edges_data_path, io::FileReader::HasNoFingerprint);

        const auto number_of_original_edges = edges_input_file.ReadElementCount64();

        const auto via_geometry_ptr =
            layout.GetBlockPtr<GeometryID, true>(memory_ptr, DataLayout::VIA_NODE_LIST);

        const auto name_id_ptr =
            layout.GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::NAME_ID_LIST);

        const auto travel_mode_ptr =
            layout.GetBlockPtr<extractor::TravelMode, true>(memory_ptr, DataLayout::TRAVEL_MODE);
        const auto pre_turn_bearing_ptr = layout.GetBlockPtr<util::guidance::TurnBearing, true>(
            memory_ptr, DataLayout::PRE_TURN_BEARING);
        const auto post_turn_bearing_ptr = layout.GetBlockPtr<util::guidance::TurnBearing, true>(
            memory_ptr, DataLayout::POST_TURN_BEARING);

        const auto lane_data_id_ptr =
            layout.GetBlockPtr<LaneDataID, true>(memory_ptr, DataLayout::LANE_DATA_ID);

        const auto turn_instructions_ptr =
            layout.GetBlockPtr<extractor::guidance::TurnInstruction, true>(
                memory_ptr, DataLayout::TURN_INSTRUCTION);

        const auto entry_class_id_ptr =
            layout.GetBlockPtr<EntryClassID, true>(memory_ptr, DataLayout::ENTRY_CLASSID);

        serialization::readEdges(edges_input_file,
                                 via_geometry_ptr,
                                 name_id_ptr,
                                 turn_instructions_ptr,
                                 lane_data_id_ptr,
                                 travel_mode_ptr,
                                 entry_class_id_ptr,
                                 pre_turn_bearing_ptr,
                                 post_turn_bearing_ptr,
                                 number_of_original_edges);
    }

    // load compressed geometry
    {
        io::FileReader geometry_input_file(config.geometries_path,
                                           io::FileReader::HasNoFingerprint);

        const auto geometry_index_count = geometry_input_file.ReadElementCount32();
        const auto geometries_index_ptr =
            layout.GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::GEOMETRIES_INDEX);
        BOOST_ASSERT(geometry_index_count == layout.num_entries[DataLayout::GEOMETRIES_INDEX]);
        geometry_input_file.ReadInto(geometries_index_ptr, geometry_index_count);

        const auto geometries_node_id_list_ptr =
            layout.GetBlockPtr<NodeID, true>(memory_ptr, DataLayout::GEOMETRIES_NODE_LIST);
        const auto geometry_node_lists_count = geometry_input_file.ReadElementCount32();
        BOOST_ASSERT(geometry_node_lists_count ==
                     layout.num_entries[DataLayout::GEOMETRIES_NODE_LIST]);
        geometry_input_file.ReadInto(geometries_node_id_list_ptr, geometry_node_lists_count);

        const auto geometries_fwd_weight_list_ptr = layout.GetBlockPtr<EdgeWeight, true>(
            memory_ptr, DataLayout::GEOMETRIES_FWD_WEIGHT_LIST);
        BOOST_ASSERT(geometry_node_lists_count ==
                     layout.num_entries[DataLayout::GEOMETRIES_FWD_WEIGHT_LIST]);
        geometry_input_file.ReadInto(geometries_fwd_weight_list_ptr, geometry_node_lists_count);

        const auto geometries_rev_weight_list_ptr = layout.GetBlockPtr<EdgeWeight, true>(
            memory_ptr, DataLayout::GEOMETRIES_REV_WEIGHT_LIST);
        BOOST_ASSERT(geometry_node_lists_count ==
                     layout.num_entries[DataLayout::GEOMETRIES_REV_WEIGHT_LIST]);
        geometry_input_file.ReadInto(geometries_rev_weight_list_ptr, geometry_node_lists_count);

        const auto geometries_fwd_duration_list_ptr = layout.GetBlockPtr<EdgeWeight, true>(
            memory_ptr, DataLayout::GEOMETRIES_FWD_DURATION_LIST);
        BOOST_ASSERT(geometry_node_lists_count ==
                     layout.num_entries[DataLayout::GEOMETRIES_FWD_DURATION_LIST]);
        geometry_input_file.ReadInto(geometries_fwd_duration_list_ptr, geometry_node_lists_count);

        const auto geometries_rev_duration_list_ptr = layout.GetBlockPtr<EdgeWeight, true>(
            memory_ptr, DataLayout::GEOMETRIES_REV_DURATION_LIST);
        BOOST_ASSERT(geometry_node_lists_count ==
                     layout.num_entries[DataLayout::GEOMETRIES_REV_DURATION_LIST]);
        geometry_input_file.ReadInto(geometries_rev_duration_list_ptr, geometry_node_lists_count);
    }

    {
        io::FileReader geometry_datasource_file(config.datasource_indexes_path,
                                                io::FileReader::HasNoFingerprint);
        const auto number_of_compressed_datasources = geometry_datasource_file.ReadElementCount64();

        // load datasource information (if it exists)
        const auto datasources_list_ptr =
            layout.GetBlockPtr<uint8_t, true>(memory_ptr, DataLayout::DATASOURCES_LIST);
        if (number_of_compressed_datasources > 0)
        {
            serialization::readDatasourceIndexes(
                geometry_datasource_file, datasources_list_ptr, number_of_compressed_datasources);
        }
    }

    {
        /* Load names */
        io::FileReader datasource_names_file(config.datasource_names_path,
                                             io::FileReader::HasNoFingerprint);

        const auto datasource_names_data =
            serialization::readDatasourceNames(datasource_names_file);

        // load datasource name information (if it exists)
        const auto datasource_name_data_ptr =
            layout.GetBlockPtr<char, true>(memory_ptr, DataLayout::DATASOURCE_NAME_DATA);
        if (layout.GetBlockSize(DataLayout::DATASOURCE_NAME_DATA) > 0)
        {
            BOOST_ASSERT(std::distance(datasource_names_data.names.begin(),
                                       datasource_names_data.names.end()) *
                             sizeof(decltype(datasource_names_data.names)::value_type) <=
                         layout.GetBlockSize(DataLayout::DATASOURCE_NAME_DATA));
            std::copy(datasource_names_data.names.begin(),
                      datasource_names_data.names.end(),
                      datasource_name_data_ptr);
        }

        const auto datasource_name_offsets_ptr =
            layout.GetBlockPtr<std::size_t, true>(memory_ptr, DataLayout::DATASOURCE_NAME_OFFSETS);
        if (layout.GetBlockSize(DataLayout::DATASOURCE_NAME_OFFSETS) > 0)
        {
            BOOST_ASSERT(std::distance(datasource_names_data.offsets.begin(),
                                       datasource_names_data.offsets.end()) *
                             sizeof(decltype(datasource_names_data.offsets)::value_type) <=
                         layout.GetBlockSize(DataLayout::DATASOURCE_NAME_OFFSETS));
            std::copy(datasource_names_data.offsets.begin(),
                      datasource_names_data.offsets.end(),
                      datasource_name_offsets_ptr);
        }

        const auto datasource_name_lengths_ptr =
            layout.GetBlockPtr<std::size_t, true>(memory_ptr, DataLayout::DATASOURCE_NAME_LENGTHS);
        if (layout.GetBlockSize(DataLayout::DATASOURCE_NAME_LENGTHS) > 0)
        {
            BOOST_ASSERT(std::distance(datasource_names_data.lengths.begin(),
                                       datasource_names_data.lengths.end()) *
                             sizeof(decltype(datasource_names_data.lengths)::value_type) <=
                         layout.GetBlockSize(DataLayout::DATASOURCE_NAME_LENGTHS));
            std::copy(datasource_names_data.lengths.begin(),
                      datasource_names_data.lengths.end(),
                      datasource_name_lengths_ptr);
        }
    }

    // Loading list of coordinates
    {
        io::FileReader nodes_file(config.nodes_data_path, io::FileReader::HasNoFingerprint);
        nodes_file.Skip<std::uint64_t>(1); // node_count
        const auto coordinates_ptr =
            layout.GetBlockPtr<util::Coordinate, true>(memory_ptr, DataLayout::COORDINATE_LIST);
        const auto osmnodeid_ptr =
            layout.GetBlockPtr<std::uint64_t, true>(memory_ptr, DataLayout::OSM_NODE_ID_LIST);
        util::PackedVector<OSMNodeID, true> osmnodeid_list;

        osmnodeid_list.reset(osmnodeid_ptr, layout.num_entries[DataLayout::OSM_NODE_ID_LIST]);

        serialization::readNodes(nodes_file,
                                 coordinates_ptr,
                                 osmnodeid_list,
                                 layout.num_entries[DataLayout::COORDINATE_LIST]);
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
        intersection_file.DeserializeVector(bearing_class_id_table);

        const auto bearing_blocks = intersection_file.ReadElementCount32();
        intersection_file.Skip<std::uint32_t>(1); // sum_lengths

        std::vector<unsigned> bearing_offsets_data(bearing_blocks);
        std::vector<typename util::RangeTable<16, true>::BlockT> bearing_blocks_data(
            bearing_blocks);

        intersection_file.ReadInto(bearing_offsets_data.data(), bearing_blocks);
        intersection_file.ReadInto(bearing_blocks_data.data(), bearing_blocks);

        const auto num_bearings = intersection_file.ReadElementCount64();

        std::vector<DiscreteBearing> bearing_class_table(num_bearings);
        intersection_file.ReadInto(bearing_class_table.data(), num_bearings);

        std::vector<util::guidance::EntryClass> entry_class_table;
        intersection_file.DeserializeVector(entry_class_table);

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
                layout.GetBlockPtr<typename util::RangeTable<16, true>::BlockT, true>(
                    memory_ptr, DataLayout::BEARING_BLOCKS);
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
            auto mld_level_data_ptr =
                layout.GetBlockPtr<util::MultiLevelPartition::LevelData, true>(
                    memory_ptr, DataLayout::MLD_LEVEL_DATA);
            auto mld_partition_ptr =
                layout.GetBlockPtr<util::PartitionID, true>(memory_ptr, DataLayout::MLD_PARTITION);
            auto mld_chilren_ptr = layout.GetBlockPtr<util::CellID, true>(
                memory_ptr, DataLayout::MLD_CELL_TO_CHILDREN);

            io::FileReader reader(config.mld_partition_path, io::FileReader::VerifyFingerprint);

            reader.ReadInto(mld_level_data_ptr);
            reader.ReadInto(mld_partition_ptr, layout.GetBlockEntries(DataLayout::MLD_PARTITION));
            reader.ReadInto(mld_chilren_ptr,
                            layout.GetBlockEntries(DataLayout::MLD_CELL_TO_CHILDREN));
        }

        const auto mld_cell_storage_ptr =
            layout.GetBlockPtr<char, true>(memory_ptr, DataLayout::MLD_CELL_STORAGE);
        const auto mld_cell_storage_size = layout.GetBlockSize(DataLayout::MLD_CELL_STORAGE);
        if (boost::filesystem::exists(config.mld_storage_path))
        {
            util::CellStorage<true>().Read(config.mld_storage_path,
                                           mld_cell_storage_ptr,
                                           mld_cell_storage_ptr + mld_cell_storage_size);
        }
    }
}
}
}

#include "storage/storage.hpp"
#include "contractor/query_edge.hpp"
#include "extractor/compressed_edge_container.hpp"
#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/original_edge_data.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/query_node.hpp"
#include "extractor/travel_mode.hpp"
#include "storage/io.hpp"
#include "storage/shared_barriers.hpp"
#include "storage/shared_datatype.hpp"
#include "storage/shared_memory.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "util/coordinate.hpp"
#include "util/exception.hpp"
#include "util/fingerprint.hpp"
#include "util/io.hpp"
#include "util/packed_vector.hpp"
#include "util/range_table.hpp"
#include "util/shared_memory_vector_wrapper.hpp"
#include "util/simple_logger.hpp"
#include "util/static_graph.hpp"
#include "util/static_rtree.hpp"
#include "util/typedefs.hpp"

#ifdef __linux__
#include <sys/mman.h>
#endif

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/sync/named_sharable_mutex.hpp>
#include <boost/interprocess/sync/named_upgradable_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/upgradable_lock.hpp>

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

Storage::Storage(StorageConfig config_) : config(std::move(config_)) {}

struct RegionsLayout
{
    SharedDataType current_layout_region;
    SharedDataType current_data_region;
    boost::interprocess::named_sharable_mutex &current_regions_mutex;
    SharedDataType old_layout_region;
    SharedDataType old_data_region;
    boost::interprocess::named_sharable_mutex &old_regions_mutex;
};

RegionsLayout getRegionsLayout(SharedBarriers &barriers)
{
    if (SharedMemory::RegionExists(CURRENT_REGIONS))
    {
        auto shared_regions = makeSharedMemory(CURRENT_REGIONS);
        const auto shared_timestamp =
            static_cast<const SharedDataTimestamp *>(shared_regions->Ptr());
        if (shared_timestamp->data == DATA_1)
        {
            BOOST_ASSERT(shared_timestamp->layout == LAYOUT_1);
            return RegionsLayout{LAYOUT_1,
                                 DATA_1,
                                 barriers.regions_1_mutex,
                                 LAYOUT_2,
                                 DATA_2,
                                 barriers.regions_2_mutex};
        }

        BOOST_ASSERT(shared_timestamp->data == DATA_2);
        BOOST_ASSERT(shared_timestamp->layout == LAYOUT_2);
    }

    return RegionsLayout{
        LAYOUT_2, DATA_2, barriers.regions_2_mutex, LAYOUT_1, DATA_1, barriers.regions_1_mutex};
}

Storage::ReturnCode Storage::Run(int max_wait)
{
    BOOST_ASSERT_MSG(config.IsValid(), "Invalid storage config");

    util::LogPolicy::GetInstance().Unmute();

    SharedBarriers barriers;

    boost::interprocess::upgradable_lock<boost::interprocess::named_upgradable_mutex>
        current_regions_lock(barriers.current_regions_mutex, boost::interprocess::defer_lock);
    try
    {
        if (!current_regions_lock.try_lock())
        {
            util::SimpleLogger().Write(logWARNING) << "A data update is in progress";
            return ReturnCode::Error;
        }
    }
    // hard unlock in case of any exception.
    catch (boost::interprocess::lock_exception &ex)
    {
        barriers.current_regions_mutex.unlock_upgradable();
        // make sure we exit here because this is bad
        throw;
    }

#ifdef __linux__
    // try to disable swapping on Linux
    const bool lock_flags = MCL_CURRENT | MCL_FUTURE;
    if (-1 == mlockall(lock_flags))
    {
        util::SimpleLogger().Write(logWARNING) << "Could not request RAM lock";
    }
#endif

    auto regions_layout = getRegionsLayout(barriers);
    const SharedDataType layout_region = regions_layout.old_layout_region;
    const SharedDataType data_region = regions_layout.old_data_region;

    if (max_wait > 0)
    {
        util::SimpleLogger().Write() << "Waiting for " << max_wait
                                     << " second for all queries on the old dataset to finish:";
    }
    else
    {
        util::SimpleLogger().Write() << "Waiting for all queries on the old dataset to finish:";
    }

    boost::interprocess::scoped_lock<boost::interprocess::named_sharable_mutex> regions_lock(
        regions_layout.old_regions_mutex, boost::interprocess::defer_lock);

    if (max_wait > 0)
    {
        if (!regions_lock.timed_lock(boost::posix_time::microsec_clock::universal_time() +
                                     boost::posix_time::seconds(max_wait)))
        {
            util::SimpleLogger().Write(logWARNING) << "Queries did not finish in " << max_wait
                                                   << " seconds. Claiming the lock by force.";
            // WARNING: if queries are still using the old dataset they might crash
            if (regions_layout.old_layout_region == LAYOUT_1)
            {
                BOOST_ASSERT(regions_layout.old_data_region == DATA_1);
                barriers.resetRegions1();
            }
            else
            {
                BOOST_ASSERT(regions_layout.old_layout_region == LAYOUT_2);
                BOOST_ASSERT(regions_layout.old_data_region == DATA_2);
                barriers.resetRegions2();
            }

            return ReturnCode::Retry;
        }
    }
    else
    {
        regions_lock.lock();
    }
    util::SimpleLogger().Write() << "Ok.";

    // since we can't change the size of a shared memory regions we delete and reallocate
    if (SharedMemory::RegionExists(layout_region) && !SharedMemory::Remove(layout_region))
    {
        throw util::exception("Could not remove " + regionToString(layout_region));
    }
    if (SharedMemory::RegionExists(data_region) && !SharedMemory::Remove(data_region))
    {
        throw util::exception("Could not remove " + regionToString(data_region));
    }

    // Allocate a memory layout in shared memory
    auto layout_memory = makeSharedMemory(layout_region, sizeof(DataLayout), true);
    auto shared_layout_ptr = new (layout_memory->Ptr()) DataLayout();

    LoadLayout(shared_layout_ptr);

    // allocate shared memory block
    util::SimpleLogger().Write() << "allocating shared memory of "
                                 << shared_layout_ptr->GetSizeOfLayout() << " bytes";
    auto shared_memory = makeSharedMemory(data_region, shared_layout_ptr->GetSizeOfLayout(), true);
    char *shared_memory_ptr = static_cast<char *>(shared_memory->Ptr());

    LoadData(shared_layout_ptr, shared_memory_ptr);

    auto data_type_memory = makeSharedMemory(CURRENT_REGIONS, sizeof(SharedDataTimestamp), true);
    SharedDataTimestamp *data_timestamp_ptr =
        static_cast<SharedDataTimestamp *>(data_type_memory->Ptr());

    {

        boost::interprocess::scoped_lock<boost::interprocess::named_upgradable_mutex>
            current_regions_exclusive_lock;

        if (max_wait > 0)
        {
            util::SimpleLogger().Write() << "Waiting for " << max_wait
                                         << " seconds to write new dataset timestamp";
            auto end_time = boost::posix_time::microsec_clock::universal_time() +
                            boost::posix_time::seconds(max_wait);
            current_regions_exclusive_lock =
                boost::interprocess::scoped_lock<boost::interprocess::named_upgradable_mutex>(
                    std::move(current_regions_lock), end_time);

            if (!current_regions_exclusive_lock.owns())
            {
                util::SimpleLogger().Write(logWARNING) << "Aquiring the lock timed out after "
                                                       << max_wait
                                                       << " seconds. Claiming the lock by force.";
                current_regions_lock.unlock();
                current_regions_lock.release();
                storage::SharedBarriers::resetCurrentRegions();
                return ReturnCode::Retry;
            }
        }
        else
        {
            util::SimpleLogger().Write() << "Waiting to write new dataset timestamp";
            current_regions_exclusive_lock =
                boost::interprocess::scoped_lock<boost::interprocess::named_upgradable_mutex>(
                    std::move(current_regions_lock));
        }

        util::SimpleLogger().Write() << "Ok.";
        data_timestamp_ptr->layout = layout_region;
        data_timestamp_ptr->data = data_region;
        data_timestamp_ptr->timestamp += 1;
    }
    util::SimpleLogger().Write() << "All data loaded.";

    return ReturnCode::Ok;
}

/**
 * This function examines all our data files and figures out how much
 * memory needs to be allocated, and the position of each data structure
 * in that big block.  It updates the fields in the DataLayout parameter.
 */
void Storage::LoadLayout(DataLayout *layout_ptr)
{
    {
        auto absolute_file_index_path = boost::filesystem::absolute(config.file_index_path);

        layout_ptr->SetBlockSize<char>(DataLayout::FILE_INDEX_PATH,
                                       absolute_file_index_path.string().length() + 1);
    }

    {
        // collect number of elements to store in shared memory object
        util::SimpleLogger().Write() << "load names from: " << config.names_data_path;
        // number of entries in name index
        io::FileReader name_file(config.names_data_path);

        const auto name_blocks = name_file.ReadElementCount32();
        layout_ptr->SetBlockSize<unsigned>(DataLayout::NAME_OFFSETS, name_blocks);
        layout_ptr->SetBlockSize<typename util::RangeTable<16, true>::BlockT>(
            DataLayout::NAME_BLOCKS, name_blocks);
        BOOST_ASSERT_MSG(0 != name_blocks, "name file broken");

        const auto number_of_chars = name_file.ReadElementCount32();
        layout_ptr->SetBlockSize<char>(DataLayout::NAME_CHAR_LIST, number_of_chars);
    }

    {
        std::vector<std::uint32_t> lane_description_offsets;
        std::vector<extractor::guidance::TurnLaneType::Mask> lane_description_masks;
        if (!util::deserializeAdjacencyArray(config.turn_lane_description_path.string(),
                                             lane_description_offsets,
                                             lane_description_masks))
            throw util::exception("Failed to read lane descriptions from: " +
                                  config.turn_lane_description_path.string());
        layout_ptr->SetBlockSize<std::uint32_t>(DataLayout::LANE_DESCRIPTION_OFFSETS,
                                                lane_description_offsets.size());
        layout_ptr->SetBlockSize<extractor::guidance::TurnLaneType::Mask>(
            DataLayout::LANE_DESCRIPTION_MASKS, lane_description_masks.size());
    }

    // Loading information for original edges
    {
        io::FileReader edges_file(config.edges_data_path);
        const auto number_of_original_edges = edges_file.ReadElementCount64();

        // note: settings this all to the same size is correct, we extract them from the same struct
        layout_ptr->SetBlockSize<NodeID>(DataLayout::VIA_NODE_LIST, number_of_original_edges);
        layout_ptr->SetBlockSize<unsigned>(DataLayout::NAME_ID_LIST, number_of_original_edges);
        layout_ptr->SetBlockSize<extractor::TravelMode>(DataLayout::TRAVEL_MODE,
                                                        number_of_original_edges);
        layout_ptr->SetBlockSize<util::guidance::TurnBearing>(DataLayout::PRE_TURN_BEARING,
                                                              number_of_original_edges);
        layout_ptr->SetBlockSize<util::guidance::TurnBearing>(DataLayout::POST_TURN_BEARING,
                                                              number_of_original_edges);
        layout_ptr->SetBlockSize<extractor::guidance::TurnInstruction>(DataLayout::TURN_INSTRUCTION,
                                                                       number_of_original_edges);
        layout_ptr->SetBlockSize<LaneDataID>(DataLayout::LANE_DATA_ID, number_of_original_edges);
        layout_ptr->SetBlockSize<EntryClassID>(DataLayout::ENTRY_CLASSID, number_of_original_edges);
    }

    {
        io::FileReader hsgr_file(config.hsgr_data_path);

        const auto hsgr_header = io::readHSGRHeader(hsgr_file);
        layout_ptr->SetBlockSize<unsigned>(DataLayout::HSGR_CHECKSUM, 1);
        layout_ptr->SetBlockSize<QueryGraph::NodeArrayEntry>(DataLayout::GRAPH_NODE_LIST,
                                                             hsgr_header.number_of_nodes);
        layout_ptr->SetBlockSize<QueryGraph::EdgeArrayEntry>(DataLayout::GRAPH_EDGE_LIST,
                                                             hsgr_header.number_of_edges);
    }

    // load rsearch tree size
    {
        io::FileReader tree_node_file(config.ram_index_path);

        const auto tree_size = tree_node_file.ReadElementCount64();
        layout_ptr->SetBlockSize<RTreeNode>(DataLayout::R_SEARCH_TREE, tree_size);
    }

    {
        // allocate space in shared memory for profile properties
        const auto properties_size = io::readPropertiesCount();
        layout_ptr->SetBlockSize<extractor::ProfileProperties>(DataLayout::PROPERTIES,
                                                               properties_size);
    }

    // read timestampsize
    {
        io::FileReader timestamp_file(config.timestamp_path);
        const auto timestamp_size = timestamp_file.Size();
        layout_ptr->SetBlockSize<char>(DataLayout::TIMESTAMP, timestamp_size);
    }

    // load core marker size
    {
        io::FileReader core_marker_file(config.core_data_path);
        const auto number_of_core_markers = core_marker_file.ReadElementCount32();
        layout_ptr->SetBlockSize<unsigned>(DataLayout::CORE_MARKER, number_of_core_markers);
    }

    // load coordinate size
    {
        io::FileReader node_file(config.nodes_data_path);
        const auto coordinate_list_size = node_file.ReadElementCount64();
        layout_ptr->SetBlockSize<util::Coordinate>(DataLayout::COORDINATE_LIST,
                                                   coordinate_list_size);
        // we'll read a list of OSM node IDs from the same data, so set the block size for the same
        // number of items:
        layout_ptr->SetBlockSize<std::uint64_t>(
            DataLayout::OSM_NODE_ID_LIST,
            util::PackedVector<OSMNodeID>::elements_to_blocks(coordinate_list_size));
    }

    // load geometries sizes
    {
        io::FileReader geometry_file(config.geometries_path);

        const auto number_of_geometries_indices = geometry_file.ReadElementCount32();
        layout_ptr->SetBlockSize<unsigned>(DataLayout::GEOMETRIES_INDEX,
                                           number_of_geometries_indices);

        geometry_file.Skip<unsigned>(number_of_geometries_indices);

        const auto number_of_compressed_geometries = geometry_file.ReadElementCount32();
        layout_ptr->SetBlockSize<NodeID>(DataLayout::GEOMETRIES_NODE_LIST,
                                         number_of_compressed_geometries);
        layout_ptr->SetBlockSize<EdgeWeight>(DataLayout::GEOMETRIES_FWD_WEIGHT_LIST,
                                             number_of_compressed_geometries);
        layout_ptr->SetBlockSize<EdgeWeight>(DataLayout::GEOMETRIES_REV_WEIGHT_LIST,
                                             number_of_compressed_geometries);
    }

    // load datasource sizes.  This file is optional, and it's non-fatal if it doesn't
    // exist.
    {
        io::FileReader geometry_datasource_file(config.datasource_indexes_path);
        const auto number_of_compressed_datasources = geometry_datasource_file.ReadElementCount64();
        layout_ptr->SetBlockSize<uint8_t>(DataLayout::DATASOURCES_LIST,
                                          number_of_compressed_datasources);
    }

    // Load datasource name sizes.  This file is optional, and it's non-fatal if it doesn't
    // exist
    {
        io::FileReader datasource_names_file(config.datasource_names_path);

        const io::DatasourceNamesData datasource_names_data =
            io::readDatasourceNames(datasource_names_file);

        layout_ptr->SetBlockSize<char>(DataLayout::DATASOURCE_NAME_DATA,
                                       datasource_names_data.names.size());
        layout_ptr->SetBlockSize<std::size_t>(DataLayout::DATASOURCE_NAME_OFFSETS,
                                              datasource_names_data.offsets.size());
        layout_ptr->SetBlockSize<std::size_t>(DataLayout::DATASOURCE_NAME_LENGTHS,
                                              datasource_names_data.lengths.size());
    }

    {
        io::FileReader intersection_file(config.intersection_class_path, true);

        std::vector<BearingClassID> bearing_class_id_table;
        intersection_file.DeserializeVector(bearing_class_id_table);

        layout_ptr->SetBlockSize<BearingClassID>(DataLayout::BEARING_CLASSID,
                                                 bearing_class_id_table.size());

        const auto bearing_blocks = intersection_file.ReadElementCount32();
        intersection_file.Skip<std::uint32_t>(1); // sum_lengths

        layout_ptr->SetBlockSize<unsigned>(DataLayout::BEARING_OFFSETS, bearing_blocks);
        layout_ptr->SetBlockSize<typename util::RangeTable<16, true>::BlockT>(
            DataLayout::BEARING_BLOCKS, bearing_blocks);

        // No need to read the data
        intersection_file.Skip<unsigned>(bearing_blocks);
        intersection_file.Skip<typename util::RangeTable<16, true>::BlockT>(bearing_blocks);

        const auto num_bearings = intersection_file.ReadElementCount64();

        // Skip over the actual data
        intersection_file.Skip<DiscreteBearing>(num_bearings);

        layout_ptr->SetBlockSize<DiscreteBearing>(DataLayout::BEARING_VALUES, num_bearings);

        std::vector<util::guidance::EntryClass> entry_class_table;
        intersection_file.DeserializeVector(entry_class_table);

        layout_ptr->SetBlockSize<util::guidance::EntryClass>(DataLayout::ENTRY_CLASS,
                                                             entry_class_table.size());
    }

    {
        // Loading turn lane data
        io::FileReader lane_data_file(config.turn_lane_data_path);
        const auto lane_tuple_count = lane_data_file.ReadElementCount64();
        layout_ptr->SetBlockSize<util::guidance::LaneTupleIdPair>(DataLayout::TURN_LANE_DATA,
                                                                  lane_tuple_count);
    }
}

void Storage::LoadData(DataLayout *layout_ptr, char *memory_ptr)
{

    // read actual data into shared memory object //

    // Load the HSGR file
    {
        io::FileReader hsgr_file(config.hsgr_data_path);
        auto hsgr_header = io::readHSGRHeader(hsgr_file);
        unsigned *checksum_ptr =
            layout_ptr->GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::HSGR_CHECKSUM);
        *checksum_ptr = hsgr_header.checksum;

        // load the nodes of the search graph
        QueryGraph::NodeArrayEntry *graph_node_list_ptr =
            layout_ptr->GetBlockPtr<QueryGraph::NodeArrayEntry, true>(memory_ptr,
                                                                      DataLayout::GRAPH_NODE_LIST);

        // load the edges of the search graph
        QueryGraph::EdgeArrayEntry *graph_edge_list_ptr =
            layout_ptr->GetBlockPtr<QueryGraph::EdgeArrayEntry, true>(memory_ptr,
                                                                      DataLayout::GRAPH_EDGE_LIST);

        io::readHSGR(hsgr_file,
                     graph_node_list_ptr,
                     hsgr_header.number_of_nodes,
                     graph_edge_list_ptr,
                     hsgr_header.number_of_edges);
    }

    // store the filename of the on-disk portion of the RTree
    {
        const auto file_index_path_ptr =
            layout_ptr->GetBlockPtr<char, true>(memory_ptr, DataLayout::FILE_INDEX_PATH);
        // make sure we have 0 ending
        std::fill(file_index_path_ptr,
                  file_index_path_ptr + layout_ptr->GetBlockSize(DataLayout::FILE_INDEX_PATH),
                  0);
        auto absolute_file_index_path = boost::filesystem::absolute(config.file_index_path);
        std::copy(absolute_file_index_path.string().begin(),
                  absolute_file_index_path.string().end(),
                  file_index_path_ptr);
    }

    // Name data
    {
        io::FileReader name_file(config.names_data_path);
        const auto name_blocks_count = name_file.ReadElementCount32();
        const auto name_char_list_count = name_file.ReadElementCount32();

        using NameRangeTable = util::RangeTable<16, true>;

        BOOST_ASSERT(name_blocks_count * sizeof(unsigned) ==
                     layout_ptr->GetBlockSize(DataLayout::NAME_OFFSETS));
        BOOST_ASSERT(name_blocks_count * sizeof(typename NameRangeTable::BlockT) ==
                     layout_ptr->GetBlockSize(DataLayout::NAME_BLOCKS));

        // Loading street names
        const auto name_offsets_ptr =
            layout_ptr->GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::NAME_OFFSETS);
        name_file.ReadInto(name_offsets_ptr, name_blocks_count);

        const auto name_blocks_ptr =
            layout_ptr->GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::NAME_BLOCKS);
        name_file.ReadInto(reinterpret_cast<char *>(name_blocks_ptr),
                           layout_ptr->GetBlockSize(DataLayout::NAME_BLOCKS));

        // The file format contains the element count a second time.  Don't know why,
        // but we need to read it here to progress the file pointer to the correct spot
        const auto temp_count = name_file.ReadElementCount32();

        const auto name_char_ptr =
            layout_ptr->GetBlockPtr<char, true>(memory_ptr, DataLayout::NAME_CHAR_LIST);

        BOOST_ASSERT_MSG(layout_ptr->AlignBlockSize(temp_count) ==
                             layout_ptr->GetBlockSize(DataLayout::NAME_CHAR_LIST),
                         "Name file corrupted!");

        name_file.ReadInto(name_char_ptr, temp_count);
    }

    // Turn lane data
    {
        io::FileReader lane_data_file(config.turn_lane_data_path);

        const auto lane_tuple_count = lane_data_file.ReadElementCount64();

        // Need to call GetBlockPtr -> it write the memory canary, even if no data needs to be
        // loaded.
        const auto turn_lane_data_ptr =
            layout_ptr->GetBlockPtr<util::guidance::LaneTupleIdPair, true>(
                memory_ptr, DataLayout::TURN_LANE_DATA);
        BOOST_ASSERT(lane_tuple_count * sizeof(util::guidance::LaneTupleIdPair) ==
                     layout_ptr->GetBlockSize(DataLayout::TURN_LANE_DATA));
        lane_data_file.ReadInto(turn_lane_data_ptr, lane_tuple_count);
    }

    // Turn lane descriptions
    {
        /* NOTE: file io - refactor this in the future */
        std::vector<std::uint32_t> lane_description_offsets;
        std::vector<extractor::guidance::TurnLaneType::Mask> lane_description_masks;
        if (!util::deserializeAdjacencyArray(config.turn_lane_description_path.string(),
                                             lane_description_offsets,
                                             lane_description_masks))
            throw util::exception("Failed to read lane descriptions from: " +
                                  config.turn_lane_description_path.string());
        /* END NOTE */
        const auto turn_lane_offset_ptr = layout_ptr->GetBlockPtr<std::uint32_t, true>(
            memory_ptr, DataLayout::LANE_DESCRIPTION_OFFSETS);
        if (!lane_description_offsets.empty())
        {
            BOOST_ASSERT(layout_ptr->GetBlockSize(DataLayout::LANE_DESCRIPTION_OFFSETS) >=
                         sizeof(lane_description_offsets[0]) * lane_description_offsets.size());
            std::copy(lane_description_offsets.begin(),
                      lane_description_offsets.end(),
                      turn_lane_offset_ptr);
        }

        const auto turn_lane_mask_ptr =
            layout_ptr->GetBlockPtr<extractor::guidance::TurnLaneType::Mask, true>(
                memory_ptr, DataLayout::LANE_DESCRIPTION_MASKS);
        if (!lane_description_masks.empty())
        {
            BOOST_ASSERT(layout_ptr->GetBlockSize(DataLayout::LANE_DESCRIPTION_MASKS) >=
                         sizeof(lane_description_masks[0]) * lane_description_masks.size());
            std::copy(
                lane_description_masks.begin(), lane_description_masks.end(), turn_lane_mask_ptr);
        }
    }

    // Load original edge data
    {
        io::FileReader edges_input_file(config.edges_data_path);

        const auto number_of_original_edges = edges_input_file.ReadElementCount64();

        const auto via_geometry_ptr =
            layout_ptr->GetBlockPtr<GeometryID, true>(memory_ptr, DataLayout::VIA_NODE_LIST);

        const auto name_id_ptr =
            layout_ptr->GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::NAME_ID_LIST);

        const auto travel_mode_ptr = layout_ptr->GetBlockPtr<extractor::TravelMode, true>(
            memory_ptr, DataLayout::TRAVEL_MODE);
        const auto pre_turn_bearing_ptr =
            layout_ptr->GetBlockPtr<util::guidance::TurnBearing, true>(
                memory_ptr, DataLayout::PRE_TURN_BEARING);
        const auto post_turn_bearing_ptr =
            layout_ptr->GetBlockPtr<util::guidance::TurnBearing, true>(
                memory_ptr, DataLayout::POST_TURN_BEARING);

        const auto lane_data_id_ptr =
            layout_ptr->GetBlockPtr<LaneDataID, true>(memory_ptr, DataLayout::LANE_DATA_ID);

        const auto turn_instructions_ptr =
            layout_ptr->GetBlockPtr<extractor::guidance::TurnInstruction, true>(
                memory_ptr, DataLayout::TURN_INSTRUCTION);

        const auto entry_class_id_ptr =
            layout_ptr->GetBlockPtr<EntryClassID, true>(memory_ptr, DataLayout::ENTRY_CLASSID);

        io::readEdges(edges_input_file,
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
        io::FileReader geometry_input_file(config.geometries_path);

        const auto geometry_index_count = geometry_input_file.ReadElementCount32();
        const auto geometries_index_ptr =
            layout_ptr->GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::GEOMETRIES_INDEX);
        BOOST_ASSERT(geometry_index_count == layout_ptr->num_entries[DataLayout::GEOMETRIES_INDEX]);
        geometry_input_file.ReadInto(geometries_index_ptr, geometry_index_count);

        const auto geometries_node_id_list_ptr =
            layout_ptr->GetBlockPtr<NodeID, true>(memory_ptr, DataLayout::GEOMETRIES_NODE_LIST);
        const auto geometry_node_lists_count = geometry_input_file.ReadElementCount32();
        BOOST_ASSERT(geometry_node_lists_count ==
                     layout_ptr->num_entries[DataLayout::GEOMETRIES_NODE_LIST]);
        geometry_input_file.ReadInto(geometries_node_id_list_ptr, geometry_node_lists_count);

        const auto geometries_fwd_weight_list_ptr = layout_ptr->GetBlockPtr<EdgeWeight, true>(
            memory_ptr, DataLayout::GEOMETRIES_FWD_WEIGHT_LIST);
        BOOST_ASSERT(geometry_node_lists_count ==
                     layout_ptr->num_entries[DataLayout::GEOMETRIES_FWD_WEIGHT_LIST]);
        geometry_input_file.ReadInto(geometries_fwd_weight_list_ptr, geometry_node_lists_count);

        const auto geometries_rev_weight_list_ptr = layout_ptr->GetBlockPtr<EdgeWeight, true>(
            memory_ptr, DataLayout::GEOMETRIES_REV_WEIGHT_LIST);
        BOOST_ASSERT(geometry_node_lists_count ==
                     layout_ptr->num_entries[DataLayout::GEOMETRIES_REV_WEIGHT_LIST]);
        geometry_input_file.ReadInto(geometries_rev_weight_list_ptr, geometry_node_lists_count);
    }

    {
        io::FileReader geometry_datasource_file(config.datasource_indexes_path);
        const auto number_of_compressed_datasources = geometry_datasource_file.ReadElementCount64();

        // load datasource information (if it exists)
        const auto datasources_list_ptr =
            layout_ptr->GetBlockPtr<uint8_t, true>(memory_ptr, DataLayout::DATASOURCES_LIST);
        if (number_of_compressed_datasources > 0)
        {
            io::readDatasourceIndexes(
                geometry_datasource_file, datasources_list_ptr, number_of_compressed_datasources);
        }
    }

    {
        /* Load names */
        io::FileReader datasource_names_file(config.datasource_names_path);

        const auto datasource_names_data = io::readDatasourceNames(datasource_names_file);

        // load datasource name information (if it exists)
        const auto datasource_name_data_ptr =
            layout_ptr->GetBlockPtr<char, true>(memory_ptr, DataLayout::DATASOURCE_NAME_DATA);
        if (layout_ptr->GetBlockSize(DataLayout::DATASOURCE_NAME_DATA) > 0)
        {
            std::copy(datasource_names_data.names.begin(),
                      datasource_names_data.names.end(),
                      datasource_name_data_ptr);
        }

        const auto datasource_name_offsets_ptr = layout_ptr->GetBlockPtr<std::size_t, true>(
            memory_ptr, DataLayout::DATASOURCE_NAME_OFFSETS);
        if (layout_ptr->GetBlockSize(DataLayout::DATASOURCE_NAME_OFFSETS) > 0)
        {
            std::copy(datasource_names_data.offsets.begin(),
                      datasource_names_data.offsets.end(),
                      datasource_name_offsets_ptr);
        }

        const auto datasource_name_lengths_ptr = layout_ptr->GetBlockPtr<std::size_t, true>(
            memory_ptr, DataLayout::DATASOURCE_NAME_LENGTHS);
        if (layout_ptr->GetBlockSize(DataLayout::DATASOURCE_NAME_LENGTHS) > 0)
        {
            std::copy(datasource_names_data.lengths.begin(),
                      datasource_names_data.lengths.end(),
                      datasource_name_lengths_ptr);
        }
    }

    // Loading list of coordinates
    {
        io::FileReader nodes_file(config.nodes_data_path);
        nodes_file.Skip<std::uint64_t>(1); // node_count
        const auto coordinates_ptr = layout_ptr->GetBlockPtr<util::Coordinate, true>(
            memory_ptr, DataLayout::COORDINATE_LIST);
        const auto osmnodeid_ptr =
            layout_ptr->GetBlockPtr<std::uint64_t, true>(memory_ptr, DataLayout::OSM_NODE_ID_LIST);
        util::PackedVector<OSMNodeID, true> osmnodeid_list;

        osmnodeid_list.reset(osmnodeid_ptr, layout_ptr->num_entries[DataLayout::OSM_NODE_ID_LIST]);

        io::readNodes(nodes_file,
                      coordinates_ptr,
                      osmnodeid_list,
                      layout_ptr->num_entries[DataLayout::COORDINATE_LIST]);
    }

    // store timestamp
    {
        io::FileReader timestamp_file(config.timestamp_path);
        const auto timestamp_size = timestamp_file.Size();

        const auto timestamp_ptr =
            layout_ptr->GetBlockPtr<char, true>(memory_ptr, DataLayout::TIMESTAMP);
        BOOST_ASSERT(timestamp_size == layout_ptr->num_entries[DataLayout::TIMESTAMP]);
        timestamp_file.ReadInto(timestamp_ptr, timestamp_size);
    }

    // store search tree portion of rtree
    {
        io::FileReader tree_node_file(config.ram_index_path);
        // perform this read so that we're at the right stream position for the next
        // read.
        tree_node_file.Skip<std::uint64_t>(1);
        const auto rtree_ptr =
            layout_ptr->GetBlockPtr<RTreeNode, true>(memory_ptr, DataLayout::R_SEARCH_TREE);

        tree_node_file.ReadInto(rtree_ptr, layout_ptr->num_entries[DataLayout::R_SEARCH_TREE]);
    }

    {
        io::FileReader core_marker_file(config.core_data_path);
        const auto number_of_core_markers = core_marker_file.ReadElementCount32();

        // load core markers
        std::vector<char> unpacked_core_markers(number_of_core_markers);
        core_marker_file.ReadInto(unpacked_core_markers.data(), number_of_core_markers);

        const auto core_marker_ptr =
            layout_ptr->GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::CORE_MARKER);

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
        io::FileReader profile_properties_file(config.properties_path);
        const auto profile_properties_ptr =
            layout_ptr->GetBlockPtr<extractor::ProfileProperties, true>(memory_ptr,
                                                                        DataLayout::PROPERTIES);
        const auto properties_size = io::readPropertiesCount();
        io::readProperties(profile_properties_file, profile_properties_ptr, properties_size);
    }

    // Load intersection data
    {
        io::FileReader intersection_file(config.intersection_class_path, true);

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
            const auto bearing_id_ptr = layout_ptr->GetBlockPtr<BearingClassID, true>(
                memory_ptr, DataLayout::BEARING_CLASSID);
            std::copy(bearing_class_id_table.begin(), bearing_class_id_table.end(), bearing_id_ptr);
        }

        if (layout_ptr->GetBlockSize(DataLayout::BEARING_OFFSETS) > 0)
        {
            const auto bearing_offsets_ptr =
                layout_ptr->GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::BEARING_OFFSETS);
            std::copy(
                bearing_offsets_data.begin(), bearing_offsets_data.end(), bearing_offsets_ptr);
        }

        if (layout_ptr->GetBlockSize(DataLayout::BEARING_BLOCKS) > 0)
        {
            const auto bearing_blocks_ptr =
                layout_ptr->GetBlockPtr<typename util::RangeTable<16, true>::BlockT, true>(
                    memory_ptr, DataLayout::BEARING_BLOCKS);
            std::copy(bearing_blocks_data.begin(), bearing_blocks_data.end(), bearing_blocks_ptr);
        }

        if (!bearing_class_table.empty())
        {
            const auto bearing_class_ptr = layout_ptr->GetBlockPtr<DiscreteBearing, true>(
                memory_ptr, DataLayout::BEARING_VALUES);
            std::copy(bearing_class_table.begin(), bearing_class_table.end(), bearing_class_ptr);
        }

        if (!entry_class_table.empty())
        {
            const auto entry_class_ptr = layout_ptr->GetBlockPtr<util::guidance::EntryClass, true>(
                memory_ptr, DataLayout::ENTRY_CLASS);
            std::copy(entry_class_table.begin(), entry_class_table.end(), entry_class_ptr);
        }
    }
}
}
}

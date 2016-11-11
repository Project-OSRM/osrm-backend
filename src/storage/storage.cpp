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
#include <boost/filesystem/fstream.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/sync/named_sharable_mutex.hpp>
#include <boost/interprocess/sync/named_upgradable_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/upgradable_lock.hpp>
#include <boost/iostreams/seek.hpp>

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
        boost::filesystem::ifstream name_stream(config.names_data_path, std::ios::binary);
        if (!name_stream)
        {
            throw util::exception("Could not open " + config.names_data_path.string() +
                                  " for reading.");
        }
        const auto name_blocks = io::readElementCount32(name_stream);
        layout_ptr->SetBlockSize<unsigned>(DataLayout::NAME_OFFSETS, name_blocks);
        layout_ptr->SetBlockSize<typename util::RangeTable<16, true>::BlockT>(
            DataLayout::NAME_BLOCKS, name_blocks);
        BOOST_ASSERT_MSG(0 != name_blocks, "name file broken");

        const auto number_of_chars = io::readElementCount32(name_stream);
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
        boost::filesystem::ifstream edges_input_stream(config.edges_data_path, std::ios::binary);
        if (!edges_input_stream)
        {
            throw util::exception("Could not open " + config.edges_data_path.string() +
                                  " for reading.");
        }
        const auto number_of_original_edges = io::readElementCount64(edges_input_stream);

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
        boost::filesystem::ifstream hsgr_input_stream(config.hsgr_data_path, std::ios::binary);
        if (!hsgr_input_stream)
        {
            throw util::exception("Could not open " + config.hsgr_data_path.string() +
                                  " for reading.");
        }

        const auto hsgr_header = io::readHSGRHeader(hsgr_input_stream);
        layout_ptr->SetBlockSize<unsigned>(DataLayout::HSGR_CHECKSUM, 1);
        layout_ptr->SetBlockSize<QueryGraph::NodeArrayEntry>(DataLayout::GRAPH_NODE_LIST,
                                                             hsgr_header.number_of_nodes);
        layout_ptr->SetBlockSize<QueryGraph::EdgeArrayEntry>(DataLayout::GRAPH_EDGE_LIST,
                                                             hsgr_header.number_of_edges);
    }

    // load rsearch tree size
    {
        boost::filesystem::ifstream tree_node_file(config.ram_index_path, std::ios::binary);

        const auto tree_size = io::readElementCount64(tree_node_file);
        layout_ptr->SetBlockSize<RTreeNode>(DataLayout::R_SEARCH_TREE, tree_size);

        // allocate space in shared memory for profile properties
        const auto properties_size = io::readPropertiesCount();
        layout_ptr->SetBlockSize<extractor::ProfileProperties>(DataLayout::PROPERTIES,
                                                               properties_size);
    }

    // read timestampsize
    {
        boost::filesystem::ifstream timestamp_stream(config.timestamp_path);
        if (!timestamp_stream)
        {
            throw util::exception("Could not open " + config.timestamp_path.string() +
                                  " for reading.");
        }
        const auto timestamp_size = io::readNumberOfBytes(timestamp_stream);
        layout_ptr->SetBlockSize<char>(DataLayout::TIMESTAMP, timestamp_size);
    }

    // load core marker size
    {
        boost::filesystem::ifstream core_marker_file(config.core_data_path, std::ios::binary);
        if (!core_marker_file)
        {
            throw util::exception("Could not open " + config.core_data_path.string() +
                                  " for reading.");
        }

        const auto number_of_core_markers = io::readElementCount32(core_marker_file);
        layout_ptr->SetBlockSize<unsigned>(DataLayout::CORE_MARKER, number_of_core_markers);
    }

    // load coordinate size
    {
        boost::filesystem::ifstream nodes_input_stream(config.nodes_data_path, std::ios::binary);
        if (!nodes_input_stream)
        {
            throw util::exception("Could not open " + config.core_data_path.string() +
                                  " for reading.");
        }
        const auto coordinate_list_size = io::readElementCount64(nodes_input_stream);
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
        boost::filesystem::ifstream geometry_input_stream(config.geometries_path, std::ios::binary);
        if (!geometry_input_stream)
        {
            throw util::exception("Could not open " + config.geometries_path.string() +
                                  " for reading.");
        }

        const auto number_of_geometries_indices = io::readElementCount32(geometry_input_stream);
        layout_ptr->SetBlockSize<unsigned>(DataLayout::GEOMETRIES_INDEX,
                                           number_of_geometries_indices);
        boost::iostreams::seek(
            geometry_input_stream, number_of_geometries_indices * sizeof(unsigned), BOOST_IOS::cur);

        const auto number_of_compressed_geometries = io::readElementCount32(geometry_input_stream);
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
        boost::filesystem::ifstream geometry_datasource_input_stream(config.datasource_indexes_path,
                                                                     std::ios::binary);
        if (!geometry_datasource_input_stream)
        {
            throw util::exception("Could not open " + config.datasource_indexes_path.string() +
                                  " for reading.");
        }
        const auto number_of_compressed_datasources =
            io::readElementCount64(geometry_datasource_input_stream);
        layout_ptr->SetBlockSize<uint8_t>(DataLayout::DATASOURCES_LIST,
                                          number_of_compressed_datasources);
    }

    // Load datasource name sizes.  This file is optional, and it's non-fatal if it doesn't
    // exist
    {
        boost::filesystem::ifstream datasource_names_input_stream(config.datasource_names_path,
                                                                  std::ios::binary);
        if (!datasource_names_input_stream)
        {
            throw util::exception("Could not open " + config.datasource_names_path.string() +
                                  " for reading.");
        }

        const io::DatasourceNamesData datasource_names_data =
            io::readDatasourceNames(datasource_names_input_stream);

        layout_ptr->SetBlockSize<char>(DataLayout::DATASOURCE_NAME_DATA,
                                       datasource_names_data.names.size());
        layout_ptr->SetBlockSize<std::size_t>(DataLayout::DATASOURCE_NAME_OFFSETS,
                                              datasource_names_data.offsets.size());
        layout_ptr->SetBlockSize<std::size_t>(DataLayout::DATASOURCE_NAME_LENGTHS,
                                              datasource_names_data.lengths.size());
    }

    {
        boost::filesystem::ifstream intersection_stream(config.intersection_class_path,
                                                        std::ios::binary);

        if (!static_cast<bool>(intersection_stream))
            throw util::exception("Could not open " + config.intersection_class_path.string() +
                                  " for reading.");

        if (!util::readAndCheckFingerprint(intersection_stream))
            throw util::exception("Fingerprint of " + config.intersection_class_path.string() +
                                  " does not match or could not read from file");

        std::vector<BearingClassID> bearing_class_id_table;
        if (!util::deserializeVector(intersection_stream, bearing_class_id_table))
            throw util::exception("Failed to bearing class ids read from " +
                                  config.names_data_path.string());

        layout_ptr->SetBlockSize<BearingClassID>(DataLayout::BEARING_CLASSID,
                                                 bearing_class_id_table.size());

        const auto bearing_blocks = io::readElementCount32(intersection_stream);
        const auto sum_lengths = io::readElementCount32(intersection_stream);

        layout_ptr->SetBlockSize<unsigned>(DataLayout::BEARING_OFFSETS, bearing_blocks);
        layout_ptr->SetBlockSize<typename util::RangeTable<16, true>::BlockT>(
            DataLayout::BEARING_BLOCKS, bearing_blocks);

        // Skip the actual data
        boost::iostreams::seek(intersection_stream,
                               bearing_blocks * sizeof(unsigned) +
                                   bearing_blocks *
                                       sizeof(typename util::RangeTable<16, true>::BlockT),
                               BOOST_IOS::cur);

        const auto num_bearings = io::readElementCount64(intersection_stream);

        // Skip over the actual data
        boost::iostreams::seek(
            intersection_stream, num_bearings * sizeof(DiscreteBearing), BOOST_IOS::cur);
        layout_ptr->SetBlockSize<DiscreteBearing>(DataLayout::BEARING_VALUES, num_bearings);

        if (!static_cast<bool>(intersection_stream))
            throw util::exception("Failed to read bearing values from " +
                                  config.intersection_class_path.string());

        std::vector<util::guidance::EntryClass> entry_class_table;
        if (!util::deserializeVector(intersection_stream, entry_class_table))
            throw util::exception("Failed to read entry classes from " +
                                  config.intersection_class_path.string());

        layout_ptr->SetBlockSize<util::guidance::EntryClass>(DataLayout::ENTRY_CLASS,
                                                             entry_class_table.size());
    }

    {
        // Loading turn lane data
        boost::filesystem::ifstream lane_data_stream(config.turn_lane_data_path, std::ios::binary);
        const auto lane_tupel_count = io::readElementCount64(lane_data_stream);
        layout_ptr->SetBlockSize<util::guidance::LaneTupleIdPair>(DataLayout::TURN_LANE_DATA,
                                                                  lane_tupel_count);
    }
}

void Storage::LoadData(DataLayout *layout_ptr, char *memory_ptr)
{

    // read actual data into shared memory object //

    // Load the HSGR file
    {
        boost::filesystem::ifstream hsgr_input_stream(config.hsgr_data_path, std::ios::binary);
        auto hsgr_header = io::readHSGRHeader(hsgr_input_stream);
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

        io::readHSGR(hsgr_input_stream,
                     graph_node_list_ptr,
                     hsgr_header.number_of_nodes,
                     graph_edge_list_ptr,
                     hsgr_header.number_of_edges);
        hsgr_input_stream.close();
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
        /* To be replaced by io loading code */
        boost::filesystem::ifstream name_stream(config.names_data_path, std::ios::binary);
        const auto name_blocks = io::readElementCount32(name_stream);
        const auto number_of_chars = io::readElementCount32(name_stream);
        /* END to be replaced */

        // Loading street names
        const auto name_offsets_ptr =
            layout_ptr->GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::NAME_OFFSETS);
        if (layout_ptr->GetBlockSize(DataLayout::NAME_OFFSETS) > 0)
        {
            name_stream.read(reinterpret_cast<char *>(name_offsets_ptr),
                             layout_ptr->GetBlockSize(DataLayout::NAME_OFFSETS));
        }

        const auto name_blocks_ptr =
            layout_ptr->GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::NAME_BLOCKS);
        if (layout_ptr->GetBlockSize(DataLayout::NAME_BLOCKS) > 0)
        {
            name_stream.read(reinterpret_cast<char *>(name_blocks_ptr),
                             layout_ptr->GetBlockSize(DataLayout::NAME_BLOCKS));
        }

        const auto name_char_ptr =
            layout_ptr->GetBlockPtr<char, true>(memory_ptr, DataLayout::NAME_CHAR_LIST);

        const auto name_char_list_count = io::readElementCount32(name_stream);

        BOOST_ASSERT_MSG(layout_ptr->AlignBlockSize(name_char_list_count) ==
                             layout_ptr->GetBlockSize(DataLayout::NAME_CHAR_LIST),
                         "Name file corrupted!");

        if (layout_ptr->GetBlockSize(DataLayout::NAME_CHAR_LIST) > 0)
        {
            name_stream.read(name_char_ptr, layout_ptr->GetBlockSize(DataLayout::NAME_CHAR_LIST));
        }
    }

    // Turn lane data
    {
        /* NOTE: file io - refactor this in the future */
        boost::filesystem::ifstream lane_data_stream(config.turn_lane_data_path, std::ios::binary);
        const auto lane_tupel_count = io::readElementCount64(lane_data_stream);
        /* END NOTE */

        // Need to call GetBlockPtr -> it write the memory canary, even if no data needs to be
        // loaded.
        const auto turn_lane_data_ptr =
            layout_ptr->GetBlockPtr<util::guidance::LaneTupleIdPair, true>(
                memory_ptr, DataLayout::TURN_LANE_DATA);
        if (layout_ptr->GetBlockSize(DataLayout::TURN_LANE_DATA) > 0)
        {
            lane_data_stream.read(reinterpret_cast<char *>(turn_lane_data_ptr),
                                  layout_ptr->GetBlockSize(DataLayout::TURN_LANE_DATA));
        }
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
        /* NOTE: file io - refactor this in the future */
        boost::filesystem::ifstream edges_input_stream(config.edges_data_path, std::ios::binary);
        if (!edges_input_stream)
        {
            throw util::exception("Could not open " + config.edges_data_path.string() +
                                  " for reading.");
        }

        const auto number_of_original_edges = io::readElementCount64(edges_input_stream);
        /* END NOTE */

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

        io::readEdges(edges_input_stream,
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
        /* NOTE: file io - refactor this in the future */
        boost::filesystem::ifstream geometry_input_stream(config.geometries_path, std::ios::binary);
        /* END NOTE */

        const auto geometry_index_count = io::readElementCount32(geometry_input_stream);
        const auto geometries_index_ptr =
            layout_ptr->GetBlockPtr<unsigned, true>(memory_ptr, DataLayout::GEOMETRIES_INDEX);
        BOOST_ASSERT(geometry_index_count == layout_ptr->num_entries[DataLayout::GEOMETRIES_INDEX]);

        if (layout_ptr->GetBlockSize(DataLayout::GEOMETRIES_INDEX) > 0)
        {
            geometry_input_stream.read(reinterpret_cast<char *>(geometries_index_ptr),
                                       layout_ptr->GetBlockSize(DataLayout::GEOMETRIES_INDEX));
        }
        const auto geometries_node_id_list_ptr =
            layout_ptr->GetBlockPtr<NodeID, true>(memory_ptr, DataLayout::GEOMETRIES_NODE_LIST);

        const auto geometry_node_lists_count = io::readElementCount32(geometry_input_stream);
        BOOST_ASSERT(geometry_node_lists_count ==
                     layout_ptr->num_entries[DataLayout::GEOMETRIES_NODE_LIST]);

        if (layout_ptr->GetBlockSize(DataLayout::GEOMETRIES_NODE_LIST) > 0)
        {
            geometry_input_stream.read(reinterpret_cast<char *>(geometries_node_id_list_ptr),
                                       layout_ptr->GetBlockSize(DataLayout::GEOMETRIES_NODE_LIST));
        }
        const auto geometries_fwd_weight_list_ptr = layout_ptr->GetBlockPtr<EdgeWeight, true>(
            memory_ptr, DataLayout::GEOMETRIES_FWD_WEIGHT_LIST);

        BOOST_ASSERT(geometry_node_lists_count ==
                     layout_ptr->num_entries[DataLayout::GEOMETRIES_FWD_WEIGHT_LIST]);

        if (layout_ptr->GetBlockSize(DataLayout::GEOMETRIES_FWD_WEIGHT_LIST) > 0)
        {
            geometry_input_stream.read(
                reinterpret_cast<char *>(geometries_fwd_weight_list_ptr),
                layout_ptr->GetBlockSize(DataLayout::GEOMETRIES_FWD_WEIGHT_LIST));
        }
        const auto geometries_rev_weight_list_ptr = layout_ptr->GetBlockPtr<EdgeWeight, true>(
            memory_ptr, DataLayout::GEOMETRIES_REV_WEIGHT_LIST);

        BOOST_ASSERT(geometry_node_lists_count ==
                     layout_ptr->num_entries[DataLayout::GEOMETRIES_REV_WEIGHT_LIST]);

        if (layout_ptr->GetBlockSize(DataLayout::GEOMETRIES_REV_WEIGHT_LIST) > 0)
        {
            geometry_input_stream.read(
                reinterpret_cast<char *>(geometries_rev_weight_list_ptr),
                layout_ptr->GetBlockSize(DataLayout::GEOMETRIES_REV_WEIGHT_LIST));
        }
    }

    {
        boost::filesystem::ifstream geometry_datasource_input_stream(config.datasource_indexes_path,
                                                                     std::ios::binary);
        if (!geometry_datasource_input_stream)
        {
            throw util::exception("Could not open " + config.datasource_indexes_path.string() +
                                  " for reading.");
        }
        const auto number_of_compressed_datasources =
            io::readElementCount64(geometry_datasource_input_stream);

        // load datasource information (if it exists)
        const auto datasources_list_ptr =
            layout_ptr->GetBlockPtr<uint8_t, true>(memory_ptr, DataLayout::DATASOURCES_LIST);
        if (number_of_compressed_datasources > 0)
        {
            io::readDatasourceIndexes(geometry_datasource_input_stream,
                                      datasources_list_ptr,
                                      number_of_compressed_datasources);
        }
    }

    {
        /* Load names */
        boost::filesystem::ifstream datasource_names_input_stream(config.datasource_names_path,
                                                                  std::ios::binary);
        if (!datasource_names_input_stream)
        {
            throw util::exception("Could not open " + config.datasource_names_path.string() +
                                  " for reading.");
        }

        const auto datasource_names_data = io::readDatasourceNames(datasource_names_input_stream);

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
        boost::filesystem::ifstream nodes_input_stream(config.nodes_data_path, std::ios::binary);
        io::readElementCount64(nodes_input_stream);
        const auto coordinates_ptr = layout_ptr->GetBlockPtr<util::Coordinate, true>(
            memory_ptr, DataLayout::COORDINATE_LIST);
        const auto osmnodeid_ptr =
            layout_ptr->GetBlockPtr<std::uint64_t, true>(memory_ptr, DataLayout::OSM_NODE_ID_LIST);
        util::PackedVector<OSMNodeID, true> osmnodeid_list;

        osmnodeid_list.reset(osmnodeid_ptr, layout_ptr->num_entries[DataLayout::OSM_NODE_ID_LIST]);

        io::readNodes(nodes_input_stream,
                      coordinates_ptr,
                      osmnodeid_list,
                      layout_ptr->num_entries[DataLayout::COORDINATE_LIST]);
    }

    // store timestamp
    {
        /* NOTE: file io - refactor this in the future */
        boost::filesystem::ifstream timestamp_stream(config.timestamp_path);
        const auto timestamp_size = io::readNumberOfBytes(timestamp_stream);
        /* END NOTE */

        const auto timestamp_ptr =
            layout_ptr->GetBlockPtr<char, true>(memory_ptr, DataLayout::TIMESTAMP);
        io::readTimestamp(timestamp_stream, timestamp_ptr, timestamp_size);
    }

    // store search tree portion of rtree
    {
        boost::filesystem::ifstream tree_node_file(config.ram_index_path, std::ios::binary);
        // perform this read so that we're at the right stream position for the next
        // read.
        io::readElementCount64(tree_node_file);
        const auto rtree_ptr =
            layout_ptr->GetBlockPtr<RTreeNode, true>(memory_ptr, DataLayout::R_SEARCH_TREE);
        io::readRamIndex(
            tree_node_file, rtree_ptr, layout_ptr->num_entries[DataLayout::R_SEARCH_TREE]);
    }

    {
        /* NOTE: file io - refactor this in the future */
        boost::filesystem::ifstream core_marker_file(config.core_data_path, std::ios::binary);
        if (!core_marker_file)
        {
            throw util::exception("Could not open " + config.core_data_path.string() +
                                  " for reading.");
        }

        const auto number_of_core_markers = io::readElementCount32(core_marker_file);
        /* END NOTE */

        // load core markers
        std::vector<char> unpacked_core_markers(number_of_core_markers);
        core_marker_file.read(reinterpret_cast<char *>(unpacked_core_markers.data()),
                              sizeof(char) * number_of_core_markers);

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
        boost::filesystem::ifstream profile_properties_stream(config.properties_path);
        if (!profile_properties_stream)
        {
            util::exception("Could not open " + config.properties_path.string() + " for reading!");
        }
        io::readPropertiesCount();
        const auto profile_properties_ptr =
            layout_ptr->GetBlockPtr<extractor::ProfileProperties, true>(memory_ptr,
                                                                        DataLayout::PROPERTIES);
        io::readProperties(profile_properties_stream,
                           profile_properties_ptr,
                           sizeof(extractor::ProfileProperties));
    }

    // Load intersection data
    {
        boost::filesystem::ifstream intersection_stream(config.intersection_class_path,
                                                        std::ios::binary);
        if (!static_cast<bool>(intersection_stream))
            throw util::exception("Could not open " + config.intersection_class_path.string() +
                                  " for reading.");

        if (!util::readAndCheckFingerprint(intersection_stream))
            throw util::exception("Fingerprint of " + config.intersection_class_path.string() +
                                  " does not match or could not read from file");

        std::vector<BearingClassID> bearing_class_id_table;
        if (!util::deserializeVector(intersection_stream, bearing_class_id_table))
            throw util::exception("Failed to bearing class ids read from " +
                                  config.names_data_path.string());

        const auto bearing_blocks = io::readElementCount32(intersection_stream);
        const auto sum_lengths = io::readElementCount32(intersection_stream);

        std::vector<unsigned> bearing_offsets_data(bearing_blocks);
        std::vector<typename util::RangeTable<16, true>::BlockT> bearing_blocks_data(
            bearing_blocks);

        if (bearing_blocks)
        {
            intersection_stream.read(reinterpret_cast<char *>(bearing_offsets_data.data()),
                                     bearing_blocks *
                                         sizeof(decltype(bearing_offsets_data)::value_type));
        }

        if (bearing_blocks)
        {
            intersection_stream.read(reinterpret_cast<char *>(bearing_blocks_data.data()),
                                     bearing_blocks *
                                         sizeof(decltype(bearing_blocks_data)::value_type));
        }

        const auto num_bearings = io::readElementCount64(intersection_stream);

        std::vector<DiscreteBearing> bearing_class_table(num_bearings);
        intersection_stream.read(reinterpret_cast<char *>(bearing_class_table.data()),
                                 sizeof(decltype(bearing_class_table)::value_type) * num_bearings);

        std::vector<util::guidance::EntryClass> entry_class_table;
        if (!util::deserializeVector(intersection_stream, entry_class_table))
            throw util::exception("Failed to read entry classes from " +
                                  config.intersection_class_path.string());

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

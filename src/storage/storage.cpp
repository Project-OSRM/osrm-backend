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
#include "storage/storage.hpp"
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
    auto layout_memory = makeSharedMemory(layout_region, sizeof(SharedDataLayout), true);
    auto shared_layout_ptr = new (layout_memory->Ptr()) SharedDataLayout();
    auto absolute_file_index_path = boost::filesystem::absolute(config.file_index_path);

    shared_layout_ptr->SetBlockSize<char>(SharedDataLayout::FILE_INDEX_PATH,
                                          absolute_file_index_path.string().length() + 1);

    // collect number of elements to store in shared memory object
    util::SimpleLogger().Write() << "load names from: " << config.names_data_path;
    // number of entries in name index
    boost::filesystem::ifstream name_stream(config.names_data_path, std::ios::binary);
    if (!name_stream)
    {
        throw util::exception("Could not open " + config.names_data_path.string() +
                              " for reading.");
    }
    unsigned name_blocks = 0;
    name_stream.read((char *)&name_blocks, sizeof(unsigned));
    shared_layout_ptr->SetBlockSize<unsigned>(SharedDataLayout::NAME_OFFSETS, name_blocks);
    shared_layout_ptr->SetBlockSize<typename util::RangeTable<16, true>::BlockT>(
        SharedDataLayout::NAME_BLOCKS, name_blocks);
    BOOST_ASSERT_MSG(0 != name_blocks, "name file broken");

    unsigned number_of_chars = 0;
    name_stream.read((char *)&number_of_chars, sizeof(unsigned));
    shared_layout_ptr->SetBlockSize<char>(SharedDataLayout::NAME_CHAR_LIST, number_of_chars);

    std::vector<std::uint32_t> lane_description_offsets;
    std::vector<extractor::guidance::TurnLaneType::Mask> lane_description_masks;
    if (!util::deserializeAdjacencyArray(config.turn_lane_description_path.string(),
                                         lane_description_offsets,
                                         lane_description_masks))
        throw util::exception("Failed to read lane descriptions from: " +
                              config.turn_lane_description_path.string());
    shared_layout_ptr->SetBlockSize<std::uint32_t>(SharedDataLayout::LANE_DESCRIPTION_OFFSETS,
                                                   lane_description_offsets.size());
    shared_layout_ptr->SetBlockSize<extractor::guidance::TurnLaneType::Mask>(
        SharedDataLayout::LANE_DESCRIPTION_MASKS, lane_description_masks.size());

    // Loading information for original edges
    boost::filesystem::ifstream edges_input_stream(config.edges_data_path, std::ios::binary);
    if (!edges_input_stream)
    {
        throw util::exception("Could not open " + config.edges_data_path.string() +
                              " for reading.");
    }
    auto number_of_original_edges = io::readEdgesSize(edges_input_stream);

    // note: settings this all to the same size is correct, we extract them from the same struct
    shared_layout_ptr->SetBlockSize<NodeID>(SharedDataLayout::VIA_NODE_LIST,
                                            number_of_original_edges);
    shared_layout_ptr->SetBlockSize<unsigned>(SharedDataLayout::NAME_ID_LIST,
                                              number_of_original_edges);
    shared_layout_ptr->SetBlockSize<extractor::TravelMode>(SharedDataLayout::TRAVEL_MODE,
                                                           number_of_original_edges);
    shared_layout_ptr->SetBlockSize<util::guidance::TurnBearing>(SharedDataLayout::PRE_TURN_BEARING,
                                                                 number_of_original_edges);
    shared_layout_ptr->SetBlockSize<util::guidance::TurnBearing>(
        SharedDataLayout::POST_TURN_BEARING, number_of_original_edges);
    shared_layout_ptr->SetBlockSize<extractor::guidance::TurnInstruction>(
        SharedDataLayout::TURN_INSTRUCTION, number_of_original_edges);
    shared_layout_ptr->SetBlockSize<LaneDataID>(SharedDataLayout::LANE_DATA_ID,
                                                number_of_original_edges);
    shared_layout_ptr->SetBlockSize<EntryClassID>(SharedDataLayout::ENTRY_CLASSID,
                                                  number_of_original_edges);

    boost::filesystem::ifstream hsgr_input_stream(config.hsgr_data_path, std::ios::binary);
    if (!hsgr_input_stream)
    {
        throw util::exception("Could not open " + config.hsgr_data_path.string() + " for reading.");
    }

    auto hsgr_header = io::readHSGRHeader(hsgr_input_stream);
    shared_layout_ptr->SetBlockSize<unsigned>(SharedDataLayout::HSGR_CHECKSUM, 1);
    shared_layout_ptr->SetBlockSize<QueryGraph::NodeArrayEntry>(SharedDataLayout::GRAPH_NODE_LIST,
                                                                hsgr_header.number_of_nodes);
    shared_layout_ptr->SetBlockSize<QueryGraph::EdgeArrayEntry>(SharedDataLayout::GRAPH_EDGE_LIST,
                                                                hsgr_header.number_of_edges);

    // load rsearch tree size
    boost::filesystem::ifstream tree_node_file(config.ram_index_path, std::ios::binary);

    uint32_t tree_size = 0;
    tree_node_file.read((char *)&tree_size, sizeof(uint32_t));
    shared_layout_ptr->SetBlockSize<RTreeNode>(SharedDataLayout::R_SEARCH_TREE, tree_size);

    // load profile properties
    shared_layout_ptr->SetBlockSize<extractor::ProfileProperties>(SharedDataLayout::PROPERTIES, 1);

    // read timestampsize
    boost::filesystem::ifstream timestamp_stream(config.timestamp_path);
    if (!timestamp_stream)
    {
        throw util::exception("Could not open " + config.timestamp_path.string() + " for reading.");
    }
    std::size_t timestamp_size = io::readTimestampSize(timestamp_stream);
    shared_layout_ptr->SetBlockSize<char>(SharedDataLayout::TIMESTAMP, timestamp_size);

    // load core marker size
    boost::filesystem::ifstream core_marker_file(config.core_data_path, std::ios::binary);
    if (!core_marker_file)
    {
        throw util::exception("Could not open " + config.core_data_path.string() + " for reading.");
    }

    uint32_t number_of_core_markers = 0;
    core_marker_file.read((char *)&number_of_core_markers, sizeof(uint32_t));
    shared_layout_ptr->SetBlockSize<unsigned>(SharedDataLayout::CORE_MARKER,
                                              number_of_core_markers);

    // load coordinate size
    boost::filesystem::ifstream nodes_input_stream(config.nodes_data_path, std::ios::binary);
    if (!nodes_input_stream)
    {
        throw util::exception("Could not open " + config.core_data_path.string() + " for reading.");
    }
    auto coordinate_list_size = io::readNodesSize(nodes_input_stream);
    shared_layout_ptr->SetBlockSize<util::Coordinate>(SharedDataLayout::COORDINATE_LIST,
                                                      coordinate_list_size);
    // we'll read a list of OSM node IDs from the same data, so set the block size for the same
    // number of items:
    shared_layout_ptr->SetBlockSize<std::uint64_t>(
        SharedDataLayout::OSM_NODE_ID_LIST,
        util::PackedVector<OSMNodeID>::elements_to_blocks(coordinate_list_size));

    // load geometries sizes
    boost::filesystem::ifstream geometry_input_stream(config.geometries_path, std::ios::binary);
    if (!geometry_input_stream)
    {
        throw util::exception("Could not open " + config.geometries_path.string() +
                              " for reading.");
    }
    unsigned number_of_geometries_indices = 0;
    unsigned number_of_compressed_geometries = 0;

    geometry_input_stream.read((char *)&number_of_geometries_indices, sizeof(unsigned));
    shared_layout_ptr->SetBlockSize<unsigned>(SharedDataLayout::GEOMETRIES_INDEX,
                                              number_of_geometries_indices);
    boost::iostreams::seek(
        geometry_input_stream, number_of_geometries_indices * sizeof(unsigned), BOOST_IOS::cur);
    geometry_input_stream.read((char *)&number_of_compressed_geometries, sizeof(unsigned));
    shared_layout_ptr->SetBlockSize<NodeID>(SharedDataLayout::GEOMETRIES_NODE_LIST,
                                            number_of_compressed_geometries);
    shared_layout_ptr->SetBlockSize<EdgeWeight>(SharedDataLayout::GEOMETRIES_FWD_WEIGHT_LIST,
                                                number_of_compressed_geometries);
    shared_layout_ptr->SetBlockSize<EdgeWeight>(SharedDataLayout::GEOMETRIES_REV_WEIGHT_LIST,
                                                number_of_compressed_geometries);

    // load datasource sizes.  This file is optional, and it's non-fatal if it doesn't
    // exist.
    boost::filesystem::ifstream geometry_datasource_input_stream(config.datasource_indexes_path,
                                                                 std::ios::binary);
    if (!geometry_datasource_input_stream)
    {
        throw util::exception("Could not open " + config.datasource_indexes_path.string() +
                              " for reading.");
    }
    std::uint64_t number_of_compressed_datasources = 0;
    if (geometry_datasource_input_stream)
    {
        geometry_datasource_input_stream.read(
            reinterpret_cast<char *>(&number_of_compressed_datasources),
            sizeof(number_of_compressed_datasources));
    }
    shared_layout_ptr->SetBlockSize<uint8_t>(SharedDataLayout::DATASOURCES_LIST,
                                             number_of_compressed_datasources);

    // Load datasource name sizes.  This file is optional, and it's non-fatal if it doesn't
    // exist
    boost::filesystem::ifstream datasource_names_input_stream(config.datasource_names_path,
                                                              std::ios::binary);
    if (!datasource_names_input_stream)
    {
        throw util::exception("Could not open " + config.datasource_names_path.string() +
                              " for reading.");
    }
    std::vector<char> m_datasource_name_data;
    std::vector<std::size_t> m_datasource_name_offsets;
    std::vector<std::size_t> m_datasource_name_lengths;
    if (datasource_names_input_stream)
    {
        std::string name;
        while (std::getline(datasource_names_input_stream, name))
        {
            m_datasource_name_offsets.push_back(m_datasource_name_data.size());
            std::copy(name.c_str(),
                      name.c_str() + name.size(),
                      std::back_inserter(m_datasource_name_data));
            m_datasource_name_lengths.push_back(name.size());
        }
    }
    shared_layout_ptr->SetBlockSize<char>(SharedDataLayout::DATASOURCE_NAME_DATA,
                                          m_datasource_name_data.size());
    shared_layout_ptr->SetBlockSize<std::size_t>(SharedDataLayout::DATASOURCE_NAME_OFFSETS,
                                                 m_datasource_name_offsets.size());
    shared_layout_ptr->SetBlockSize<std::size_t>(SharedDataLayout::DATASOURCE_NAME_LENGTHS,
                                                 m_datasource_name_lengths.size());

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

    shared_layout_ptr->SetBlockSize<BearingClassID>(SharedDataLayout::BEARING_CLASSID,
                                                    bearing_class_id_table.size());
    unsigned bearing_blocks = 0;
    intersection_stream.read((char *)&bearing_blocks, sizeof(unsigned));
    unsigned sum_lengths = 0;
    intersection_stream.read((char *)&sum_lengths, sizeof(unsigned));

    shared_layout_ptr->SetBlockSize<unsigned>(SharedDataLayout::BEARING_OFFSETS, bearing_blocks);
    shared_layout_ptr->SetBlockSize<typename util::RangeTable<16, true>::BlockT>(
        SharedDataLayout::BEARING_BLOCKS, bearing_blocks);

    std::vector<unsigned> bearing_offsets_data(bearing_blocks);
    std::vector<typename util::RangeTable<16, true>::BlockT> bearing_blocks_data(bearing_blocks);

    if (bearing_blocks)
    {
        intersection_stream.read(reinterpret_cast<char *>(&bearing_offsets_data[0]),
                                 bearing_blocks * sizeof(bearing_offsets_data[0]));
    }

    if (bearing_blocks)
    {
        intersection_stream.read(reinterpret_cast<char *>(&bearing_blocks_data[0]),
                                 bearing_blocks * sizeof(bearing_blocks_data[0]));
    }

    std::uint64_t num_bearings;
    intersection_stream.read(reinterpret_cast<char *>(&num_bearings), sizeof(num_bearings));

    std::vector<DiscreteBearing> bearing_class_table(num_bearings);
    intersection_stream.read(reinterpret_cast<char *>(&bearing_class_table[0]),
                             sizeof(bearing_class_table[0]) * num_bearings);
    shared_layout_ptr->SetBlockSize<DiscreteBearing>(SharedDataLayout::BEARING_VALUES,
                                                     num_bearings);

    // Loading turn lane data
    boost::filesystem::ifstream lane_data_stream(config.turn_lane_data_path, std::ios::binary);
    std::uint64_t lane_tupel_count = 0;
    lane_data_stream.read(reinterpret_cast<char *>(&lane_tupel_count), sizeof(lane_tupel_count));
    shared_layout_ptr->SetBlockSize<util::guidance::LaneTupleIdPair>(
        SharedDataLayout::TURN_LANE_DATA, lane_tupel_count);

    if (!static_cast<bool>(intersection_stream))
        throw util::exception("Failed to read bearing values from " +
                              config.intersection_class_path.string());

    std::vector<util::guidance::EntryClass> entry_class_table;
    if (!util::deserializeVector(intersection_stream, entry_class_table))
        throw util::exception("Failed to read entry classes from " +
                              config.intersection_class_path.string());

    shared_layout_ptr->SetBlockSize<util::guidance::EntryClass>(SharedDataLayout::ENTRY_CLASS,
                                                                entry_class_table.size());

    // allocate shared memory block
    util::SimpleLogger().Write() << "allocating shared memory of "
                                 << shared_layout_ptr->GetSizeOfLayout() << " bytes";
    auto shared_memory = makeSharedMemory(data_region, shared_layout_ptr->GetSizeOfLayout(), true);
    char *shared_memory_ptr = static_cast<char *>(shared_memory->Ptr());

    // read actual data into shared memory object //

    // hsgr checksum
    unsigned *checksum_ptr = shared_layout_ptr->GetBlockPtr<unsigned, true>(
        shared_memory_ptr, SharedDataLayout::HSGR_CHECKSUM);
    *checksum_ptr = hsgr_header.checksum;

    // ram index file name
    char *file_index_path_ptr = shared_layout_ptr->GetBlockPtr<char, true>(
        shared_memory_ptr, SharedDataLayout::FILE_INDEX_PATH);
    // make sure we have 0 ending
    std::fill(file_index_path_ptr,
              file_index_path_ptr +
                  shared_layout_ptr->GetBlockSize(SharedDataLayout::FILE_INDEX_PATH),
              0);
    std::copy(absolute_file_index_path.string().begin(),
              absolute_file_index_path.string().end(),
              file_index_path_ptr);

    // Loading street names
    unsigned *name_offsets_ptr = shared_layout_ptr->GetBlockPtr<unsigned, true>(
        shared_memory_ptr, SharedDataLayout::NAME_OFFSETS);
    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::NAME_OFFSETS) > 0)
    {
        name_stream.read((char *)name_offsets_ptr,
                         shared_layout_ptr->GetBlockSize(SharedDataLayout::NAME_OFFSETS));
    }

    unsigned *name_blocks_ptr = shared_layout_ptr->GetBlockPtr<unsigned, true>(
        shared_memory_ptr, SharedDataLayout::NAME_BLOCKS);
    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::NAME_BLOCKS) > 0)
    {
        name_stream.read((char *)name_blocks_ptr,
                         shared_layout_ptr->GetBlockSize(SharedDataLayout::NAME_BLOCKS));
    }

    char *name_char_ptr = shared_layout_ptr->GetBlockPtr<char, true>(
        shared_memory_ptr, SharedDataLayout::NAME_CHAR_LIST);
    unsigned temp_length = 0;
    name_stream.read((char *)&temp_length, sizeof(unsigned));

    BOOST_ASSERT_MSG(shared_layout_ptr->AlignBlockSize(temp_length) ==
                         shared_layout_ptr->GetBlockSize(SharedDataLayout::NAME_CHAR_LIST),
                     "Name file corrupted!");

    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::NAME_CHAR_LIST) > 0)
    {
        name_stream.read(name_char_ptr,
                         shared_layout_ptr->GetBlockSize(SharedDataLayout::NAME_CHAR_LIST));
    }
    name_stream.close();

    // make sure do write canary...
    auto *turn_lane_data_ptr =
        shared_layout_ptr->GetBlockPtr<util::guidance::LaneTupleIdPair, true>(
            shared_memory_ptr, SharedDataLayout::TURN_LANE_DATA);
    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::TURN_LANE_DATA) > 0)
    {
        lane_data_stream.read(reinterpret_cast<char *>(turn_lane_data_ptr),
                              shared_layout_ptr->GetBlockSize(SharedDataLayout::TURN_LANE_DATA));
    }
    lane_data_stream.close();

    auto *turn_lane_offset_ptr = shared_layout_ptr->GetBlockPtr<std::uint32_t, true>(
        shared_memory_ptr, SharedDataLayout::LANE_DESCRIPTION_OFFSETS);
    if (!lane_description_offsets.empty())
    {
        BOOST_ASSERT(shared_layout_ptr->GetBlockSize(SharedDataLayout::LANE_DESCRIPTION_OFFSETS) >=
                     sizeof(lane_description_offsets[0]) * lane_description_offsets.size());
        std::copy(
            lane_description_offsets.begin(), lane_description_offsets.end(), turn_lane_offset_ptr);
        std::vector<std::uint32_t> tmp;
        lane_description_offsets.swap(tmp);
    }

    auto *turn_lane_mask_ptr =
        shared_layout_ptr->GetBlockPtr<extractor::guidance::TurnLaneType::Mask, true>(
            shared_memory_ptr, SharedDataLayout::LANE_DESCRIPTION_MASKS);
    if (!lane_description_masks.empty())
    {
        BOOST_ASSERT(shared_layout_ptr->GetBlockSize(SharedDataLayout::LANE_DESCRIPTION_MASKS) >=
                     sizeof(lane_description_masks[0]) * lane_description_masks.size());
        std::copy(lane_description_masks.begin(), lane_description_masks.end(), turn_lane_mask_ptr);
        std::vector<extractor::guidance::TurnLaneType::Mask> tmp;
        lane_description_masks.swap(tmp);
    }

    // load original edge information
    GeometryID *via_geometry_ptr = shared_layout_ptr->GetBlockPtr<GeometryID, true>(
        shared_memory_ptr, SharedDataLayout::VIA_NODE_LIST);

    unsigned *name_id_ptr = shared_layout_ptr->GetBlockPtr<unsigned, true>(
        shared_memory_ptr, SharedDataLayout::NAME_ID_LIST);

    extractor::TravelMode *travel_mode_ptr =
        shared_layout_ptr->GetBlockPtr<extractor::TravelMode, true>(shared_memory_ptr,
                                                                    SharedDataLayout::TRAVEL_MODE);
    util::guidance::TurnBearing *pre_turn_bearing_ptr =
        shared_layout_ptr->GetBlockPtr<util::guidance::TurnBearing, true>(
            shared_memory_ptr, SharedDataLayout::PRE_TURN_BEARING);
    util::guidance::TurnBearing *post_turn_bearing_ptr =
        shared_layout_ptr->GetBlockPtr<util::guidance::TurnBearing, true>(
            shared_memory_ptr, SharedDataLayout::POST_TURN_BEARING);

    LaneDataID *lane_data_id_ptr = shared_layout_ptr->GetBlockPtr<LaneDataID, true>(
        shared_memory_ptr, SharedDataLayout::LANE_DATA_ID);

    extractor::guidance::TurnInstruction *turn_instructions_ptr =
        shared_layout_ptr->GetBlockPtr<extractor::guidance::TurnInstruction, true>(
            shared_memory_ptr, SharedDataLayout::TURN_INSTRUCTION);

    EntryClassID *entry_class_id_ptr = shared_layout_ptr->GetBlockPtr<EntryClassID, true>(
        shared_memory_ptr, SharedDataLayout::ENTRY_CLASSID);

    io::readEdgesData(edges_input_stream,
                      via_geometry_ptr,
                      name_id_ptr,
                      turn_instructions_ptr,
                      lane_data_id_ptr,
                      travel_mode_ptr,
                      entry_class_id_ptr,
                      pre_turn_bearing_ptr,
                      post_turn_bearing_ptr,
                      number_of_original_edges);
    edges_input_stream.close();

    // load compressed geometry
    unsigned temporary_value;
    unsigned *geometries_index_ptr = shared_layout_ptr->GetBlockPtr<unsigned, true>(
        shared_memory_ptr, SharedDataLayout::GEOMETRIES_INDEX);
    geometry_input_stream.seekg(0, geometry_input_stream.beg);
    geometry_input_stream.read((char *)&temporary_value, sizeof(unsigned));
    BOOST_ASSERT(temporary_value ==
                 shared_layout_ptr->num_entries[SharedDataLayout::GEOMETRIES_INDEX]);

    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::GEOMETRIES_INDEX) > 0)
    {
        geometry_input_stream.read(
            (char *)geometries_index_ptr,
            shared_layout_ptr->GetBlockSize(SharedDataLayout::GEOMETRIES_INDEX));
    }
    NodeID *geometries_node_id_list_ptr = shared_layout_ptr->GetBlockPtr<NodeID, true>(
        shared_memory_ptr, SharedDataLayout::GEOMETRIES_NODE_LIST);

    geometry_input_stream.read((char *)&temporary_value, sizeof(unsigned));
    BOOST_ASSERT(temporary_value ==
                 shared_layout_ptr->num_entries[SharedDataLayout::GEOMETRIES_NODE_LIST]);

    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::GEOMETRIES_NODE_LIST) > 0)
    {
        geometry_input_stream.read(
            (char *)geometries_node_id_list_ptr,
            shared_layout_ptr->GetBlockSize(SharedDataLayout::GEOMETRIES_NODE_LIST));
    }
    EdgeWeight *geometries_fwd_weight_list_ptr = shared_layout_ptr->GetBlockPtr<EdgeWeight, true>(
        shared_memory_ptr, SharedDataLayout::GEOMETRIES_FWD_WEIGHT_LIST);

    BOOST_ASSERT(temporary_value ==
                 shared_layout_ptr->num_entries[SharedDataLayout::GEOMETRIES_FWD_WEIGHT_LIST]);

    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::GEOMETRIES_FWD_WEIGHT_LIST) > 0)
    {
        geometry_input_stream.read(
            (char *)geometries_fwd_weight_list_ptr,
            shared_layout_ptr->GetBlockSize(SharedDataLayout::GEOMETRIES_FWD_WEIGHT_LIST));
    }
    EdgeWeight *geometries_rev_weight_list_ptr = shared_layout_ptr->GetBlockPtr<EdgeWeight, true>(
        shared_memory_ptr, SharedDataLayout::GEOMETRIES_REV_WEIGHT_LIST);

    BOOST_ASSERT(temporary_value ==
                 shared_layout_ptr->num_entries[SharedDataLayout::GEOMETRIES_REV_WEIGHT_LIST]);

    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::GEOMETRIES_REV_WEIGHT_LIST) > 0)
    {
        geometry_input_stream.read(
            (char *)geometries_rev_weight_list_ptr,
            shared_layout_ptr->GetBlockSize(SharedDataLayout::GEOMETRIES_REV_WEIGHT_LIST));
    }

    // load datasource information (if it exists)
    uint8_t *datasources_list_ptr = shared_layout_ptr->GetBlockPtr<uint8_t, true>(
        shared_memory_ptr, SharedDataLayout::DATASOURCES_LIST);
    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::DATASOURCES_LIST) > 0)
    {
        geometry_datasource_input_stream.read(
            reinterpret_cast<char *>(datasources_list_ptr),
            shared_layout_ptr->GetBlockSize(SharedDataLayout::DATASOURCES_LIST));
    }

    // load datasource name information (if it exists)
    char *datasource_name_data_ptr = shared_layout_ptr->GetBlockPtr<char, true>(
        shared_memory_ptr, SharedDataLayout::DATASOURCE_NAME_DATA);
    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::DATASOURCE_NAME_DATA) > 0)
    {
        std::copy(
            m_datasource_name_data.begin(), m_datasource_name_data.end(), datasource_name_data_ptr);
    }

    auto datasource_name_offsets_ptr = shared_layout_ptr->GetBlockPtr<std::size_t, true>(
        shared_memory_ptr, SharedDataLayout::DATASOURCE_NAME_OFFSETS);
    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::DATASOURCE_NAME_OFFSETS) > 0)
    {
        std::copy(m_datasource_name_offsets.begin(),
                  m_datasource_name_offsets.end(),
                  datasource_name_offsets_ptr);
    }

    auto datasource_name_lengths_ptr = shared_layout_ptr->GetBlockPtr<std::size_t, true>(
        shared_memory_ptr, SharedDataLayout::DATASOURCE_NAME_LENGTHS);
    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::DATASOURCE_NAME_LENGTHS) > 0)
    {
        std::copy(m_datasource_name_lengths.begin(),
                  m_datasource_name_lengths.end(),
                  datasource_name_lengths_ptr);
    }

    // Loading list of coordinates
    util::Coordinate *coordinates_ptr = shared_layout_ptr->GetBlockPtr<util::Coordinate, true>(
        shared_memory_ptr, SharedDataLayout::COORDINATE_LIST);
    std::uint64_t *osmnodeid_ptr = shared_layout_ptr->GetBlockPtr<std::uint64_t, true>(
        shared_memory_ptr, SharedDataLayout::OSM_NODE_ID_LIST);
    util::PackedVector<OSMNodeID, true> osmnodeid_list;
    osmnodeid_list.reset(osmnodeid_ptr,
                         shared_layout_ptr->num_entries[SharedDataLayout::OSM_NODE_ID_LIST]);

    io::readNodesData(nodes_input_stream, coordinates_ptr, osmnodeid_list, coordinate_list_size);
    nodes_input_stream.close();

    // store timestamp
    char *timestamp_ptr =
        shared_layout_ptr->GetBlockPtr<char, true>(shared_memory_ptr, SharedDataLayout::TIMESTAMP);
    io::readTimestamp(timestamp_stream, timestamp_ptr, timestamp_size);

    // store search tree portion of rtree
    char *rtree_ptr = shared_layout_ptr->GetBlockPtr<char, true>(shared_memory_ptr,
                                                                 SharedDataLayout::R_SEARCH_TREE);

    if (tree_size > 0)
    {
        tree_node_file.read(rtree_ptr, sizeof(RTreeNode) * tree_size);
    }
    tree_node_file.close();

    // load core markers
    std::vector<char> unpacked_core_markers(number_of_core_markers);
    core_marker_file.read((char *)unpacked_core_markers.data(),
                          sizeof(char) * number_of_core_markers);

    unsigned *core_marker_ptr = shared_layout_ptr->GetBlockPtr<unsigned, true>(
        shared_memory_ptr, SharedDataLayout::CORE_MARKER);

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

    // load the nodes of the search graph
    QueryGraph::NodeArrayEntry *graph_node_list_ptr =
        shared_layout_ptr->GetBlockPtr<QueryGraph::NodeArrayEntry, true>(
            shared_memory_ptr, SharedDataLayout::GRAPH_NODE_LIST);

    // load the edges of the search graph
    QueryGraph::EdgeArrayEntry *graph_edge_list_ptr =
        shared_layout_ptr->GetBlockPtr<QueryGraph::EdgeArrayEntry, true>(
            shared_memory_ptr, SharedDataLayout::GRAPH_EDGE_LIST);

    io::readHSGR(hsgr_input_stream,
                 graph_node_list_ptr,
                 hsgr_header.number_of_nodes,
                 graph_edge_list_ptr,
                 hsgr_header.number_of_edges);
    hsgr_input_stream.close();

    // load profile properties
    auto profile_properties_ptr =
        shared_layout_ptr->GetBlockPtr<extractor::ProfileProperties, true>(
            shared_memory_ptr, SharedDataLayout::PROPERTIES);
    boost::filesystem::ifstream profile_properties_stream(config.properties_path);
    if (!profile_properties_stream)
    {
        util::exception("Could not open " + config.properties_path.string() + " for reading!");
    }
    profile_properties_stream.read(reinterpret_cast<char *>(profile_properties_ptr),
                                   sizeof(extractor::ProfileProperties));

    // load intersection classes
    if (!bearing_class_id_table.empty())
    {
        auto bearing_id_ptr = shared_layout_ptr->GetBlockPtr<BearingClassID, true>(
            shared_memory_ptr, SharedDataLayout::BEARING_CLASSID);
        std::copy(bearing_class_id_table.begin(), bearing_class_id_table.end(), bearing_id_ptr);
    }

    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::BEARING_OFFSETS) > 0)
    {
        auto *bearing_offsets_ptr = shared_layout_ptr->GetBlockPtr<unsigned, true>(
            shared_memory_ptr, SharedDataLayout::BEARING_OFFSETS);
        std::copy(bearing_offsets_data.begin(), bearing_offsets_data.end(), bearing_offsets_ptr);
    }

    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::BEARING_BLOCKS) > 0)
    {
        auto *bearing_blocks_ptr =
            shared_layout_ptr->GetBlockPtr<typename util::RangeTable<16, true>::BlockT, true>(
                shared_memory_ptr, SharedDataLayout::BEARING_BLOCKS);
        std::copy(bearing_blocks_data.begin(), bearing_blocks_data.end(), bearing_blocks_ptr);
    }

    if (!bearing_class_table.empty())
    {
        auto bearing_class_ptr = shared_layout_ptr->GetBlockPtr<DiscreteBearing, true>(
            shared_memory_ptr, SharedDataLayout::BEARING_VALUES);
        std::copy(bearing_class_table.begin(), bearing_class_table.end(), bearing_class_ptr);
    }

    if (!entry_class_table.empty())
    {
        auto entry_class_ptr = shared_layout_ptr->GetBlockPtr<util::guidance::EntryClass, true>(
            shared_memory_ptr, SharedDataLayout::ENTRY_CLASS);
        std::copy(entry_class_table.begin(), entry_class_table.end(), entry_class_ptr);
    }

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
}
}

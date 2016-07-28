#include "storage/storage.hpp"
#include "contractor/query_edge.hpp"
#include "extractor/compressed_edge_container.hpp"
#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/original_edge_data.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/query_node.hpp"
#include "extractor/travel_mode.hpp"
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

#include <boost/filesystem/fstream.hpp>
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

// delete a shared memory region. report warning if it could not be deleted
void deleteRegion(const SharedDataType region)
{
    if (SharedMemory::RegionExists(region) && !SharedMemory::Remove(region))
    {
        const std::string name = [&] {
            switch (region)
            {
            case CURRENT_REGIONS:
                return "CURRENT_REGIONS";
            case LAYOUT_1:
                return "LAYOUT_1";
            case DATA_1:
                return "DATA_1";
            case LAYOUT_2:
                return "LAYOUT_2";
            case DATA_2:
                return "DATA_2";
            case LAYOUT_NONE:
                return "LAYOUT_NONE";
            default: // DATA_NONE:
                return "DATA_NONE";
            }
        }();

        util::SimpleLogger().Write(logWARNING) << "could not delete shared memory region " << name;
    }
}

Storage::Storage(StorageConfig config_) : config(std::move(config_)) {}

int Storage::Run()
{
    BOOST_ASSERT_MSG(config.IsValid(), "Invalid storage config");

    util::LogPolicy::GetInstance().Unmute();
    SharedBarriers barrier;

#ifdef __linux__
    // try to disable swapping on Linux
    const bool lock_flags = MCL_CURRENT | MCL_FUTURE;
    if (-1 == mlockall(lock_flags))
    {
        util::SimpleLogger().Write(logWARNING) << "Could not request RAM lock";
    }
#endif

    try
    {
        boost::interprocess::scoped_lock<boost::interprocess::named_mutex> pending_lock(
            barrier.pending_update_mutex);
    }
    catch (...)
    {
        // hard unlock in case of any exception.
        barrier.pending_update_mutex.unlock();
    }

    // determine segment to use
    bool segment2_in_use = SharedMemory::RegionExists(LAYOUT_2);
    const storage::SharedDataType layout_region = [&] {
        return segment2_in_use ? LAYOUT_1 : LAYOUT_2;
    }();
    const storage::SharedDataType data_region = [&] { return segment2_in_use ? DATA_1 : DATA_2; }();
    const storage::SharedDataType previous_layout_region = [&] {
        return segment2_in_use ? LAYOUT_2 : LAYOUT_1;
    }();
    const storage::SharedDataType previous_data_region = [&] {
        return segment2_in_use ? DATA_2 : DATA_1;
    }();

    // Allocate a memory layout in shared memory, deallocate previous
    auto *layout_memory = makeSharedMemory(layout_region, sizeof(SharedDataLayout));
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
    util::SimpleLogger().Write() << "name offsets size: " << name_blocks;
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
    unsigned number_of_original_edges = 0;
    edges_input_stream.read((char *)&number_of_original_edges, sizeof(unsigned));

    // note: settings this all to the same size is correct, we extract them from the same struct
    shared_layout_ptr->SetBlockSize<NodeID>(SharedDataLayout::VIA_NODE_LIST,
                                            number_of_original_edges);
    shared_layout_ptr->SetBlockSize<unsigned>(SharedDataLayout::NAME_ID_LIST,
                                              number_of_original_edges);
    shared_layout_ptr->SetBlockSize<extractor::TravelMode>(SharedDataLayout::TRAVEL_MODE,
                                                           number_of_original_edges);
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

    util::FingerPrint fingerprint_valid = util::FingerPrint::GetValid();
    util::FingerPrint fingerprint_loaded;
    hsgr_input_stream.read((char *)&fingerprint_loaded, sizeof(util::FingerPrint));
    if (fingerprint_loaded.TestGraphUtil(fingerprint_valid))
    {
        util::SimpleLogger().Write(logDEBUG) << "Fingerprint checked out ok";
    }
    else
    {
        util::SimpleLogger().Write(logWARNING) << ".hsgr was prepared with different build. "
                                                  "Reprocess to get rid of this warning.";
    }

    // load checksum
    unsigned checksum = 0;
    hsgr_input_stream.read((char *)&checksum, sizeof(unsigned));
    shared_layout_ptr->SetBlockSize<unsigned>(SharedDataLayout::HSGR_CHECKSUM, 1);
    // load graph node size
    unsigned number_of_graph_nodes = 0;
    hsgr_input_stream.read((char *)&number_of_graph_nodes, sizeof(unsigned));

    BOOST_ASSERT_MSG((0 != number_of_graph_nodes), "number of nodes is zero");
    shared_layout_ptr->SetBlockSize<QueryGraph::NodeArrayEntry>(SharedDataLayout::GRAPH_NODE_LIST,
                                                                number_of_graph_nodes);

    // load graph edge size
    unsigned number_of_graph_edges = 0;
    hsgr_input_stream.read((char *)&number_of_graph_edges, sizeof(unsigned));
    // BOOST_ASSERT_MSG(0 != number_of_graph_edges, "number of graph edges is zero");
    shared_layout_ptr->SetBlockSize<QueryGraph::EdgeArrayEntry>(SharedDataLayout::GRAPH_EDGE_LIST,
                                                                number_of_graph_edges);

    // load rsearch tree size
    boost::filesystem::ifstream tree_node_file(config.ram_index_path, std::ios::binary);

    uint32_t tree_size = 0;
    tree_node_file.read((char *)&tree_size, sizeof(uint32_t));
    shared_layout_ptr->SetBlockSize<RTreeNode>(SharedDataLayout::R_SEARCH_TREE, tree_size);

    // load profile properties
    shared_layout_ptr->SetBlockSize<extractor::ProfileProperties>(SharedDataLayout::PROPERTIES, 1);

    // load timestamp size
    boost::filesystem::ifstream timestamp_stream(config.timestamp_path);
    std::string m_timestamp;
    getline(timestamp_stream, m_timestamp);
    shared_layout_ptr->SetBlockSize<char>(SharedDataLayout::TIMESTAMP, m_timestamp.length());

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
    unsigned coordinate_list_size = 0;
    nodes_input_stream.read((char *)&coordinate_list_size, sizeof(unsigned));
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
    shared_layout_ptr->SetBlockSize<extractor::CompressedEdgeContainer::CompressedEdge>(
        SharedDataLayout::GEOMETRIES_LIST, number_of_compressed_geometries);

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
    shared_layout_ptr->SetBlockSize<util::guidance::LaneTupelIdPair>(
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
    auto *shared_memory = makeSharedMemory(data_region, shared_layout_ptr->GetSizeOfLayout());
    char *shared_memory_ptr = static_cast<char *>(shared_memory->Ptr());

    // read actual data into shared memory object //

    // hsgr checksum
    unsigned *checksum_ptr = shared_layout_ptr->GetBlockPtr<unsigned, true>(
        shared_memory_ptr, SharedDataLayout::HSGR_CHECKSUM);
    *checksum_ptr = checksum;

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
        shared_layout_ptr->GetBlockPtr<util::guidance::LaneTupelIdPair, true>(
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
    NodeID *via_node_ptr = shared_layout_ptr->GetBlockPtr<NodeID, true>(
        shared_memory_ptr, SharedDataLayout::VIA_NODE_LIST);

    unsigned *name_id_ptr = shared_layout_ptr->GetBlockPtr<unsigned, true>(
        shared_memory_ptr, SharedDataLayout::NAME_ID_LIST);

    extractor::TravelMode *travel_mode_ptr =
        shared_layout_ptr->GetBlockPtr<extractor::TravelMode, true>(shared_memory_ptr,
                                                                    SharedDataLayout::TRAVEL_MODE);

    LaneDataID *lane_data_id_ptr = shared_layout_ptr->GetBlockPtr<LaneDataID, true>(
        shared_memory_ptr, SharedDataLayout::LANE_DATA_ID);

    extractor::guidance::TurnInstruction *turn_instructions_ptr =
        shared_layout_ptr->GetBlockPtr<extractor::guidance::TurnInstruction, true>(
            shared_memory_ptr, SharedDataLayout::TURN_INSTRUCTION);

    EntryClassID *entry_class_id_ptr = shared_layout_ptr->GetBlockPtr<EntryClassID, true>(
        shared_memory_ptr, SharedDataLayout::ENTRY_CLASSID);

    extractor::OriginalEdgeData current_edge_data;
    for (unsigned i = 0; i < number_of_original_edges; ++i)
    {
        edges_input_stream.read((char *)&(current_edge_data), sizeof(extractor::OriginalEdgeData));
        via_node_ptr[i] = current_edge_data.via_node;
        name_id_ptr[i] = current_edge_data.name_id;
        travel_mode_ptr[i] = current_edge_data.travel_mode;
        lane_data_id_ptr[i] = current_edge_data.lane_data_id;
        turn_instructions_ptr[i] = current_edge_data.turn_instruction;
        entry_class_id_ptr[i] = current_edge_data.entry_classid;
    }
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
    extractor::CompressedEdgeContainer::CompressedEdge *geometries_list_ptr =
        shared_layout_ptr->GetBlockPtr<extractor::CompressedEdgeContainer::CompressedEdge, true>(
            shared_memory_ptr, SharedDataLayout::GEOMETRIES_LIST);

    geometry_input_stream.read((char *)&temporary_value, sizeof(unsigned));
    BOOST_ASSERT(temporary_value ==
                 shared_layout_ptr->num_entries[SharedDataLayout::GEOMETRIES_LIST]);

    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::GEOMETRIES_LIST) > 0)
    {
        geometry_input_stream.read(
            (char *)geometries_list_ptr,
            shared_layout_ptr->GetBlockSize(SharedDataLayout::GEOMETRIES_LIST));
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
        util::SimpleLogger().Write()
            << "Copying " << (m_datasource_name_data.end() - m_datasource_name_data.begin())
            << " chars into name data ptr";
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
    osmnodeid_list.reset(
        osmnodeid_ptr, shared_layout_ptr->num_entries[storage::SharedDataLayout::OSM_NODE_ID_LIST]);

    extractor::QueryNode current_node;
    for (unsigned i = 0; i < coordinate_list_size; ++i)
    {
        nodes_input_stream.read((char *)&current_node, sizeof(extractor::QueryNode));
        coordinates_ptr[i] = util::Coordinate(current_node.lon, current_node.lat);
        osmnodeid_list.push_back(current_node.node_id);
    }
    nodes_input_stream.close();

    // store timestamp
    char *timestamp_ptr =
        shared_layout_ptr->GetBlockPtr<char, true>(shared_memory_ptr, SharedDataLayout::TIMESTAMP);
    std::copy(m_timestamp.c_str(), m_timestamp.c_str() + m_timestamp.length(), timestamp_ptr);

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
    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::GRAPH_NODE_LIST) > 0)
    {
        hsgr_input_stream.read((char *)graph_node_list_ptr,
                               shared_layout_ptr->GetBlockSize(SharedDataLayout::GRAPH_NODE_LIST));
    }

    // load the edges of the search graph
    QueryGraph::EdgeArrayEntry *graph_edge_list_ptr =
        shared_layout_ptr->GetBlockPtr<QueryGraph::EdgeArrayEntry, true>(
            shared_memory_ptr, SharedDataLayout::GRAPH_EDGE_LIST);
    if (shared_layout_ptr->GetBlockSize(SharedDataLayout::GRAPH_EDGE_LIST) > 0)
    {
        hsgr_input_stream.read((char *)graph_edge_list_ptr,
                               shared_layout_ptr->GetBlockSize(SharedDataLayout::GRAPH_EDGE_LIST));
    }
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

    // acquire lock
    SharedMemory *data_type_memory =
        makeSharedMemory(CURRENT_REGIONS, sizeof(SharedDataTimestamp), true, false);
    SharedDataTimestamp *data_timestamp_ptr =
        static_cast<SharedDataTimestamp *>(data_type_memory->Ptr());

    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> query_lock(
        barrier.query_mutex);

    // notify all processes that were waiting for this condition
    if (0 < barrier.number_of_queries)
    {
        barrier.no_running_queries_condition.wait(query_lock);
    }

    data_timestamp_ptr->layout = layout_region;
    data_timestamp_ptr->data = data_region;
    data_timestamp_ptr->timestamp += 1;
    deleteRegion(previous_data_region);
    deleteRegion(previous_layout_region);
    util::SimpleLogger().Write() << "all data loaded";

    return EXIT_SUCCESS;
}
}
}

#ifndef SHARED_DATA_TYPE_HPP
#define SHARED_DATA_TYPE_HPP

#include "util/exception.hpp"
#include "util/simple_logger.hpp"

#include <cstdint>

#include <array>

namespace osrm
{
namespace storage
{

// Added at the start and end of each block as sanity check
const constexpr char CANARY[4] = {'O', 'S', 'R', 'M'};

const constexpr char *block_id_to_name[] = {"NAME_OFFSETS",
                                            "NAME_BLOCKS",
                                            "NAME_CHAR_LIST",
                                            "NAME_ID_LIST",
                                            "VIA_NODE_LIST",
                                            "GRAPH_NODE_LIST",
                                            "GRAPH_EDGE_LIST",
                                            "COORDINATE_LIST",
                                            "OSM_NODE_ID_LIST",
                                            "TURN_INSTRUCTION",
                                            "TRAVEL_MODE",
                                            "ENTRY_CLASSID",
                                            "R_SEARCH_TREE",
                                            "GEOMETRIES_INDEX",
                                            "GEOMETRIES_LIST",
                                            "HSGR_CHECKSUM",
                                            "TIMESTAMP",
                                            "FILE_INDEX_PATH",
                                            "CORE_MARKER",
                                            "DATASOURCES_LIST",
                                            "DATASOURCE_NAME_DATA",
                                            "DATASOURCE_NAME_OFFSETS",
                                            "DATASOURCE_NAME_LENGTHS",
                                            "PROPERTIES",
                                            "BEARING_CLASSID",
                                            "BEARING_OFFSETS",
                                            "BEARING_BLOCKS",
                                            "BEARING_VALUES",
                                            "ENTRY_CLASS",
                                            "LANE_DATA_ID",
                                            "TURN_LANE_DATA",
                                            "LANE_DESCRIPTION_OFFSETS",
                                            "LANE_DESCRIPTION_MASKS"};

struct SharedDataLayout
{
    enum BlockID
    {
        NAME_OFFSETS = 0,
        NAME_BLOCKS,
        NAME_CHAR_LIST,
        NAME_ID_LIST,
        VIA_NODE_LIST,
        GRAPH_NODE_LIST,
        GRAPH_EDGE_LIST,
        COORDINATE_LIST,
        OSM_NODE_ID_LIST,
        TURN_INSTRUCTION,
        TRAVEL_MODE,
        ENTRY_CLASSID,
        R_SEARCH_TREE,
        GEOMETRIES_INDEX,
        GEOMETRIES_LIST,
        HSGR_CHECKSUM,
        TIMESTAMP,
        FILE_INDEX_PATH,
        CORE_MARKER,
        DATASOURCES_LIST,
        DATASOURCE_NAME_DATA,
        DATASOURCE_NAME_OFFSETS,
        DATASOURCE_NAME_LENGTHS,
        PROPERTIES,
        BEARING_CLASSID,
        BEARING_OFFSETS,
        BEARING_BLOCKS,
        BEARING_VALUES,
        ENTRY_CLASS,
        LANE_DATA_ID,
        TURN_LANE_DATA,
        LANE_DESCRIPTION_OFFSETS,
        LANE_DESCRIPTION_MASKS,
        NUM_BLOCKS
    };

    std::array<uint64_t, NUM_BLOCKS> num_entries;
    std::array<uint64_t, NUM_BLOCKS> entry_size;

    SharedDataLayout() : num_entries(), entry_size() {}

    template <typename T> inline void SetBlockSize(BlockID bid, uint64_t entries)
    {
        num_entries[bid] = entries;
        entry_size[bid] = sizeof(T);
    }

    inline uint64_t AlignBlockSize(uint64_t block_size) const
    {
        const uint64_t alignment = 4;
        return (block_size + (alignment - 1)) & ~(alignment - 1);
    }

    inline uint64_t GetBlockSize(BlockID bid) const
    {
        // special bit encoding
        if (bid == CORE_MARKER)
        {
            return AlignBlockSize((num_entries[bid] / 32 + 1) * entry_size[bid]);
        }
        return AlignBlockSize(num_entries[bid] * entry_size[bid]);
    }

    inline uint64_t GetSizeOfLayout() const
    {
        return GetBlockOffset(NUM_BLOCKS) + NUM_BLOCKS * 2 * sizeof(CANARY);
    }

    inline uint64_t GetBlockOffset(BlockID bid) const
    {
        uint64_t result = sizeof(CANARY);
        for (auto i = 0; i < bid; i++)
        {
            result += GetBlockSize((BlockID)i) + 2 * sizeof(CANARY);
        }
        return result;
    }

    template <typename T, bool WRITE_CANARY = false>
    inline T *GetBlockPtr(char *shared_memory, BlockID bid)
    {
        T *ptr = (T *)(shared_memory + GetBlockOffset(bid));
        if (WRITE_CANARY)
        {
            char *start_canary_ptr = shared_memory + GetBlockOffset(bid) - sizeof(CANARY);
            char *end_canary_ptr = shared_memory + GetBlockOffset(bid) + GetBlockSize(bid);
            std::copy(CANARY, CANARY + sizeof(CANARY), start_canary_ptr);
            std::copy(CANARY, CANARY + sizeof(CANARY), end_canary_ptr);
        }
        else
        {
            char *start_canary_ptr = shared_memory + GetBlockOffset(bid) - sizeof(CANARY);
            char *end_canary_ptr = shared_memory + GetBlockOffset(bid) + GetBlockSize(bid);
            bool start_canary_alive = std::equal(CANARY, CANARY + sizeof(CANARY), start_canary_ptr);
            bool end_canary_alive = std::equal(CANARY, CANARY + sizeof(CANARY), end_canary_ptr);
            if (!start_canary_alive)
            {
                throw util::exception(std::string("Start canary of block corrupted. (") +
                                      block_id_to_name[bid] + ")");
            }
            if (!end_canary_alive)
            {
                throw util::exception(std::string("End canary of block corrupted. (") +
                                      block_id_to_name[bid] + ")");
            }
        }

        return ptr;
    }
};

enum SharedDataType
{
    CURRENT_REGIONS,
    LAYOUT_1,
    DATA_1,
    LAYOUT_2,
    DATA_2,
    LAYOUT_NONE,
    DATA_NONE
};

struct SharedDataTimestamp
{
    SharedDataType layout;
    SharedDataType data;
    unsigned timestamp;
};

inline std::string regionToString(const SharedDataType region)
{
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
    case DATA_NONE:
        return "DATA_NONE";
    default:
        return "INVALID_REGION";
    }
}

static_assert(sizeof(block_id_to_name) / sizeof(*block_id_to_name) == SharedDataLayout::NUM_BLOCKS,
              "Number of blocks needs to match the number of Block names.");
}
}

#endif /* SHARED_DATA_TYPE_HPP */

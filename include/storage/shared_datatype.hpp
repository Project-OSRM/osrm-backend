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
const constexpr char CANARY[] = "OSRM";

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
        ENTRY_CLASSID,
        TRAVEL_MODE,
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

    inline uint64_t GetBlockSize(BlockID bid) const
    {
        // special bit encoding
        if (bid == CORE_MARKER)
        {
            return (num_entries[bid] / 32 + 1) * entry_size[bid];
        }

        return num_entries[bid] * entry_size[bid];
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
                throw util::exception("Start canary of block corrupted.");
            }
            if (!end_canary_alive)
            {
                throw util::exception("End canary of block corrupted.");
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
}
}

#endif /* SHARED_DATA_TYPE_HPP */

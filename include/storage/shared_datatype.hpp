#ifndef SHARED_DATA_TYPE_HPP
#define SHARED_DATA_TYPE_HPP

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/log.hpp"

#include <boost/assert.hpp>

#include <array>
#include <cstdint>

namespace osrm
{
namespace storage
{

// Added at the start and end of each block as sanity check
const constexpr char CANARY[4] = {'O', 'S', 'R', 'M'};

const constexpr char *block_id_to_name[] = {"NAME_CHAR_DATA",
                                            "NAME_ID_LIST",
                                            "VIA_NODE_LIST",
                                            "CH_GRAPH_NODE_LIST",
                                            "CH_GRAPH_EDGE_LIST",
                                            "COORDINATE_LIST",
                                            "OSM_NODE_ID_LIST",
                                            "TURN_INSTRUCTION",
                                            "TRAVEL_MODE",
                                            "ENTRY_CLASSID",
                                            "R_SEARCH_TREE",
                                            "GEOMETRIES_INDEX",
                                            "GEOMETRIES_NODE_LIST",
                                            "GEOMETRIES_FWD_WEIGHT_LIST",
                                            "GEOMETRIES_REV_WEIGHT_LIST",
                                            "GEOMETRIES_FWD_DURATION_LIST",
                                            "GEOMETRIES_REV_DURATION_LIST",
                                            "HSGR_CHECKSUM",
                                            "TIMESTAMP",
                                            "FILE_INDEX_PATH",
                                            "CH_CORE_MARKER",
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
                                            "PRE_TURN_BEARING",
                                            "POST_TURN_BEARING",
                                            "TURN_LANE_DATA",
                                            "LANE_DESCRIPTION_OFFSETS",
                                            "LANE_DESCRIPTION_MASKS",
                                            "TURN_WEIGHT_PENALTIES",
                                            "TURN_DURATION_PENALTIES",
                                            "MLD_LEVEL_DATA",
                                            "MLD_PARTITION",
                                            "MLD_CELL_TO_CHILDREN",
                                            "MLD_CELL_STORAGE"};

struct DataLayout
{
    enum BlockID
    {
        NAME_CHAR_DATA = 0,
        NAME_ID_LIST,
        VIA_NODE_LIST,
        CH_GRAPH_NODE_LIST,
        CH_GRAPH_EDGE_LIST,
        COORDINATE_LIST,
        OSM_NODE_ID_LIST,
        TURN_INSTRUCTION,
        TRAVEL_MODE,
        ENTRY_CLASSID,
        R_SEARCH_TREE,
        GEOMETRIES_INDEX,
        GEOMETRIES_NODE_LIST,
        GEOMETRIES_FWD_WEIGHT_LIST,
        GEOMETRIES_REV_WEIGHT_LIST,
        GEOMETRIES_FWD_DURATION_LIST,
        GEOMETRIES_REV_DURATION_LIST,
        HSGR_CHECKSUM,
        TIMESTAMP,
        FILE_INDEX_PATH,
        CH_CORE_MARKER,
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
        PRE_TURN_BEARING,
        POST_TURN_BEARING,
        TURN_LANE_DATA,
        LANE_DESCRIPTION_OFFSETS,
        LANE_DESCRIPTION_MASKS,
        TURN_WEIGHT_PENALTIES,
        TURN_DURATION_PENALTIES,
        MLD_LEVEL_DATA,
        MLD_PARTITION,
        MLD_CELL_TO_CHILDREN,
        MLD_CELL_STORAGE,
        NUM_BLOCKS
    };

    std::array<std::uint64_t, NUM_BLOCKS> num_entries;
    std::array<std::size_t, NUM_BLOCKS> entry_size;
    std::array<std::size_t, NUM_BLOCKS> entry_align;

    DataLayout() : num_entries(), entry_size(), entry_align() {}

    template <typename T> inline void SetBlockSize(BlockID bid, uint64_t entries)
    {
        static_assert(sizeof(T) % alignof(T) == 0, "aligned T* can't be used as an array pointer");
        num_entries[bid] = entries;
        entry_size[bid] = sizeof(T);
        entry_align[bid] = alignof(T);
    }

    inline uint64_t GetBlockEntries(BlockID bid) const { return num_entries[bid]; }

    inline uint64_t GetBlockSize(BlockID bid) const
    {
        // special bit encoding
        if (bid == CH_CORE_MARKER)
        {
            return (num_entries[bid] / 32 + 1) * entry_size[bid];
        }
        return num_entries[bid] * entry_size[bid];
    }

    inline uint64_t GetSizeOfLayout() const
    {
        uint64_t result = 0;
        for (auto i = 0; i < NUM_BLOCKS; i++)
        {
            BOOST_ASSERT(entry_align[i] > 0);
            result += 2 * sizeof(CANARY) + GetBlockSize((BlockID)i) + entry_align[i];
        }
        return result;
    }

    // \brief Fit aligned storage in buffer.
    // Interface Similar to [ptr.align] but omits space computation.
    // The method can be removed and changed directly to an std::align
    // function call after dropping gcc < 5 support.
    inline void *align(std::size_t align, std::size_t, void *&ptr) const noexcept
    {
        const auto intptr = reinterpret_cast<uintptr_t>(ptr);
        const auto aligned = (intptr - 1u + align) & -align;
        return ptr = reinterpret_cast<void *>(aligned);
    }

    inline void *GetAlignedBlockPtr(void *ptr, BlockID bid) const
    {
        for (auto i = 0; i < bid; i++)
        {
            ptr = static_cast<char *>(ptr) + sizeof(CANARY);
            ptr = align(entry_align[i], entry_size[i], ptr);
            ptr = static_cast<char *>(ptr) + GetBlockSize((BlockID)i);
            ptr = static_cast<char *>(ptr) + sizeof(CANARY);
        }

        ptr = static_cast<char *>(ptr) + sizeof(CANARY);
        ptr = align(entry_align[bid], entry_size[bid], ptr);
        return ptr;
    }

    template <typename T> inline T *GetBlockEnd(char *shared_memory, BlockID bid) const
    {
        auto begin = GetBlockPtr<T>(shared_memory, bid);
        return begin + GetBlockEntries(bid);
    }

    template <typename T, bool WRITE_CANARY = false>
    inline T *GetBlockPtr(char *shared_memory, BlockID bid) const
    {
        char *ptr = (char *)GetAlignedBlockPtr(shared_memory, bid);
        if (WRITE_CANARY)
        {
            char *start_canary_ptr = ptr - sizeof(CANARY);
            char *end_canary_ptr = ptr + GetBlockSize(bid);
            std::copy(CANARY, CANARY + sizeof(CANARY), start_canary_ptr);
            std::copy(CANARY, CANARY + sizeof(CANARY), end_canary_ptr);
        }
        else
        {
            char *start_canary_ptr = ptr - sizeof(CANARY);
            char *end_canary_ptr = ptr + GetBlockSize(bid);
            bool start_canary_alive = std::equal(CANARY, CANARY + sizeof(CANARY), start_canary_ptr);
            bool end_canary_alive = std::equal(CANARY, CANARY + sizeof(CANARY), end_canary_ptr);
            if (!start_canary_alive)
            {
                throw util::exception("Start canary of block corrupted. (" +
                                      std::string(block_id_to_name[bid]) + ")" + SOURCE_REF);
            }
            if (!end_canary_alive)
            {
                throw util::exception("End canary of block corrupted. (" +
                                      std::string(block_id_to_name[bid]) + ")" + SOURCE_REF);
            }
        }

        return (T *)ptr;
    }
};

enum SharedDataType
{
    REGION_NONE,
    REGION_1,
    REGION_2
};

struct SharedDataTimestamp
{
    explicit SharedDataTimestamp(SharedDataType region, unsigned timestamp)
        : region(region), timestamp(timestamp)
    {
    }

    SharedDataType region;
    unsigned timestamp;

    static constexpr const char *name = "osrm-region";
};

inline std::string regionToString(const SharedDataType region)
{
    switch (region)
    {
    case REGION_1:
        return "REGION_1";
    case REGION_2:
        return "REGION_2";
    case REGION_NONE:
        return "REGION_NONE";
    default:
        return "INVALID_REGION";
    }
}

static_assert(sizeof(block_id_to_name) / sizeof(*block_id_to_name) == DataLayout::NUM_BLOCKS,
              "Number of blocks needs to match the number of Block names.");
}
}

#endif /* SHARED_DATA_TYPE_HPP */

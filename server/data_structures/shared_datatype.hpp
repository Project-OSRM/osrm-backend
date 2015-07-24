/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef SHARED_DATA_TYPE_HPP
#define SHARED_DATA_TYPE_HPP

#include "../../util/osrm_exception.hpp"
#include "../../util/simple_logger.hpp"

#include <cstdint>

#include <array>

namespace
{
// Added at the start and end of each block as sanity check
static const char CANARY[] = "OSRM";
}

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
        TURN_INSTRUCTION,
        TRAVEL_MODE,
        R_SEARCH_TREE,
        GEOMETRIES_INDEX,
        GEOMETRIES_LIST,
        GEOMETRIES_INDICATORS,
        HSGR_CHECKSUM,
        TIMESTAMP,
        FILE_INDEX_PATH,
        CORE_MARKER,
        TRAFFIC_SEGMENT_ID_LIST,
        NUM_BLOCKS
    };

    std::array<uint64_t, NUM_BLOCKS> num_entries;
    std::array<uint64_t, NUM_BLOCKS> entry_size;

    SharedDataLayout() : num_entries(), entry_size() {}

    void PrintInformation() const
    {
        SimpleLogger().Write(logDEBUG) << "NAME_OFFSETS         "
                                       << ": " << GetBlockSize(NAME_OFFSETS);
        SimpleLogger().Write(logDEBUG) << "NAME_BLOCKS          "
                                       << ": " << GetBlockSize(NAME_BLOCKS);
        SimpleLogger().Write(logDEBUG) << "NAME_CHAR_LIST       "
                                       << ": " << GetBlockSize(NAME_CHAR_LIST);
        SimpleLogger().Write(logDEBUG) << "NAME_ID_LIST         "
                                       << ": " << GetBlockSize(NAME_ID_LIST);
        SimpleLogger().Write(logDEBUG) << "VIA_NODE_LIST        "
                                       << ": " << GetBlockSize(VIA_NODE_LIST);
        SimpleLogger().Write(logDEBUG) << "GRAPH_NODE_LIST      "
                                       << ": " << GetBlockSize(GRAPH_NODE_LIST);
        SimpleLogger().Write(logDEBUG) << "GRAPH_EDGE_LIST      "
                                       << ": " << GetBlockSize(GRAPH_EDGE_LIST);
        SimpleLogger().Write(logDEBUG) << "COORDINATE_LIST      "
                                       << ": " << GetBlockSize(COORDINATE_LIST);
        SimpleLogger().Write(logDEBUG) << "TURN_INSTRUCTION     "
                                       << ": " << GetBlockSize(TURN_INSTRUCTION);
        SimpleLogger().Write(logDEBUG) << "TRAVEL_MODE          "
                                       << ": " << GetBlockSize(TRAVEL_MODE);
        SimpleLogger().Write(logDEBUG) << "R_SEARCH_TREE        "
                                       << ": " << GetBlockSize(R_SEARCH_TREE);
        SimpleLogger().Write(logDEBUG) << "GEOMETRIES_INDEX     "
                                       << ": " << GetBlockSize(GEOMETRIES_INDEX);
        SimpleLogger().Write(logDEBUG) << "GEOMETRIES_LIST      "
                                       << ": " << GetBlockSize(GEOMETRIES_LIST);
        SimpleLogger().Write(logDEBUG) << "GEOMETRIES_INDICATORS"
                                       << ": " << GetBlockSize(GEOMETRIES_INDICATORS);
        SimpleLogger().Write(logDEBUG) << "HSGR_CHECKSUM        "
                                       << ": " << GetBlockSize(HSGR_CHECKSUM);
        SimpleLogger().Write(logDEBUG) << "TIMESTAMP            "
                                       << ": " << GetBlockSize(TIMESTAMP);
        SimpleLogger().Write(logDEBUG) << "FILE_INDEX_PATH      "
                                       << ": " << GetBlockSize(FILE_INDEX_PATH);
        SimpleLogger().Write(logDEBUG) << "CORE_MARKER          "
                                       << ": " << GetBlockSize(CORE_MARKER);
        SimpleLogger().Write(logDEBUG) << "TRAFFIC_SEGMENT_ID_LIST         "
                                       << ": " << GetBlockSize(TRAFFIC_SEGMENT_ID_LIST);
    }

    template <typename T> inline void SetBlockSize(BlockID bid, uint64_t entries)
    {
        num_entries[bid] = entries;
        entry_size[bid] = sizeof(T);
    }

    inline uint64_t GetBlockSize(BlockID bid) const
    {
        // special bit encoding
        if (bid == GEOMETRIES_INDICATORS || bid == CORE_MARKER)
        {
            return (num_entries[bid] / 32 + 1) *
                   entry_size[bid];
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
                throw osrm::exception("Start canary of block corrupted.");
            }
            if (!end_canary_alive)
            {
                throw osrm::exception("End canary of block corrupted.");
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

#endif /* SHARED_DATA_TYPE_HPP */

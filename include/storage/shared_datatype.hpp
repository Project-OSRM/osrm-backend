#ifndef SHARED_DATA_TYPE_HPP
#define SHARED_DATA_TYPE_HPP

#include "storage/block.hpp"
#include "storage/io_fwd.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/log.hpp"

#include <boost/assert.hpp>

#include <array>
#include <cstdint>
#include <map>

namespace osrm
{
namespace storage
{

class DataLayout;
namespace serialization
{
inline void read(io::BufferReader &reader, DataLayout &layout);

inline void write(io::BufferWriter &writer, const DataLayout &layout);
}

// Added at the start and end of each block as sanity check
const constexpr char CANARY[4] = {'O', 'S', 'R', 'M'};

class DataLayout
{
  public:
    DataLayout() : blocks{} {}

    inline void SetBlock(const std::string &name, Block block) { blocks[name] = std::move(block); }

    inline uint64_t GetBlockEntries(const std::string &name) const
    {
        auto iter = blocks.find(name);
        if (iter == blocks.end())
        {
            throw util::exception("Could not find block " + name);
        }

        return iter->second.num_entries;
    }

    inline uint64_t GetBlockSize(const std::string &name) const
    {
        auto iter = blocks.find(name);
        if (iter == blocks.end())
        {
            throw util::exception("Could not find block " + name);
        }

        return iter->second.byte_size;
    }

    inline uint64_t GetSizeOfLayout() const
    {
        uint64_t result = 0;
        for (const auto &name_and_block : blocks)
        {
            result += 2 * sizeof(CANARY) + GetBlockSize(name_and_block.first) + BLOCK_ALIGNMENT;
        }
        return result;
    }

    template <typename T, bool WRITE_CANARY = false>
    inline T *GetBlockPtr(char *shared_memory, const std::string &name) const
    {
        static_assert(BLOCK_ALIGNMENT % std::alignment_of<T>::value == 0,
                      "Datatype does not fit alignment constraints.");

        char *ptr = (char *)GetAlignedBlockPtr(shared_memory, name);
        if (WRITE_CANARY)
        {
            char *start_canary_ptr = ptr - sizeof(CANARY);
            char *end_canary_ptr = ptr + GetBlockSize(name);
            std::copy(CANARY, CANARY + sizeof(CANARY), start_canary_ptr);
            std::copy(CANARY, CANARY + sizeof(CANARY), end_canary_ptr);
        }
        else
        {
            char *start_canary_ptr = ptr - sizeof(CANARY);
            char *end_canary_ptr = ptr + GetBlockSize(name);
            bool start_canary_alive = std::equal(CANARY, CANARY + sizeof(CANARY), start_canary_ptr);
            bool end_canary_alive = std::equal(CANARY, CANARY + sizeof(CANARY), end_canary_ptr);
            if (!start_canary_alive)
            {
                throw util::exception("Start canary of block corrupted. (" + name + ")" +
                                      SOURCE_REF);
            }
            if (!end_canary_alive)
            {
                throw util::exception("End canary of block corrupted. (" + name + ")" + SOURCE_REF);
            }
        }

        return (T *)ptr;
    }

  private:
    friend void serialization::read(io::BufferReader &reader, DataLayout &layout);
    friend void serialization::write(io::BufferWriter &writer, const DataLayout &layout);

    // Fit aligned storage in buffer to 64 bytes to conform with AVX 512 types
    inline void *align(void *&ptr) const noexcept
    {
        const auto intptr = reinterpret_cast<uintptr_t>(ptr);
        const auto aligned = (intptr - 1u + BLOCK_ALIGNMENT) & -BLOCK_ALIGNMENT;
        return ptr = reinterpret_cast<void *>(aligned);
    }

    inline void *GetAlignedBlockPtr(void *ptr, const std::string &name) const
    {
        for (auto iter = blocks.begin(); iter != blocks.end() && iter->first != name; ++iter)
        {
            ptr = static_cast<char *>(ptr) + sizeof(CANARY);
            ptr = align(ptr);
            ptr = static_cast<char *>(ptr) + GetBlockSize(name);
            ptr = static_cast<char *>(ptr) + sizeof(CANARY);
        }

        ptr = static_cast<char *>(ptr) + sizeof(CANARY);
        ptr = align(ptr);
        return ptr;
    }

    template <typename T> inline T *GetBlockEnd(char *shared_memory, const std::string &name) const
    {
        auto begin = GetBlockPtr<T>(shared_memory, name);
        return begin + GetBlockEntries(name);
    }

    static constexpr std::size_t BLOCK_ALIGNMENT = 64;
    std::map<std::string, Block> blocks;
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
}
}

#endif /* SHARED_DATA_TYPE_HPP */

#ifndef SHARED_DATA_TYPE_HPP
#define SHARED_DATA_TYPE_HPP

#include "storage/block.hpp"
#include "storage/io_fwd.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"

#include <boost/assert.hpp>

#include <array>
#include <cstdint>
#include <map>
#include <numeric>
#include <unordered_set>

namespace osrm
{
namespace storage
{

class BaseDataLayout;
namespace serialization
{
inline void read(io::BufferReader &reader, BaseDataLayout &layout);

inline void write(io::BufferWriter &writer, const BaseDataLayout &layout);
} // namespace serialization

namespace detail
{
// Removes the file name if name_prefix is a directory and name is not a file in that directory
inline std::string trimName(const std::string &name_prefix, const std::string &name)
{
    // list directory and
    if (!name_prefix.empty() && name_prefix.back() == '/')
    {
        auto directory_position = name.find_first_of("/", name_prefix.length());
        // this is a "file" in the directory of name_prefix
        if (directory_position == std::string::npos)
        {
            return name;
        }
        else
        {
            return name.substr(0, directory_position);
        }
    }
    else
    {
        return name;
    }
}
} // namespace detail

class BaseDataLayout
{
  public:
    virtual ~BaseDataLayout() = default;

    inline void SetBlock(const std::string &name, Block block) { blocks[name] = std::move(block); }

    inline std::uint64_t GetBlockEntries(const std::string &name) const
    {
        return GetBlock(name).num_entries;
    }

    inline std::uint64_t GetBlockSize(const std::string &name) const
    {
        return GetBlock(name).byte_size;
    }

    inline bool HasBlock(const std::string &name) const
    {
        return blocks.find(name) != blocks.end();
    }

    // Depending on the name prefix this function either lists all blocks with the same prefix
    // or all entries in the sub-directory.
    // '/ch/edge' -> '/ch/edge_filter/0/blocks', '/ch/edge_filter/1/blocks'
    // '/ch/edge_filters/' -> '/ch/edge_filter/0', '/ch/edge_filter/1'
    template <typename OutIter> void List(const std::string &name_prefix, OutIter out) const
    {
        std::unordered_set<std::string> returned_name;

        for (const auto &pair : blocks)
        {
            // check if string begins with the name prefix
            if (pair.first.find(name_prefix) == 0)
            {
                auto trimmed_name = detail::trimName(name_prefix, pair.first);
                auto ret = returned_name.insert(trimmed_name);
                if (ret.second)
                {
                    *out++ = trimmed_name;
                }
            }
        }
    }

    virtual inline void *GetBlockPtr(void *base_ptr, const std::string &name) const = 0;
    virtual inline std::uint64_t GetSizeOfLayout() const = 0;

  protected:
    const Block &GetBlock(const std::string &name) const
    {
        auto iter = blocks.find(name);
        if (iter == blocks.end())
        {
            throw util::exception("Could not find block " + name);
        }

        return iter->second;
    }

    friend void serialization::read(io::BufferReader &reader, BaseDataLayout &layout);
    friend void serialization::write(io::BufferWriter &writer, const BaseDataLayout &layout);

    std::map<std::string, Block> blocks;
};

class ContiguousDataLayout final : public BaseDataLayout
{
  public:
    inline std::uint64_t GetSizeOfLayout() const override final
    {
        std::uint64_t result = 0;
        for (const auto &name_and_block : blocks)
        {
            result += GetBlockSize(name_and_block.first) + BLOCK_ALIGNMENT;
        }
        return result;
    }

    inline void *GetBlockPtr(void *base_ptr, const std::string &name) const override final
    {
        // TODO: re-enable this alignment checking somehow
        // static_assert(BLOCK_ALIGNMENT % std::alignment_of<T>::value == 0,
        //               "Datatype does not fit alignment constraints.");

        return GetAlignedBlockPtr(base_ptr, name);
    }

  private:
    friend void serialization::read(io::BufferReader &reader, BaseDataLayout &layout);
    friend void serialization::write(io::BufferWriter &writer, const BaseDataLayout &layout);

    // Fit aligned storage in buffer to 64 bytes to conform with AVX 512 types
    inline void *align(void *&ptr) const noexcept
    {
        const auto intptr = reinterpret_cast<std::uintptr_t>(ptr);
        const auto aligned = (intptr - 1u + BLOCK_ALIGNMENT) & -BLOCK_ALIGNMENT;
        return ptr = reinterpret_cast<void *>(aligned);
    }

    inline void *GetAlignedBlockPtr(void *ptr, const std::string &name) const
    {
        auto block_iter = blocks.find(name);
        if (block_iter == blocks.end())
        {
            throw util::exception("Could not find block " + name);
        }

        for (auto iter = blocks.begin(); iter != block_iter; ++iter)
        {
            ptr = align(ptr);
            ptr = static_cast<char *>(ptr) + iter->second.byte_size;
        }

        ptr = align(ptr);
        return ptr;
    }

    static constexpr std::size_t BLOCK_ALIGNMENT = 64;
};

class TarDataLayout final : public BaseDataLayout
{
  public:
    inline std::uint64_t GetSizeOfLayout() const override final
    {
        std::uint64_t result = 0;
        for (const auto &name_and_block : blocks)
        {
            result += GetBlockSize(name_and_block.first);
        }
        return result;
    }

    inline void *GetBlockPtr(void *base_ptr, const std::string &name) const override final
    {
        auto offset = GetBlock(name).offset;
        const auto offset_address = reinterpret_cast<std::uintptr_t>(base_ptr) + offset;
        return reinterpret_cast<void *>(offset_address);
    }
};

struct SharedRegion
{
    static constexpr const int MAX_NAME_LENGTH = 254;

    SharedRegion() : name{0}, timestamp{0} {}
    SharedRegion(const std::string &name_, std::uint64_t timestamp, std::uint16_t shm_key)
        : name{0}, timestamp{timestamp}, shm_key{shm_key}
    {
        std::copy_n(name_.begin(), std::min<std::size_t>(MAX_NAME_LENGTH, name_.size()), name);
    }

    bool IsEmpty() const { return timestamp == 0; }

    char name[MAX_NAME_LENGTH + 1];
    std::uint64_t timestamp;
    std::uint16_t shm_key;
};

// Keeps a list of all shared regions in a fixed-sized struct
// for fast access and deserialization.
struct SharedRegionRegister
{
    using RegionID = std::uint16_t;
    static constexpr const RegionID INVALID_REGION_ID = std::numeric_limits<RegionID>::max();
    using ShmKey = decltype(SharedRegion::shm_key);

    // Returns the key of the region with the given name
    RegionID Find(const std::string &name) const
    {
        auto iter = std::find_if(regions.begin(), regions.end(), [&](const auto &region) {
            return std::strncmp(region.name, name.c_str(), SharedRegion::MAX_NAME_LENGTH) == 0;
        });

        if (iter == regions.end())
        {
            return INVALID_REGION_ID;
        }
        else
        {
            return std::distance(regions.begin(), iter);
        }
    }

    RegionID Register(const std::string &name, ShmKey key)
    {
        auto iter = std::find_if(
            regions.begin(), regions.end(), [&](const auto &region) { return region.IsEmpty(); });
        if (iter == regions.end())
        {
            throw util::exception("No shared memory regions left. Could not register " + name +
                                  ".");
        }
        else
        {
            constexpr std::uint32_t INITIAL_TIMESTAMP = 1;
            *iter = SharedRegion{name, INITIAL_TIMESTAMP, key};
            RegionID key = std::distance(regions.begin(), iter);
            return key;
        }
    }

    template <typename OutIter> void List(OutIter out) const
    {
        for (const auto &region : regions)
        {
            if (!region.IsEmpty())
            {
                *out++ = region.name;
            }
        }
    }

    const auto &GetRegion(const RegionID key) const { return regions[key]; }

    auto &GetRegion(const RegionID key) { return regions[key]; }

    ShmKey ReserveKey()
    {
        auto free_key_iter = std::find(shm_key_in_use.begin(), shm_key_in_use.end(), false);
        if (free_key_iter == shm_key_in_use.end())
        {
            throw util::exception("Could not reserve a new SHM key. All keys are in use");
        }

        *free_key_iter = true;
        return std::distance(shm_key_in_use.begin(), free_key_iter);
    }

    void ReleaseKey(ShmKey key) { shm_key_in_use[key] = false; }

    static constexpr const std::size_t MAX_SHARED_REGIONS = 512;
    static_assert(MAX_SHARED_REGIONS < std::numeric_limits<RegionID>::max(),
                  "Number of shared memory regions needs to be less than the region id size.");

    static constexpr const std::size_t MAX_SHM_KEYS = MAX_SHARED_REGIONS * 2;

    static constexpr const char *name = "osrm-region";

  private:
    std::array<SharedRegion, MAX_SHARED_REGIONS> regions;
    std::array<bool, MAX_SHM_KEYS> shm_key_in_use;
};
} // namespace storage
} // namespace osrm

#endif /* SHARED_DATA_TYPE_HPP */

#ifndef OSRM_STORAGE_BLOCK_HPP
#define OSRM_STORAGE_BLOCK_HPP

#include "storage/io.hpp"

#include <cstdint>
#include <string>
#include <tuple>

namespace osrm
{
namespace storage
{

struct Block
{
    std::uint64_t num_entries;
    std::uint64_t byte_size;
    std::uint64_t offset;

    Block() : num_entries(0), byte_size(0), offset(0) {}
    Block(std::uint64_t num_entries, std::uint64_t byte_size, std::uint64_t offset)
        : num_entries(num_entries), byte_size(byte_size), offset(offset)
    {
    }
    Block(std::uint64_t num_entries, std::uint64_t byte_size)
        : num_entries(num_entries), byte_size(byte_size), offset(0)
    {
    }
};

using NamedBlock = std::tuple<std::string, Block>;

template <typename T> Block make_block(uint64_t num_entries)
{
    static_assert(sizeof(T) % alignof(T) == 0, "aligned T* can't be used as an array pointer");
    return Block{num_entries, sizeof(T) * num_entries, 0};
}
}
}

#endif

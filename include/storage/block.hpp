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
    std::uint64_t entry_size;
    std::uint64_t entry_align;
};

template <typename T> Block make_block(uint64_t num_entries)
{
    static_assert(sizeof(T) % alignof(T) == 0, "aligned T* can't be used as an array pointer");
    return Block{num_entries, sizeof(T) * num_entries, sizeof(T), alignof(T)};
}
}
}

#endif

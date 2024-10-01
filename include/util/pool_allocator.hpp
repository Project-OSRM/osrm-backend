#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <boost/assert.hpp>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <new>
#include <vector>

namespace osrm::util
{

inline size_t align_up(size_t n, size_t alignment)
{
    return (n + alignment - 1) & ~(alignment - 1);
}

inline size_t get_next_power_of_two_exponent(size_t n)
{
    BOOST_ASSERT(n > 0);
    return (sizeof(size_t) * 8) - std::countl_zero(n - 1);
}

class MemoryPool
{
  private:
    constexpr static size_t MIN_CHUNK_SIZE_BYTES = 4096;

  public:
    static std::shared_ptr<MemoryPool> instance()
    {
        static thread_local std::shared_ptr<MemoryPool> instance;
        if (!instance)
        {
            instance = std::shared_ptr<MemoryPool>(new MemoryPool());
        }
        return instance;
    }

    template <typename T> T *allocate(std::size_t items_count)
    {
        static_assert(alignof(T) <= alignof(std::max_align_t),
                      "Type is over-aligned for this allocator.");

        size_t free_list_index = get_next_power_of_two_exponent(items_count * sizeof(T));
        auto &free_list = free_lists_[free_list_index];
        if (free_list.empty())
        {
            size_t block_size_in_bytes = 1u << free_list_index;
            block_size_in_bytes = align_up(block_size_in_bytes, alignof(std::max_align_t));
            // check if there is space in current memory chunk
            if (current_chunk_left_bytes_ < block_size_in_bytes)
            {
                allocate_chunk(block_size_in_bytes);
            }

            free_list.push_back(current_chunk_ptr_);
            current_chunk_left_bytes_ -= block_size_in_bytes;
            current_chunk_ptr_ += block_size_in_bytes;
        }
        auto ptr = reinterpret_cast<T *>(free_list.back());
        free_list.pop_back();
        return ptr;
    }

    template <typename T> void deallocate(T *p, std::size_t n) noexcept
    {
        size_t free_list_index = get_next_power_of_two_exponent(n * sizeof(T));
        // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
        free_lists_[free_list_index].push_back(reinterpret_cast<void *>(p));
    }

    ~MemoryPool()
    {
        for (auto chunk : chunks_)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
            std::free(chunk);
        }
    }

  private:
    MemoryPool() = default;
    MemoryPool(const MemoryPool &) = delete;
    MemoryPool &operator=(const MemoryPool &) = delete;

    void allocate_chunk(size_t bytes)
    {
        auto chunk_size = std::max(bytes, MIN_CHUNK_SIZE_BYTES);
        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
        void *chunk = std::malloc(chunk_size);
        if (!chunk)
        {
            throw std::bad_alloc();
        }
        chunks_.push_back(chunk);
        current_chunk_ptr_ = static_cast<uint8_t *>(chunk);
        current_chunk_left_bytes_ = chunk_size;
    }

    // we have 64 free lists, one for each possible power of two
    std::array<std::vector<void *>, sizeof(std::size_t) * 8> free_lists_;

    // list of allocated memory chunks, we don't free them until the pool is destroyed
    std::vector<void *> chunks_;

    uint8_t *current_chunk_ptr_ = nullptr;
    size_t current_chunk_left_bytes_ = 0;
};

template <typename T> class PoolAllocator
{
  public:
    using value_type = T;

    PoolAllocator() noexcept : pool(MemoryPool::instance()){};

    template <typename U>
    PoolAllocator(const PoolAllocator<U> &) noexcept : pool(MemoryPool::instance())
    {
    }

    template <typename U> struct rebind
    {
        using other = PoolAllocator<U>;
    };

    T *allocate(std::size_t n) { return pool->allocate<T>(n); }

    void deallocate(T *p, std::size_t n) noexcept { pool->deallocate<T>(p, n); }

    PoolAllocator(const PoolAllocator &) = default;
    PoolAllocator &operator=(const PoolAllocator &) = default;
    PoolAllocator(PoolAllocator &&) noexcept = default;
    PoolAllocator &operator=(PoolAllocator &&) noexcept = default;

  private:
    // using shared_ptr guarantees that memory pool won't be destroyed before all allocators using
    // it (important if there are static instances of PoolAllocator)
    std::shared_ptr<MemoryPool> pool;
};
template <typename T, typename U>
bool operator==(const PoolAllocator<T> &, const PoolAllocator<U> &)
{
    return true;
}

template <typename T, typename U>
bool operator!=(const PoolAllocator<T> &, const PoolAllocator<U> &)
{
    return false;
}

} // namespace osrm::util

#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <boost/assert.hpp>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <new>
#include <vector>
#include <memory>

namespace osrm::util
{

template <typename T>
class PoolAllocator;

template <typename T>
class MemoryManager
{
private:
    constexpr static size_t MIN_ITEMS_IN_BLOCK = 1024;
public:
    static std::shared_ptr<MemoryManager> instance()
    {
        static thread_local std::shared_ptr<MemoryManager> instance;
        if (!instance)
        {
            instance = std::shared_ptr<MemoryManager>(new MemoryManager());
        }
        return instance;
    }

    T *allocate(std::size_t n)
    {
        size_t free_list_index = get_next_power_of_two_exponent(n);
        auto &free_list = free_lists_[free_list_index];
        const auto items_in_block = 1u << free_list_index;
        if (free_list.empty())
        {
            // Check if there is space in current block
            if (current_block_left_items_ < items_in_block)
            {
                allocate_block(items_in_block);
            }

            free_list.push_back(current_block_ptr_);
            current_block_left_items_ -= items_in_block;
            current_block_ptr_ += items_in_block;
        }
        auto ptr = free_list.back();
        free_list.pop_back();
        return ptr;
    }

    void deallocate(T *p, std::size_t n) noexcept
    {
        size_t free_list_index = get_next_power_of_two_exponent(n);
        free_lists_[free_list_index].push_back(p);
    }

    ~MemoryManager()
    {
       // std::cerr << "~MemoryManager()" << std::endl;
        for (auto block : blocks_)
        {
            std::free(block);
        }
    }
private:
    MemoryManager() = default;
    MemoryManager(const MemoryManager &) = delete;
    MemoryManager &operator=(const MemoryManager &) = delete;

    size_t get_next_power_of_two_exponent(size_t n) const
    {
        BOOST_ASSERT(n > 0);
        return (sizeof(size_t) * 8) - std::countl_zero(n - 1);
    }

    void allocate_block(size_t items_in_block)
    {
        items_in_block = std::max(items_in_block, MIN_ITEMS_IN_BLOCK);

        size_t block_size = items_in_block * sizeof(T);
        T *block = static_cast<T *>(std::malloc(block_size));
        if (!block)
        {
            throw std::bad_alloc();
        }
        total_allocated_ += block_size;
        blocks_.push_back(block);
        current_block_ptr_ = block;
        current_block_left_items_ = items_in_block;
    }

    std::array<std::vector<T *>, 32> free_lists_;
    std::vector<T *> blocks_;
    T *current_block_ptr_ = nullptr;
    size_t current_block_left_items_ = 0;

    size_t total_allocated_ = 0;
};

template <typename T>
class PoolAllocator
{
public:
    using value_type = T;

    PoolAllocator() noexcept : pool(MemoryManager<T>::instance()) {};

    template <typename U>
    PoolAllocator(const PoolAllocator<U> &) noexcept : pool(MemoryManager<T>::instance()) {}

    template <typename U>
    struct rebind
    {
        using other = PoolAllocator<U>;
    };

    T *allocate(std::size_t n)
    {
        return pool->allocate(n);
    }

    void deallocate(T *p, std::size_t n) noexcept
    {
        pool->deallocate(p, n);
    }

    ~PoolAllocator() = default;

    PoolAllocator(const PoolAllocator &) = default;
    PoolAllocator &operator=(const PoolAllocator &) = default;
    PoolAllocator(PoolAllocator &&) noexcept = default;
    PoolAllocator &operator=(PoolAllocator &&) noexcept = default;

private:
    std::shared_ptr<MemoryManager<T>> pool;
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

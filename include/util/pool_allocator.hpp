#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <boost/assert.hpp>
#include <cstdlib>
#include <iostream>
#include <new>
#include <vector>

namespace osrm::util
{

template <typename T> class PoolAllocator
{
  public:
    using value_type = T;

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

    ~PoolAllocator()
    {
        for (auto block : blocks_)
        {
            std::free(block);
        }
    }

  private:
    size_t get_next_power_of_two_exponent(size_t n) const
    {
        return std::countr_zero(std::bit_ceil(n));
    }

    void allocate_block(size_t items_in_block)
    {
        size_t block_size = std::max(items_in_block, (size_t)256) * sizeof(T);
        T *block = static_cast<T *>(std::malloc(block_size));
        if (!block)
        {
            throw std::bad_alloc();
        }
        blocks_.push_back(block);
        current_block_ = block;
        current_block_ptr_ = block;
        current_block_left_items_ = block_size / sizeof(T);
    }

    std::array<std::vector<T *>, 32> free_lists_;
    std::vector<T *> blocks_;
    T *current_block_ = nullptr;
    T *current_block_ptr_ = nullptr;
    size_t current_block_left_items_ = 0;
};

} // namespace osrm::util

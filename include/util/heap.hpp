// Copyright Malte Skarupke 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>
#include <functional>
#include <utility>

namespace minmax_heap_helpers
{
// returns the index of the highest set bit. undefined if no bits are set.
// examples:
// highest_set_bit(1) = 0
// highest_set_bit(4) = 2
// highest_set_bit(55) = 5
inline int highest_set_bit(uint64_t i)
{
#ifdef _MSC_VER
    unsigned long result;
    _BitScanReverse64(&result, i);
    return result;
#else
    return 63 - __builtin_clzl(i);
#endif
}

inline bool is_new_item_min(uint64_t length) { return (highest_set_bit(length) & 1) == 0; }

inline bool is_min_item(uint64_t index) { return is_new_item_min(index + 1); }

inline uint64_t grandparent_index(uint64_t index) { return (index - 3) / 4; }

inline uint64_t parent_index(uint64_t index) { return (index - 1) / 2; }

inline uint64_t first_child_index(uint64_t index) { return (index * 2) + 1; }
inline uint64_t last_grandchild_index(uint64_t index) { return (index * 4) + 6; }
template <typename It, typename Compare>
uint64_t smallest_descendant(
    It begin, uint64_t length, uint64_t first_child, uint64_t first_grandchild, Compare &&compare)
{
    uint64_t second_child = first_child + 1;
    if (first_grandchild >= length)
        return first_child +
               (second_child != length && compare(begin[second_child], begin[first_child]));
    uint64_t second_grandchild = first_grandchild + 1;
    if (second_grandchild == length)
        return compare(begin[first_grandchild], begin[second_child]) ? first_grandchild
                                                                     : second_child;
    uint64_t min_grandchild =
        first_grandchild + !!compare(begin[second_grandchild], begin[first_grandchild]);
    uint64_t third_grandchild = second_grandchild + 1;
    if (third_grandchild == length)
        return compare(begin[min_grandchild], begin[second_child]) ? min_grandchild : second_child;
    else
        return compare(begin[min_grandchild], begin[third_grandchild]) ? min_grandchild
                                                                       : third_grandchild;
}
template <typename It, typename Compare>
uint64_t largest_descendant(
    It begin, uint64_t length, uint64_t first_child, uint64_t first_grandchild, Compare &&compare)
{
    uint64_t second_child = first_child + 1;
    if (first_grandchild >= length)
        return first_child +
               (second_child != length && compare(begin[first_child], begin[second_child]));
    uint64_t second_grandchild = first_grandchild + 1;
    if (second_grandchild == length)
        return compare(begin[second_child], begin[first_grandchild]) ? first_grandchild
                                                                     : second_child;
    uint64_t max_grandchild =
        first_grandchild + !!compare(begin[first_grandchild], begin[second_grandchild]);
    uint64_t third_grandchild = second_grandchild + 1;
    if (third_grandchild == length)
        return compare(begin[second_child], begin[max_grandchild]) ? max_grandchild : second_child;
    else
        return compare(begin[max_grandchild], begin[third_grandchild]) ? third_grandchild
                                                                       : max_grandchild;
}

template <typename It, typename Compare>
void push_down_min(It begin,
                   typename std::iterator_traits<It>::value_type value,
                   uint64_t index,
                   uint64_t length,
                   Compare &&compare)
{
    using std::swap;
    for (;;)
    {
        uint64_t last_grandchild = last_grandchild_index(index);
        if (last_grandchild < length)
        {
            auto it = begin + last_grandchild;
            uint64_t min_first_half = last_grandchild - 2 - !!compare(it[-3], it[-2]);
            uint64_t min_second_half = last_grandchild - !!compare(it[-1], it[0]);
            uint64_t smallest = compare(begin[min_second_half], begin[min_first_half])
                                    ? min_second_half
                                    : min_first_half;
            if (!compare(begin[smallest], value))
                break;
            begin[index] = std::move(begin[smallest]);
            index = smallest;
            uint64_t parent = parent_index(index);
            if (compare(begin[parent], value))
                swap(begin[parent], value);
        }
        else
        {
            uint64_t first_child = first_child_index(index);
            if (first_child >= length)
                break;
            uint64_t first_grandchild = last_grandchild - 3;
            uint64_t smallest =
                smallest_descendant(begin, length, first_child, first_grandchild, compare);
            if (!compare(begin[smallest], value))
                break;
            begin[index] = std::move(begin[smallest]);
            index = smallest;
            if (smallest < first_grandchild)
                break;
            uint64_t parent = parent_index(index);
            if (compare(begin[parent], value))
            {
                begin[index] = std::move(begin[parent]);
                index = parent;
            }
            break;
        }
    }
    begin[index] = std::move(value);
}

template <typename It, typename Compare>
void push_down_min_one_child_only(It begin, uint64_t index, Compare &&compare)
{
    using std::swap;
    uint64_t child = first_child_index(index);
    if (compare(begin[child], begin[index]))
        swap(begin[index], begin[child]);
}

template <typename It, typename Compare>
void push_down_min_one_level_only(It begin, uint64_t index, Compare &&compare)
{
    using std::swap;
    uint64_t first_child = first_child_index(index);
    uint64_t smaller_child = first_child + !!compare(begin[first_child + 1], begin[first_child]);
    if (compare(begin[smaller_child], begin[index]))
        swap(begin[index], begin[smaller_child]);
}

template <typename It, typename Compare>
void push_down_max(It begin,
                   typename std::iterator_traits<It>::value_type value,
                   uint64_t index,
                   uint64_t length,
                   Compare &&compare)
{
    using std::swap;
    for (;;)
    {
        uint64_t last_grandchild = last_grandchild_index(index);
        if (last_grandchild < length)
        {
            auto it = begin + last_grandchild;
            uint64_t max_first_half = last_grandchild - 2 - !!compare(it[-2], it[-3]);
            uint64_t max_second_half = last_grandchild - !!compare(it[0], it[-1]);
            uint64_t largest = compare(begin[max_first_half], begin[max_second_half])
                                   ? max_second_half
                                   : max_first_half;
            if (!compare(value, begin[largest]))
                break;
            begin[index] = std::move(begin[largest]);
            index = largest;
            uint64_t parent = parent_index(index);
            if (compare(value, begin[parent]))
                swap(begin[parent], value);
        }
        else
        {
            uint64_t first_child = first_child_index(index);
            if (first_child >= length)
                break;
            uint64_t first_grandchild = last_grandchild - 3;
            uint64_t largest =
                largest_descendant(begin, length, first_child, first_grandchild, compare);
            if (!compare(value, begin[largest]))
                break;
            begin[index] = std::move(begin[largest]);
            index = largest;
            if (largest < first_grandchild)
                break;
            uint64_t parent = parent_index(index);
            if (compare(value, begin[parent]))
            {
                begin[index] = std::move(begin[parent]);
                index = parent;
            }
            break;
        }
    }
    begin[index] = std::move(value);
}

template <typename It, typename Compare>
void push_down_max_one_child_only(It begin, uint64_t index, Compare &&compare)
{
    using std::swap;
    uint64_t child = first_child_index(index);
    if (compare(begin[index], begin[child]))
        swap(begin[index], begin[child]);
}

template <typename It, typename Compare>
void push_down_max_one_level_only(It begin, uint64_t index, Compare &&compare)
{
    using std::swap;
    uint64_t first_child = first_child_index(index);
    uint64_t bigger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
    if (compare(begin[index], begin[bigger_child]))
        swap(begin[index], begin[bigger_child]);
}

} // namespace minmax_heap_helpers

template <typename It, typename Compare> bool is_minmax_heap(It begin, It end, Compare &&compare)
{
    uint64_t length = static_cast<uint64_t>(end - begin);
    auto test_index = [](uint64_t index, auto compare_index)
    {
        uint64_t first_child = minmax_heap_helpers::first_child_index(index);
        uint64_t second_child = first_child + 1;
        uint64_t first_grandchild = minmax_heap_helpers::first_child_index(first_child);
        uint64_t second_grandchild = first_grandchild + 1;
        uint64_t third_grandchild = minmax_heap_helpers::first_child_index(second_child);
        uint64_t fourth_grandchild = third_grandchild + 1;
        return compare_index(first_child) && compare_index(second_child) &&
               compare_index(first_grandchild) && compare_index(second_grandchild) &&
               compare_index(third_grandchild) && compare_index(fourth_grandchild);
    };
    for (uint64_t i = 0; i < length; ++i)
    {
        if (minmax_heap_helpers::is_min_item(i))
        {
            auto compare_one = [&](uint64_t child)
            { return child >= length || !compare(begin[child], begin[i]); };
            if (!test_index(i, compare_one))
                return false;
        }
        else
        {
            auto compare_one = [&](uint64_t child)
            { return child >= length || !compare(begin[i], begin[child]); };
            if (!test_index(i, compare_one))
                return false;
        }
    }
    return true;
}
template <typename It> bool is_minmax_heap(It begin, It end)
{
    return is_minmax_heap(begin, end, std::less<>{});
}

template <typename It, typename Compare> void push_minmax_heap(It begin, It end, Compare &&compare)
{
    uint64_t length = static_cast<uint64_t>(end - begin);
    uint64_t index = length - 1;
    uint64_t parent = minmax_heap_helpers::parent_index(index);
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    if (minmax_heap_helpers::is_new_item_min(length))
    {
        if (index == 0)
            static_cast<void>(0);
        else if (compare(begin[parent], value))
        {
            begin[index] = std::move(begin[parent]);
            index = parent;
            goto push_up_max;
        }
        else
        {
            for (;;)
            {
                {
                    uint64_t grandparent = minmax_heap_helpers::grandparent_index(index);
                    if (compare(value, begin[grandparent]))
                    {
                        begin[index] = std::move(begin[grandparent]);
                        index = grandparent;
                    }
                    else
                        break;
                }
            push_up_min:
                if (!index)
                    break;
            }
        }
    }
    else if (compare(value, begin[parent]))
    {
        begin[index] = std::move(begin[parent]);
        index = parent;
        goto push_up_min;
    }
    else
    {
    push_up_max:
        while (index > 2)
        {
            uint64_t grandparent = minmax_heap_helpers::grandparent_index(index);
            if (compare(begin[grandparent], value))
            {
                begin[index] = std::move(begin[grandparent]);
                index = grandparent;
            }
            else
                break;
        }
    }
    begin[index] = std::move(value);
}
template <typename It> void push_minmax_heap(It begin, It end)
{
    push_minmax_heap(begin, end, std::less<>{});
}

template <typename It, typename Compare>
void pop_minmax_heap_min(It begin, It end, Compare &&compare)
{
    uint64_t length = static_cast<uint64_t>(end - begin) - 1;
    if (length == 0)
        return;
    minmax_heap_helpers::push_down_min(
        begin, std::exchange(end[-1], std::move(begin[0])), 0, length, compare);
}

template <typename It> void pop_minmax_heap_min(It begin, It end)
{
    pop_minmax_heap_min(begin, end, std::less<>{});
}

template <typename It, typename Compare>
void pop_minmax_heap_max(It begin, It end, Compare &&compare)
{
    uint64_t length = static_cast<uint64_t>(end - begin) - 1;
    if (length <= 1)
        return;

    uint64_t index = 1 + !!compare(begin[1], begin[2]);
    minmax_heap_helpers::push_down_max(begin,
                                       std::exchange(end[-1], std::move(begin[index])),
                                       index,
                                       length,
                                       std::forward<Compare>(compare));
}
template <typename It> void pop_minmax_heap_max(It begin, It end)
{
    pop_minmax_heap_max(begin, end, std::less<>{});
}

template <typename It, typename Compare> void make_minmax_heap(It begin, It end, Compare &&compare)
{
    uint64_t length = end - begin;
    uint64_t index = length / 2;
    if (index == 0)
        return;
    // optimization: there can be only one item that has only one child
    // handling that item up front simplifies the second loop a little, since
    // we know that all other items have two children
    if ((length & 1) == 0)
    {
        --index;
        if (minmax_heap_helpers::is_min_item(index))
            minmax_heap_helpers::push_down_min_one_child_only(begin, index, compare);
        else
            minmax_heap_helpers::push_down_max_one_child_only(begin, index, compare);
        if (index == 0)
            return;
    }
    // optimization: half of all the items will have no grandchildren. this
    // simplifies the push_down function a lot, so we handle these items
    // first. we could then do another optimization where we know that
    // after the first half, the next quarter of items has grandchildren but
    // no great-grandchildren, but the code is already big enough
    if (length != 4)
    {
        uint64_t lowest_index_with_no_grandchildren = length / 4;
        for (;;)
        {
            int highest_bit = minmax_heap_helpers::highest_set_bit(index);
            uint64_t loop_until = std::max(lowest_index_with_no_grandchildren,
                                           (static_cast<uint64_t>(1) << highest_bit) - 1);
            --index;
            if (highest_bit & 1)
            {
                for (;; --index)
                {
                    minmax_heap_helpers::push_down_max_one_level_only(begin, index, compare);
                    if (index == loop_until)
                        break;
                }
            }
            else
            {
                for (;; --index)
                {
                    minmax_heap_helpers::push_down_min_one_level_only(begin, index, compare);
                    if (index == loop_until)
                        break;
                }
                if (index == 0)
                    return;
            }
            if (index == lowest_index_with_no_grandchildren)
                break;
        }
    }
    int highest_bit = minmax_heap_helpers::highest_set_bit(index);
    uint64_t loop_until = (static_cast<uint64_t>(1) << highest_bit) - 1;
    switch (highest_bit & 1)
    {
        for (;;)
        {
        case 0:
            for (;;)
            {
                --index;
                minmax_heap_helpers::push_down_min(
                    begin, std::move(begin[index]), index, length, compare);
                if (index == loop_until)
                    break;
            }
            if (index == 0)
                return;
            loop_until /= 2;
            [[fallthrough]];
        case 1:
            for (;;)
            {
                --index;
                minmax_heap_helpers::push_down_max(
                    begin, std::move(begin[index]), index, length, compare);
                if (index == loop_until)
                    break;
            }
            loop_until /= 2;
        }
    }
}
template <typename It> void make_minmax_heap(It begin, It end)
{
    return make_minmax_heap(begin, end, std::less<>{});
}

namespace dary_heap_helpers
{
template <int D> uint64_t first_child_index(uint64_t index) { return index * D + 1; }
template <int D> uint64_t last_child_index(uint64_t index) { return index * D + D; }
template <int D> uint64_t last_grandchild_index(uint64_t index)
{
    return index * (D * D) + (D * D + D);
}
template <int D> uint64_t parent_index(uint64_t index) { return (index - 1) / D; }
template <int D> uint64_t grandparent_index(uint64_t index) { return (index - (D + 1)) / (D * D); }
template <int D> uint64_t index_with_no_grandchild(uint64_t length)
{
    return grandparent_index<D>(length - 1) + 1;
}
template <int D, typename It, typename Compare>
inline It largest_child(It first_child_it, Compare &&compare)
{
    if constexpr (D == 1)
        return first_child_it;
    else if constexpr (D == 2)
        return first_child_it + !!compare(first_child_it[0], first_child_it[1]);
    else
    {
        It first_half_largest = largest_child<D / 2>(first_child_it, compare);
        It second_half_largest = largest_child<D - D / 2>(first_child_it + D / 2, compare);
        return compare(*first_half_largest, *second_half_largest) ? second_half_largest
                                                                  : first_half_largest;
    }
}
template <int D, typename It, typename Compare>
It largest_child(It first_child_it, int num_children, Compare &&compare)
{
    if constexpr (D == 2)
        return first_child_it;
    else if constexpr (D == 3)
    {
        if (num_children == 1)
            return first_child_it;
        else
            return first_child_it + !!compare(first_child_it[0], first_child_it[1]);
    }
    else if constexpr (D == 4)
    {
        switch (num_children)
        {
        case 1:
            return first_child_it;
        case 2:
            return first_child_it + !!compare(first_child_it[0], first_child_it[1]);
        default:
            It largest = first_child_it + !!compare(first_child_it[0], first_child_it[1]);
            return compare(*largest, first_child_it[2]) ? first_child_it + 2 : largest;
        }
    }
    else
    {
        switch (num_children)
        {
        case 1:
            return first_child_it;
        case 2:
            return first_child_it + !!compare(first_child_it[0], first_child_it[1]);
        case 3:
        {
            It largest = first_child_it + !!compare(first_child_it[0], first_child_it[1]);
            return compare(*largest, first_child_it[2]) ? first_child_it + 2 : largest;
        }
        case 4:
        {
            It largest_first_half =
                first_child_it + !!compare(first_child_it[0], first_child_it[1]);
            It largest_second_half =
                first_child_it + 2 + !!compare(first_child_it[2], first_child_it[3]);
            return compare(*largest_first_half, *largest_second_half) ? largest_second_half
                                                                      : largest_first_half;
        }
        default:
            int half = num_children / 2;
            It first_half_largest = largest_child<D>(first_child_it, half, compare);
            It second_half_largest =
                largest_child<D>(first_child_it + half, num_children - half, compare);
            return compare(*first_half_largest, *second_half_largest) ? second_half_largest
                                                                      : first_half_largest;
        }
    }
}
} // namespace dary_heap_helpers

template <int D, typename It, typename Compare>
void make_dary_heap(It begin, It end, Compare &&compare)
{
    using std::swap;
    uint64_t length = end - begin;
    if (length <= 1)
        return;
    uint64_t index = (length - 2) / D;
    // optimization: there can be only one item that has fewer than D children
    // handling that item up front simplifies the second loop a little, since
    // we know that all other items have two children
    int num_children_end = (length - 1) % D;
    if (num_children_end)
    {
        It largest_child = dary_heap_helpers::largest_child<D>(
            begin + dary_heap_helpers::first_child_index<D>(index), num_children_end, compare);
        if (compare(begin[index], *largest_child))
            swap(begin[index], *largest_child);
        if (index == 0)
            return;
        --index;
    }
    // optimization: half of all the items will have no grandchildren. this
    // simplifies the push_down function a lot, so we handle these items
    // first. we could then do another optimization where we know that
    // after the first half, the next quarter of items has grandchildren but
    // no great-grandchildren, but the code is already big enough
    if (index > 0)
    {
        uint64_t lowest_index_with_no_grandchildren =
            dary_heap_helpers::index_with_no_grandchild<D>(length);
        for (;;)
        {
            It largest_child = dary_heap_helpers::largest_child<D>(
                begin + dary_heap_helpers::first_child_index<D>(index), compare);
            if (compare(begin[index], *largest_child))
                swap(begin[index], *largest_child);
            if (index-- == lowest_index_with_no_grandchildren)
                break;
        }
    }
    for (;; --index)
    {
        typename std::iterator_traits<It>::value_type value = std::move(begin[index]);
        uint64_t move_down_index = index;
        for (;;)
        {
            uint64_t last_child_index = dary_heap_helpers::last_child_index<D>(move_down_index);
            uint64_t first_child_index = last_child_index - (D - 1);
            It largest_child = begin;
            if (last_child_index < length)
                largest_child =
                    dary_heap_helpers::largest_child<D>(begin + first_child_index, compare);
            else if (first_child_index >= length)
                break;
            else
                largest_child = dary_heap_helpers::largest_child<D>(
                    begin + first_child_index, length - first_child_index, compare);
            if (!compare(value, *largest_child))
                break;
            begin[move_down_index] = std::move(*largest_child);
            move_down_index = largest_child - begin;
        }
        begin[move_down_index] = std::move(value);
        if (index == 0)
            break;
    }
}
template <int D, typename It> void make_dary_heap(It begin, It end)
{
    make_dary_heap<D>(begin, end, std::less<>{});
}

template <int D, typename It, typename Compare>
bool is_dary_heap(It begin, It end, Compare &&compare)
{
    uint64_t length = end - begin;
    for (uint64_t i = 1; i < length; ++i)
    {
        uint64_t parent = dary_heap_helpers::parent_index<D>(i);
        if (compare(begin[parent], begin[i]))
            return false;
    }
    return true;
}
template <int D, typename It> bool is_dary_heap(It begin, It end)
{
    return is_dary_heap<D>(begin, end, std::less<>{});
}

template <int D, typename It, typename Compare>
void push_dary_heap(It begin, It end, Compare &&compare)
{
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    uint64_t index = (end - begin) - 1;
    while (index > 0)
    {
        uint64_t parent = dary_heap_helpers::parent_index<D>(index);
        if (!compare(begin[parent], value))
            break;
        begin[index] = std::move(begin[parent]);
        index = parent;
    }
    begin[index] = std::move(value);
}

template <int D, typename It> void push_dary_heap(It begin, It end)
{
    return push_dary_heap<D>(begin, end, std::less<>{});
}

template <int D, typename It, typename Compare>
void pop_dary_heap(It begin, It end, Compare &&compare)
{
    uint64_t length = (end - begin) - 1;
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    end[-1] = std::move(begin[0]);
    uint64_t index = 0;
    for (;;)
    {
        uint64_t last_child = dary_heap_helpers::last_child_index<D>(index);
        uint64_t first_child = last_child - (D - 1);
        if (last_child < length)
        {
            It largest_child = dary_heap_helpers::largest_child<D>(begin + first_child, compare);
            if (!compare(value, *largest_child))
                break;
            begin[index] = std::move(*largest_child);
            index = largest_child - begin;
        }
        else if (first_child < length)
        {
            It largest_child = dary_heap_helpers::largest_child<D>(
                begin + first_child, length - first_child, compare);
            if (compare(value, *largest_child))
            {
                begin[index] = std::move(*largest_child);
                index = largest_child - begin;
            }
            break;
        }
        else
            break;
    }
    begin[index] = std::move(value);
}
template <int D, typename It> void pop_dary_heap(It begin, It end)
{
    return pop_dary_heap<D>(begin, end, std::less<>{});
}
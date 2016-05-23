#ifndef PACKED_VECTOR_HPP
#define PACKED_VECTOR_HPP

#include "util/typedefs.hpp"

#include <cmath>
#include <vector>

namespace osrm
{
namespace util
{

const constexpr std::size_t BITSIZE = 33;
const constexpr std::size_t ELEMSIZE = 64;
const constexpr std::size_t PACKSIZE = BITSIZE * ELEMSIZE;

/**
 * Since OSM node IDs are (at the time of writing) not quite yet overflowing 32 bits, and
 * will predictably be containable within 33 bits for a long time, the following packs
 * 64-bit OSM IDs as 33-bit numbers within a 64-bit vector.
 */
class PackedVector
{
  public:
    PackedVector() = default;

    void push_back(OSMNodeID incoming_node_id)
    {
        std::uint64_t node_id = static_cast<std::uint64_t>(incoming_node_id);
        // mask incoming values, just in case they are > bitsize
        const std::uint64_t incoming_mask = static_cast<std::uint64_t>(pow(2, BITSIZE)) - 1;
        node_id = node_id & incoming_mask;

        const std::size_t available = (PACKSIZE - BITSIZE * num_elements) % ELEMSIZE;

        if (available == 0)
        {
            // insert ID at the left side of this element
            std::uint64_t at_left = node_id << (ELEMSIZE - BITSIZE);
            vec.push_back(at_left);
        }
        else if (available >= BITSIZE)
        {
            // insert ID somewhere in the middle of this element; ID can be contained
            // entirely within one element
            const std::uint64_t shifted = node_id << (available - BITSIZE);
            vec.back() = vec.back() | shifted;
        }
        else
        {
            // ID will be split between the end of this element and the beginning
            // of the next element
            const std::uint64_t left = node_id >> (BITSIZE - available);
            vec.back() = vec.back() | left;

            std::uint64_t right = node_id << (ELEMSIZE - (BITSIZE - available));
            vec.push_back(right);
        }

        num_elements++;
    }

    OSMNodeID at(const std::size_t &a_index) const
    {
        BOOST_ASSERT(a_index < num_elements);

        const std::size_t pack_group = trunc(a_index / ELEMSIZE);
        const std::size_t pack_index = (a_index + ELEMSIZE) % ELEMSIZE;
        const std::size_t left_index = (PACKSIZE - BITSIZE * pack_index) % ELEMSIZE;

        const bool back_half = pack_index >= BITSIZE;
        const std::size_t index = pack_group * BITSIZE + trunc(pack_index / BITSIZE) +
                                  trunc((pack_index - back_half) / 2);

        BOOST_ASSERT(index < vec.size());
        const std::uint64_t elem = vec.at(index);

        if (left_index == 0)
        {
            // ID is at the far left side of this element
            return static_cast<OSMNodeID>(elem >> (ELEMSIZE - BITSIZE));
        }
        else if (left_index >= BITSIZE)
        {
            // ID is entirely contained within this element
            const std::uint64_t at_right = elem >> (left_index - BITSIZE);
            const std::uint64_t left_mask = static_cast<std::uint64_t>(pow(2, BITSIZE)) - 1;
            return static_cast<OSMNodeID>(at_right & left_mask);
        }
        else
        {
            // ID is split between this and the next element
            const std::uint64_t left_mask = static_cast<std::uint64_t>(pow(2, left_index)) - 1;
            const std::uint64_t left_side = (elem & left_mask) << (BITSIZE - left_index);

            BOOST_ASSERT(index < vec.size() - 1);
            const std::uint64_t next_elem = vec.at(index + 1);

            const std::uint64_t right_side = next_elem >> (ELEMSIZE - (BITSIZE - left_index));
            return static_cast<OSMNodeID>(left_side | right_side);
        }
    }

    std::size_t size() const
    {
        return num_elements;
    }

    void reserve(std::size_t capacity)
    {
        vec.reserve(ceil(float(capacity) / ELEMSIZE) * BITSIZE);
    }

    std::size_t capacity() const
    {
        return floor(float(vec.capacity()) / BITSIZE) * ELEMSIZE;
    }

  private:
    std::vector<std::uint64_t> vec;
    std::size_t num_elements = 0;
};
}
}

#endif /* PACKED_VECTOR_HPP */

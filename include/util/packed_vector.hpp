#ifndef PACKED_VECTOR_HPP
#define PACKED_VECTOR_HPP

#include "util/typedefs.hpp"

#include <vector>

namespace osrm
{
namespace util
{

const constexpr std::size_t BITSIZE = 33;
const constexpr std::size_t ELEMSIZE = 64;
const constexpr std::size_t PACKSIZE = BITSIZE * ELEMSIZE;

class PackedVector
{
  public:
    PackedVector() = default;

    void insert(OSMNodeID &node_id)
    {
        // mask incoming values, just in case they are > bitsize
        std::uint64_t incoming_mask = static_cast<std::uint64_t>(pow(2, BITSIZE)) - 1;
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
            std::uint64_t shifted = node_id << (available - BITSIZE);
            vec.back() = vec.back() | shifted;
        }
        else
        {
            // ID will be split between the end of this element and the beginning
            // of the next element
            std::uint64_t left = node_id >> (BITSIZE - available);
            vec.back() = vec.back() | left;

            std::uint64_t right = node_id << (ELEMSIZE - (BITSIZE - available));
            vec.push_back(right);
        }

        num_elements++;
    }

    OSMNodeID retrieve(const std::size_t &a_index) const
    {
        // TODO check if OOB

        const std::size_t pack_group = trunc(a_index / ELEMSIZE);
        const std::size_t pack_index = (a_index + ELEMSIZE) % ELEMSIZE;       // ?
        const std::size_t left_index = (PACKSIZE - BITSIZE * pack_index) % ELEMSIZE;

        const bool back_half = pack_index >= BITSIZE;
        std::size_t index = pack_group * BITSIZE + trunc(pack_index / BITSIZE) + trunc((pack_index - back_half) / 2);

        std::uint64_t elem = vec.at(index);

        if (left_index == 0)
        {
            // ID is at the far left side of this element
            return elem >> (ELEMSIZE - BITSIZE);
        }
        else if (left_index >= BITSIZE)
        {
            // ID is entirely contained within this element
            std::uint64_t at_right = elem >> (left_index - BITSIZE);
            std::uint64_t left_mask = static_cast<std::uint64_t>(pow(2, BITSIZE)) - 1;
            return at_right & left_mask;
        }
        else
        {
            // ID is split between this and the next element
            std::uint64_t left_mask = static_cast<std::uint64_t>(pow(2, left_index)) - 1;
            std::uint64_t left_side = (elem & left_mask) << (BITSIZE - left_index);

            // TODO check OOB
            std::uint64_t next_elem = vec.at(index + 1);

            std::uint64_t right_side = next_elem >> (ELEMSIZE - (BITSIZE - left_index));
            return left_side | right_side;
        }
    }

  private:
    std::vector<OSMNodeID> vec;
    std::size_t num_elements = 0;
};

#endif /* PACKED_VECTOR_HPP */

#ifndef PACKED_VECTOR_HPP
#define PACKED_VECTOR_HPP

#include "util/typedefs.hpp"
#include "util/vector_view.hpp"

#include "storage/io_fwd.hpp"
#include "storage/shared_memory_ownership.hpp"

#include <cmath>
#include <vector>

namespace osrm
{
namespace util
{
namespace detail
{
template <typename T, storage::Ownership Ownership> class PackedVector;
}

namespace serialization
{
template <typename T, storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader, detail::PackedVector<T, Ownership> &vec);

template <typename T, storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer, const detail::PackedVector<T, Ownership> &vec);
}

namespace detail
{
/**
 * Since OSM node IDs are (at the time of writing) not quite yet overflowing 32 bits, and
 * will predictably be containable within 33 bits for a long time, the following packs
 * 64-bit OSM IDs as 33-bit numbers within a 64-bit vector.
 *
 * NOTE: this type is templated for future use, but will require a slight refactor to
 * configure BITSIZE and ELEMSIZE
 */
template <typename T, storage::Ownership Ownership> class PackedVector
{
    static const constexpr std::size_t BITSIZE = 33;
    static const constexpr std::size_t ELEMSIZE = 64;
    static const constexpr std::size_t PACKSIZE = BITSIZE * ELEMSIZE;

  public:
    using value_type = T;

    /**
     * Returns the size of the packed vector datastructure with `elements` packed elements (the size
     * of
     * its underlying uint64 vector)
     */
    inline static std::size_t elements_to_blocks(std::size_t elements)
    {
        return std::ceil(static_cast<double>(elements) * BITSIZE / ELEMSIZE);
    }

    void push_back(T incoming_node_id)
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

            add_last_elem(at_left);
        }
        else if (available >= BITSIZE)
        {
            // insert ID somewhere in the middle of this element; ID can be contained
            // entirely within one element
            const std::uint64_t shifted = node_id << (available - BITSIZE);

            replace_last_elem(vec_back() | shifted);
        }
        else
        {
            // ID will be split between the end of this element and the beginning
            // of the next element
            const std::uint64_t left = node_id >> (BITSIZE - available);

            std::uint64_t right = node_id << (ELEMSIZE - (BITSIZE - available));

            replace_last_elem(vec_back() | left);
            add_last_elem(right);
        }

        num_elements++;
    }

    T operator[](const std::size_t index) const { return at(index); }

    T at(const std::size_t a_index) const
    {
        BOOST_ASSERT(a_index < num_elements);

        const std::size_t pack_group = trunc(a_index / ELEMSIZE);
        const std::size_t pack_index = (a_index + ELEMSIZE) % ELEMSIZE;
        const std::size_t left_index = (PACKSIZE - BITSIZE * pack_index) % ELEMSIZE;

        const bool back_half = pack_index >= BITSIZE;
        const std::size_t index = pack_group * BITSIZE + trunc(pack_index / BITSIZE) +
                                  trunc((pack_index - back_half) / 2);

        BOOST_ASSERT(index < vec.size());
        const std::uint64_t elem = static_cast<std::uint64_t>(vec.at(index));

        if (left_index == 0)
        {
            // ID is at the far left side of this element
            return T{elem >> (ELEMSIZE - BITSIZE)};
        }
        else if (left_index >= BITSIZE)
        {
            // ID is entirely contained within this element
            const std::uint64_t at_right = elem >> (left_index - BITSIZE);
            const std::uint64_t left_mask = static_cast<std::uint64_t>(pow(2, BITSIZE)) - 1;
            return T{at_right & left_mask};
        }
        else
        {
            // ID is split between this and the next element
            const std::uint64_t left_mask = static_cast<std::uint64_t>(pow(2, left_index)) - 1;
            const std::uint64_t left_side = (elem & left_mask) << (BITSIZE - left_index);

            BOOST_ASSERT(index < vec.size() - 1);
            const std::uint64_t next_elem = static_cast<std::uint64_t>(vec.at(index + 1));

            const std::uint64_t right_side = next_elem >> (ELEMSIZE - (BITSIZE - left_index));
            return T{left_side | right_side};
        }
    }

    std::size_t size() const { return num_elements; }

    template <bool enabled = (Ownership == storage::Ownership::View)>
    void reserve(typename std::enable_if<!enabled, std::size_t>::type capacity)
    {
        vec.reserve(elements_to_blocks(capacity));
    }

    template <bool enabled = (Ownership == storage::Ownership::View)>
    void reset(typename std::enable_if<enabled, std::uint64_t>::type *ptr,
               typename std::enable_if<enabled, std::size_t>::type size)
    {
        vec.reset(ptr, size);
    }

    template <bool enabled = (Ownership == storage::Ownership::View)>
    void set_number_of_entries(typename std::enable_if<enabled, std::size_t>::type count)
    {
        num_elements = count;
    }

    std::size_t capacity() const
    {
        return std::floor(static_cast<double>(vec.capacity()) * ELEMSIZE / BITSIZE);
    }

    friend void serialization::read<T, Ownership>(storage::io::FileReader &reader,
                                                  detail::PackedVector<T, Ownership> &vec);

    friend void serialization::write<T, Ownership>(storage::io::FileWriter &writer,
                                                   const detail::PackedVector<T, Ownership> &vec);

  private:
    util::ViewOrVector<std::uint64_t, Ownership> vec;

    std::uint64_t num_elements = 0;

    signed cursor = -1;

    template <bool enabled = (Ownership == storage::Ownership::View)>
    void replace_last_elem(typename std::enable_if<enabled, std::uint64_t>::type last_elem)
    {
        vec[cursor] = last_elem;
    }

    template <bool enabled = (Ownership == storage::Ownership::View)>
    void replace_last_elem(typename std::enable_if<!enabled, std::uint64_t>::type last_elem)
    {
        vec.back() = last_elem;
    }

    template <bool enabled = (Ownership == storage::Ownership::View)>
    void add_last_elem(typename std::enable_if<enabled, std::uint64_t>::type last_elem)
    {
        vec[cursor + 1] = last_elem;
        cursor++;
    }

    template <bool enabled = (Ownership == storage::Ownership::View)>
    void add_last_elem(typename std::enable_if<!enabled, std::uint64_t>::type last_elem)
    {
        vec.push_back(last_elem);
    }

    template <bool enabled = (Ownership == storage::Ownership::View)>
    std::uint64_t vec_back(typename std::enable_if<enabled>::type * = nullptr)
    {
        return vec[cursor];
    }

    template <bool enabled = (Ownership == storage::Ownership::View)>
    std::uint64_t vec_back(typename std::enable_if<!enabled>::type * = nullptr)
    {
        return vec.back();
    }
};
}

template <typename T> using PackedVector = detail::PackedVector<T, storage::Ownership::Container>;
template <typename T> using PackedVectorView = detail::PackedVector<T, storage::Ownership::View>;
}
}

#endif /* PACKED_VECTOR_HPP */

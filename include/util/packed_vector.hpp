#ifndef PACKED_VECTOR_HPP
#define PACKED_VECTOR_HPP

#include "util/integer_range.hpp"
#include "util/typedefs.hpp"
#include "util/vector_view.hpp"

#include "storage/shared_memory_ownership.hpp"
#include "storage/tar_fwd.hpp"

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/reverse_iterator.hpp>

#include <tbb/atomic.h>

#include <array>
#include <cmath>
#include <vector>

namespace osrm
{
namespace util
{
namespace detail
{
template <typename T, std::size_t Bits, storage::Ownership Ownership> class PackedVector;
}

namespace serialization
{
template <typename T, std::size_t Bits, storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::PackedVector<T, Bits, Ownership> &vec);

template <typename T, std::size_t Bits, storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::PackedVector<T, Bits, Ownership> &vec);
} // namespace serialization

namespace detail
{

template <typename WordT, typename T>
inline T get_lower_half_value(WordT word,
                              WordT mask,
                              std::uint8_t offset,
                              typename std::enable_if_t<std::is_integral<T>::value> * = 0)
{
    return static_cast<T>((word & mask) >> offset);
}

template <typename WordT, typename T>
inline T
get_lower_half_value(WordT word, WordT mask, std::uint8_t offset, typename T::value_type * = 0)
{
    return T{static_cast<typename T::value_type>((word & mask) >> offset)};
}

template <typename WordT, typename T>
inline T get_upper_half_value(WordT word,
                              WordT mask,
                              std::uint8_t offset,
                              typename std::enable_if_t<std::is_integral<T>::value> * = 0)
{
    return static_cast<T>((word & mask) << offset);
}

template <typename WordT, typename T>
inline T
get_upper_half_value(WordT word, WordT mask, std::uint8_t offset, typename T::value_type * = 0)
{
    static_assert(std::is_unsigned<WordT>::value, "Only unsigned word types supported for now.");
    return T{static_cast<typename T::value_type>((word & mask) << offset)};
}

template <typename WordT, typename T>
inline WordT set_lower_value(WordT word, WordT mask, std::uint8_t offset, T value)
{
    static_assert(std::is_unsigned<WordT>::value, "Only unsigned word types supported for now.");
    return (word & ~mask) | ((static_cast<WordT>(value) << offset) & mask);
}

template <typename WordT, typename T>
inline WordT set_upper_value(WordT word, WordT mask, std::uint8_t offset, T value)
{
    static_assert(std::is_unsigned<WordT>::value, "Only unsigned word types supported for now.");
    return (word & ~mask) | ((static_cast<WordT>(value) >> offset) & mask);
}

template <typename T, std::size_t Bits, storage::Ownership Ownership> class PackedVector
{
    using WordT = std::uint64_t;

    // This fails for all strong typedef types
    // static_assert(std::is_integral<T>::value, "T must be an integral type.");
    static_assert(sizeof(T) <= sizeof(WordT), "Maximum size of type T is 8 bytes");
    static_assert(Bits > 0, "Minimum number of bits is 0.");
    static_assert(Bits <= sizeof(WordT) * CHAR_BIT, "Maximum number of bits is 64.");

    static constexpr std::size_t WORD_BITS = sizeof(WordT) * CHAR_BIT;
    // number of elements per block, use the number of bits so we make sure
    // we can devide the total number of bits by the element bis
  public:
    static constexpr std::size_t BLOCK_ELEMENTS = WORD_BITS;

  private:
    // number of words per block
    static constexpr std::size_t BLOCK_WORDS = (Bits * BLOCK_ELEMENTS) / WORD_BITS;

    // C++14 does not allow operator[] to be constexpr, this is fixed in C++17.
    static /* constexpr */ std::array<WordT, BLOCK_ELEMENTS> initialize_lower_mask()
    {
        std::array<WordT, BLOCK_ELEMENTS> lower_mask;

        const WordT mask = (1ULL << Bits) - 1;
        auto offset = 0;
        for (auto element_index = 0u; element_index < BLOCK_ELEMENTS; element_index++)
        {
            auto local_offset = offset % WORD_BITS;
            lower_mask[element_index] = mask << local_offset;
            offset += Bits;
        }

        return lower_mask;
    }

    static /* constexpr */ std::array<WordT, BLOCK_ELEMENTS> initialize_upper_mask()
    {
        std::array<WordT, BLOCK_ELEMENTS> upper_mask;

        const WordT mask = (1ULL << Bits) - 1;
        auto offset = 0;
        for (auto element_index = 0u; element_index < BLOCK_ELEMENTS; element_index++)
        {
            auto local_offset = offset % WORD_BITS;
            // check we sliced off bits
            if (local_offset + Bits > WORD_BITS)
            {
                upper_mask[element_index] = mask >> (WORD_BITS - local_offset);
            }
            else
            {
                upper_mask[element_index] = 0;
            }
            offset += Bits;
        }

        return upper_mask;
    }

    static /* constexpr */ std::array<std::uint8_t, BLOCK_ELEMENTS> initialize_lower_offset()
    {
        std::array<std::uint8_t, WORD_BITS> lower_offset;

        auto offset = 0;
        for (auto element_index = 0u; element_index < BLOCK_ELEMENTS; element_index++)
        {
            auto local_offset = offset % WORD_BITS;
            lower_offset[element_index] = local_offset;
            offset += Bits;
        }

        return lower_offset;
    }

    static /* constexpr */ std::array<std::uint8_t, BLOCK_ELEMENTS> initialize_upper_offset()
    {
        std::array<std::uint8_t, BLOCK_ELEMENTS> upper_offset;

        auto offset = 0;
        for (auto element_index = 0u; element_index < BLOCK_ELEMENTS; element_index++)
        {
            auto local_offset = offset % WORD_BITS;
            // check we sliced off bits
            if (local_offset + Bits > WORD_BITS)
            {
                upper_offset[element_index] = WORD_BITS - local_offset;
            }
            else
            {
                upper_offset[element_index] = Bits;
            }
            offset += Bits;
        }

        return upper_offset;
    }

    static /* constexpr */ std::array<std::uint8_t, BLOCK_ELEMENTS> initialize_word_offset()
    {
        std::array<std::uint8_t, BLOCK_ELEMENTS> word_offset;

        auto offset = 0;
        for (auto element_index = 0u; element_index < BLOCK_ELEMENTS; element_index++)
        {
            word_offset[element_index] = offset / WORD_BITS;
            offset += Bits;
        }

        return word_offset;
    }

    // For now we need to call these on object creation
    void initialize()
    {
        lower_mask = initialize_lower_mask();
        upper_mask = initialize_upper_mask();
        lower_offset = initialize_lower_offset();
        upper_offset = initialize_upper_offset();
        word_offset = initialize_word_offset();
    }

    // mask for the lower/upper word of a record
    // TODO: With C++17 these could be constexpr
    /* static constexpr */ std::array<WordT, BLOCK_ELEMENTS>
        lower_mask /* = initialize_lower_mask()*/;
    /* static constexpr */ std::array<WordT, BLOCK_ELEMENTS>
        upper_mask /* = initialize_upper_mask()*/;
    /* static constexpr */ std::array<std::uint8_t, BLOCK_ELEMENTS>
        lower_offset /* = initialize_lower_offset()*/;
    /* static constexpr */ std::array<std::uint8_t, BLOCK_ELEMENTS>
        upper_offset /* = initialize_upper_offset()*/;
    // in which word of the block is the element
    /* static constexpr */ std::array<std::uint8_t, BLOCK_ELEMENTS> word_offset =
        initialize_word_offset();

    struct InternalIndex
    {
        // index to the word that contains the lower
        // part of the value
        // note: upper_word == lower_word + 1
        std::size_t lower_word;
        // index to the element of the block
        std::uint8_t element;

        bool operator==(const InternalIndex &other) const
        {
            return std::tie(lower_word, element) == std::tie(other.lower_word, other.element);
        }
    };

  public:
    using value_type = T;
    static constexpr std::size_t value_size = Bits;
    using block_type = WordT;

    class internal_reference
    {
      public:
        internal_reference(PackedVector &container, const InternalIndex internal_index)
            : container(container), internal_index(internal_index)
        {
        }

        internal_reference &operator=(const value_type value)
        {
            container.set_value(internal_index, value);
            return *this;
        }

        operator T() const { return container.get_value(internal_index); }

        bool operator==(const internal_reference &other) const
        {
            return &container == &other.container && internal_index == other.internal_index;
        }

        friend std::ostream &operator<<(std::ostream &os, const internal_reference &rhs)
        {
            return os << static_cast<T>(rhs);
        }

      private:
        PackedVector &container;
        const InternalIndex internal_index;
    };

    template <typename DataT, typename ContainerT, typename ReferenceT = internal_reference>
    class iterator_impl
        : public boost::iterator_facade<iterator_impl<DataT, ContainerT, ReferenceT>,
                                        DataT,
                                        boost::random_access_traversal_tag,
                                        ReferenceT>
    {
        typedef boost::iterator_facade<iterator_impl<DataT, ContainerT, ReferenceT>,
                                       DataT,
                                       boost::random_access_traversal_tag,
                                       ReferenceT>
            base_t;

      public:
        typedef typename base_t::value_type value_type;
        typedef typename base_t::difference_type difference_type;
        typedef typename base_t::reference reference;
        typedef std::random_access_iterator_tag iterator_category;

        explicit iterator_impl()
            : container(nullptr), index(std::numeric_limits<std::size_t>::max())
        {
        }
        explicit iterator_impl(ContainerT *container, const std::size_t index)
            : container(container), index(index)
        {
        }

      private:
        void increment() { ++index; }
        void decrement() { --index; }
        void advance(difference_type offset) { index += offset; }
        bool equal(const iterator_impl &other) const { return index == other.index; }
        auto dereference() const { return (*container)[index]; }
        difference_type distance_to(const iterator_impl &other) const
        {
            return other.index - index;
        }

      private:
        ContainerT *container;
        std::size_t index;

        friend class ::boost::iterator_core_access;
    };

    using iterator = iterator_impl<T, PackedVector>;
    using const_iterator = iterator_impl<const T, const PackedVector, T>;
    using reverse_iterator = boost::reverse_iterator<iterator>;

    PackedVector(std::initializer_list<T> list)
    {
        initialize();
        reserve(list.size());
        for (const auto value : list)
            push_back(value);
    }

    PackedVector() { initialize(); };
    PackedVector(const PackedVector &) = default;
    PackedVector(PackedVector &&) = default;
    PackedVector &operator=(const PackedVector &) = default;
    PackedVector &operator=(PackedVector &&) = default;

    PackedVector(std::size_t size)
    {
        initialize();
        resize(size);
    }

    PackedVector(std::size_t size, T initial_value)
    {
        initialize();
        resize(size);
        fill(initial_value);
    }

    PackedVector(util::ViewOrVector<WordT, Ownership> vec_, std::size_t num_elements)
        : vec(std::move(vec_)), num_elements(num_elements)
    {
        initialize();
    }

    // forces the efficient read-only lookup
    auto peek(const std::size_t index) const { return operator[](index); }

    auto operator[](const std::size_t index) const { return get_value(get_internal_index(index)); }

    auto operator[](const std::size_t index)
    {
        return internal_reference{*this, get_internal_index(index)};
    }

    auto at(std::size_t index) const
    {
        if (index < num_elements)
            return operator[](index);
        else
            throw std::out_of_range(std::to_string(index) + " is bigger then container size " +
                                    std::to_string(num_elements));
    }

    auto at(std::size_t index)
    {
        if (index < num_elements)
            return operator[](index);
        else
            throw std::out_of_range(std::to_string(index) + " is bigger then container size " +
                                    std::to_string(num_elements));
    }

    auto begin() { return iterator(this, 0); }

    auto end() { return iterator(this, num_elements); }

    auto begin() const { return const_iterator(this, 0); }

    auto end() const { return const_iterator(this, num_elements); }

    auto cbegin() const { return const_iterator(this, 0); }

    auto cend() const { return const_iterator(this, num_elements); }

    auto rbegin() { return reverse_iterator(end()); }

    auto rend() { return reverse_iterator(begin()); }

    auto front() const { return operator[](0); }
    auto back() const { return operator[](num_elements - 1); }
    auto front() { return operator[](0); }
    auto back() { return operator[](num_elements - 1); }

    // Since we only allow passing by value anyway this is just an alias
    template <class... Args> void emplace_back(Args... args)
    {
        push_back(T{std::forward<Args>(args)...});
    }

    void push_back(const T value)
    {
        BOOST_ASSERT_MSG(value <= T{(1ULL << Bits) - 1}, "Value too big for packed storage.");

        auto internal_index = get_internal_index(num_elements);

        while (internal_index.lower_word + 1 >= vec.size())
        {
            allocate_blocks(1);
        }

        set_value(internal_index, value);
        num_elements++;

        BOOST_ASSERT(static_cast<T>(back()) == value);
    }

    std::size_t size() const { return num_elements; }

    void resize(std::size_t elements)
    {
        num_elements = elements;
        auto num_blocks = (elements + BLOCK_ELEMENTS - 1) / BLOCK_ELEMENTS;
        vec.resize(num_blocks * BLOCK_WORDS + 1);
    }

    std::size_t capacity() const { return (vec.capacity() / BLOCK_WORDS) * BLOCK_ELEMENTS; }

    template <bool enabled = (Ownership == storage::Ownership::View)>
    void reserve(typename std::enable_if<!enabled, std::size_t>::type capacity)
    {
        auto num_blocks = (capacity + BLOCK_ELEMENTS - 1) / BLOCK_ELEMENTS;
        vec.reserve(num_blocks * BLOCK_WORDS + 1);
    }

    friend void serialization::read<T, Bits, Ownership>(storage::tar::FileReader &reader,
                                                        const std::string &name,
                                                        PackedVector &vec);

    friend void serialization::write<T, Bits, Ownership>(storage::tar::FileWriter &writer,
                                                         const std::string &name,
                                                         const PackedVector &vec);

    inline void swap(PackedVector &other) noexcept
    {
        std::swap(vec, other.vec);
        std::swap(num_elements, other.num_elements);
    }

  private:
    void allocate_blocks(std::size_t num_blocks)
    {
        vec.resize(vec.size() + num_blocks * BLOCK_WORDS);
    }

    inline InternalIndex get_internal_index(const std::size_t index) const
    {
        const auto block_offset = BLOCK_WORDS * (index / BLOCK_ELEMENTS);
        const std::uint8_t element_index = index % BLOCK_ELEMENTS;
        const auto lower_word_index = block_offset + word_offset[element_index];

        return InternalIndex{lower_word_index, element_index};
    }

    inline void fill(const T value)
    {
        for (auto block_index : util::irange<std::size_t>(0, vec.size() / BLOCK_WORDS))
        {
            const auto block_offset = block_index * BLOCK_WORDS;

            for (auto element_index : util::irange<std::uint8_t>(0, BLOCK_ELEMENTS))
            {
                const auto lower_word_index = block_offset + word_offset[element_index];
                set_value({lower_word_index, element_index}, value);
            }
        }
    }

    inline T get_value(const InternalIndex internal_index) const
    {
        const auto lower_word = vec[internal_index.lower_word];
        // note this can actually already be a word of the next block however in
        // that case the upper mask will be 0.
        // we make sure to have a sentinel element to avoid out-of-bounds errors.
        const auto upper_word = vec[internal_index.lower_word + 1];
        const auto value = get_lower_half_value<WordT, T>(lower_word,
                                                          lower_mask[internal_index.element],
                                                          lower_offset[internal_index.element]) |
                           get_upper_half_value<WordT, T>(upper_word,
                                                          upper_mask[internal_index.element],
                                                          upper_offset[internal_index.element]);
        return value;
    }

    inline void set_value(const InternalIndex internal_index, const T value)
    {
        // ⚠ The method uses CAS spinlocks to prevent data races in parallel calls
        // TBB internal atomic's are used for CAS on non-atomic data
        // Parallel read and write access is not allowed

        auto &lower_word = vec[internal_index.lower_word];
        auto &upper_word = vec[internal_index.lower_word + 1];

        // Lock-free update of the lower word
        WordT local_lower_word, new_lower_word;
        do
        {
            local_lower_word = lower_word;
            new_lower_word = set_lower_value<WordT, T>(local_lower_word,
                                                       lower_mask[internal_index.element],
                                                       lower_offset[internal_index.element],
                                                       value);
        } while (tbb::internal::as_atomic(lower_word)
                     .compare_and_swap(new_lower_word, local_lower_word) != local_lower_word);

        // Lock-free update of the upper word
        WordT local_upper_word, new_upper_word;
        do
        {
            local_upper_word = upper_word;
            new_upper_word = set_upper_value<WordT, T>(local_upper_word,
                                                       upper_mask[internal_index.element],
                                                       upper_offset[internal_index.element],
                                                       value);
        } while (tbb::internal::as_atomic(upper_word)
                     .compare_and_swap(new_upper_word, local_upper_word) != local_upper_word);
    }

    util::ViewOrVector<WordT, Ownership> vec;
    std::uint64_t num_elements = 0;
};
} // namespace detail

template <typename T, std::size_t Bits>
using PackedVector = detail::PackedVector<T, Bits, storage::Ownership::Container>;
template <typename T, std::size_t Bits>
using PackedVectorView = detail::PackedVector<T, Bits, storage::Ownership::View>;
} // namespace util
} // namespace osrm

#endif /* PACKED_VECTOR_HPP */

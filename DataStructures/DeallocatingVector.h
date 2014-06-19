/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef DEALLOCATINGVECTOR_H_
#define DEALLOCATINGVECTOR_H_

#include <boost/assert.hpp>
#include <cstring>
#include <vector>

template <typename ElementT,
          std::size_t bucketSizeC = 8388608 / sizeof(ElementT),
          bool DeallocateC = false>
class DeallocatingVectorIterator : public std::iterator<std::random_access_iterator_tag, ElementT>
{
  protected:
    class DeallocatingVectorIteratorState
    {
      private:
        // make constructors explicit, so we do not mix random access and deallocation iterators.
        DeallocatingVectorIteratorState();

      public:
        explicit DeallocatingVectorIteratorState(const DeallocatingVectorIteratorState &r)
            : index(r.index), bucket_list(r.bucket_list)
        {
        }
        explicit DeallocatingVectorIteratorState(const std::size_t idx,
                                                 std::vector<ElementT *> &input_list)
            : index(idx), bucket_list(input_list)
        {
        }
        std::size_t index;
        std::vector<ElementT *> &bucket_list;

        inline bool operator!=(const DeallocatingVectorIteratorState &other)
        {
            return index != other.index;
        }

        inline bool operator==(const DeallocatingVectorIteratorState &other)
        {
            return index == other.index;
        }

        bool operator<(const DeallocatingVectorIteratorState &other) const
        {
            return index < other.index;
        }

        bool operator>(const DeallocatingVectorIteratorState &other) const
        {
            return index > other.index;
        }

        bool operator>=(const DeallocatingVectorIteratorState &other) const
        {
            return index >= other.index;
        }

        // This is a hack to make assignment operator possible with reference member
        inline DeallocatingVectorIteratorState &operator=(const DeallocatingVectorIteratorState &a)
        {
            if (this != &a)
            {
                this->DeallocatingVectorIteratorState::
                    ~DeallocatingVectorIteratorState();        // explicit non-virtual destructor
                new (this) DeallocatingVectorIteratorState(a); // placement new
            }
            return *this;
        }
    };

    DeallocatingVectorIteratorState current_state;

  public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef typename std::iterator<std::random_access_iterator_tag, ElementT>::value_type
    value_type;
    typedef typename std::iterator<std::random_access_iterator_tag, ElementT>::difference_type
    difference_type;
    typedef typename std::iterator<std::random_access_iterator_tag, ElementT>::reference reference;
    typedef typename std::iterator<std::random_access_iterator_tag, ElementT>::pointer pointer;

    DeallocatingVectorIterator() {}

    template <typename T2>
    explicit DeallocatingVectorIterator(const DeallocatingVectorIterator<T2> &r)
        : current_state(r.current_state)
    {
    }

    DeallocatingVectorIterator(std::size_t idx, std::vector<ElementT *> &input_list)
        : current_state(idx, input_list)
    {
    }
    explicit DeallocatingVectorIterator(const DeallocatingVectorIteratorState &r) : current_state(r) {}

    template <typename T2>
    DeallocatingVectorIterator &operator=(const DeallocatingVectorIterator<T2> &r)
    {
        if (DeallocateC)
        {
            BOOST_ASSERT(false);
        }
        current_state = r.current_state;
        return *this;
    }

    inline DeallocatingVectorIterator &operator++()
    { // prefix
        ++current_state.index;
        return *this;
    }

    inline DeallocatingVectorIterator &operator--()
    { // prefix
        if (DeallocateC)
        {
            BOOST_ASSERT(false);
        }
        --current_state.index;
        return *this;
    }

    inline DeallocatingVectorIterator operator++(int)
    { // postfix
        DeallocatingVectorIteratorState my_state(current_state);
        current_state.index++;
        return DeallocatingVectorIterator(my_state);
    }
    inline DeallocatingVectorIterator operator--(int)
    { // postfix
        if (DeallocateC)
        {
            BOOST_ASSERT(false);
        }
        DeallocatingVectorIteratorState my_state(current_state);
        current_state.index--;
        return DeallocatingVectorIterator(my_state);
    }

    inline DeallocatingVectorIterator operator+(const difference_type &n) const
    {
        DeallocatingVectorIteratorState my_state(current_state);
        my_state.index += n;
        return DeallocatingVectorIterator(my_state);
    }

    inline DeallocatingVectorIterator &operator+=(const difference_type &n)
    {
        current_state.index += n;
        return *this;
    }

    inline DeallocatingVectorIterator operator-(const difference_type &n) const
    {
        if (DeallocateC)
        {
            BOOST_ASSERT(false);
        }
        DeallocatingVectorIteratorState my_state(current_state);
        my_state.index -= n;
        return DeallocatingVectorIterator(my_state);
    }

    inline DeallocatingVectorIterator &operator-=(const difference_type &n) const
    {
        if (DeallocateC)
        {
            BOOST_ASSERT(false);
        }
        current_state.index -= n;
        return *this;
    }

    inline reference operator*() const
    {
        std::size_t current_bucket = current_state.index / bucketSizeC;
        std::size_t current_index = current_state.index % bucketSizeC;
        return (current_state.bucket_list[current_bucket][current_index]);
    }

    inline pointer operator->() const
    {
        std::size_t current_bucket = current_state.index / bucketSizeC;
        std::size_t current_index = current_state.index % bucketSizeC;
        return &(current_state.bucket_list[current_bucket][current_index]);
    }

    inline bool operator!=(const DeallocatingVectorIterator &other)
    {
        return current_state != other.current_state;
    }

    inline bool operator==(const DeallocatingVectorIterator &other)
    {
        return current_state == other.current_state;
    }

    inline bool operator<(const DeallocatingVectorIterator &other) const
    {
        return current_state < other.current_state;
    }

    inline bool operator>(const DeallocatingVectorIterator &other) const
    {
        return current_state > other.current_state;
    }

    inline bool operator>=(const DeallocatingVectorIterator &other) const
    {
        return current_state >= other.current_state;
    }

    difference_type operator-(const DeallocatingVectorIterator &other)
    {
        if (DeallocateC)
        {
            BOOST_ASSERT(false);
        }
        return current_state.index - other.current_state.index;
    }
};

template <typename ElementT, std::size_t bucketSizeC = 8388608 / sizeof(ElementT)>
class DeallocatingVector
{
  private:
    std::size_t current_size;
    std::vector<ElementT *> bucket_list;

  public:
    typedef ElementT value_type;
    typedef DeallocatingVectorIterator<ElementT, bucketSizeC, false> iterator;
    typedef DeallocatingVectorIterator<ElementT, bucketSizeC, false> const_iterator;

    // this iterator deallocates all buckets that have been visited. Iterators to visited objects
    // become invalid.
    typedef DeallocatingVectorIterator<ElementT, bucketSizeC, true> deallocation_iterator;

    DeallocatingVector() : current_size(0)
    {
        // initial bucket
        bucket_list.emplace_back(new ElementT[bucketSizeC]);
    }

    ~DeallocatingVector() { clear(); }

    inline void swap(DeallocatingVector<ElementT, bucketSizeC> &other)
    {
        std::swap(current_size, other.current_size);
        bucket_list.swap(other.bucket_list);
    }

    inline void clear()
    {
        // Delete[]'ing ptr's to all Buckets
        for (unsigned i = 0; i < bucket_list.size(); ++i)
        {
            if (nullptr != bucket_list[i])
            {
                delete[] bucket_list[i];
                bucket_list[i] = nullptr;
            }
        }
        // Removing all ptrs from vector
        std::vector<ElementT *>().swap(bucket_list);
        current_size = 0;
    }

    inline void push_back(const ElementT &element)
    {
        const std::size_t current_capacity = capacity();
        if (current_size == current_capacity)
        {
            bucket_list.push_back(new ElementT[bucketSizeC]);
        }

        std::size_t current_index = size() % bucketSizeC;
        bucket_list.back()[current_index] = element;
        ++current_size;
    }

    inline void emplace_back(const ElementT &&element)
    {
        const std::size_t current_capacity = capacity();
        if (current_size == current_capacity)
        {
            bucket_list.push_back(new ElementT[bucketSizeC]);
        }

        const std::size_t current_index = size() % bucketSizeC;
        bucket_list.back()[current_index] = element;
        ++current_size;
    }

    inline void reserve(const std::size_t) const
    {
        // don't do anything
    }

    inline void resize(const std::size_t new_size)
    {
        if (new_size > current_size)
        {
            while (capacity() < new_size)
            {
                bucket_list.push_back(new ElementT[bucketSizeC]);
            }
            current_size = new_size;
        }
        if (new_size < current_size)
        {
            const std::size_t number_of_necessary_buckets = 1 + (new_size / bucketSizeC);

            for (std::size_t i = number_of_necessary_buckets; i < bucket_list.size(); ++i)
            {
                delete[] bucket_list[i];
            }
            bucket_list.resize(number_of_necessary_buckets);
            current_size = new_size;
        }
    }

    inline std::size_t size() const { return current_size; }

    inline std::size_t capacity() const { return bucket_list.size() * bucketSizeC; }

    inline iterator begin() { return iterator(static_cast<std::size_t>(0), bucket_list); }

    inline iterator end() { return iterator(size(), bucket_list); }

    inline deallocation_iterator dbegin()
    {
        return deallocation_iterator(static_cast<std::size_t>(0), bucket_list);
    }

    inline deallocation_iterator dend() { return deallocation_iterator(size(), bucket_list); }

    inline const_iterator begin() const
    {
        return const_iterator(static_cast<std::size_t>(0), bucket_list);
    }

    inline const_iterator end() const { return const_iterator(size(), bucket_list); }

    inline ElementT &operator[](const std::size_t index)
    {
        std::size_t _bucket = index / bucketSizeC;
        std::size_t _index = index % bucketSizeC;
        return (bucket_list[_bucket][_index]);
    }

    const inline ElementT &operator[](const std::size_t index) const
    {
        std::size_t _bucket = index / bucketSizeC;
        std::size_t _index = index % bucketSizeC;
        return (bucket_list[_bucket][_index]);
    }

    inline ElementT &back()
    {
        std::size_t _bucket = current_size / bucketSizeC;
        std::size_t _index = current_size % bucketSizeC;
        return (bucket_list[_bucket][_index]);
    }

    const inline ElementT &back() const
    {
        std::size_t _bucket = current_size / bucketSizeC;
        std::size_t _index = current_size % bucketSizeC;
        return (bucket_list[_bucket][_index]);
    }
};

#endif /* DEALLOCATINGVECTOR_H_ */

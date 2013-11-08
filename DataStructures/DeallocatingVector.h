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

#if __cplusplus > 199711L
#define DEALLOCATION_VECTOR_NULL_PTR nullptr
#else
#define DEALLOCATION_VECTOR_NULL_PTR NULL
#endif


template<typename ElementT, std::size_t bucketSizeC = 8388608/sizeof(ElementT), bool DeallocateC = false>
class DeallocatingVectorIterator : public std::iterator<std::random_access_iterator_tag, ElementT> {
protected:

    class DeallocatingVectorIteratorState {
    private:
        //make constructors explicit, so we do not mix random access and deallocation iterators.
        DeallocatingVectorIteratorState();
    public:
        explicit DeallocatingVectorIteratorState(const DeallocatingVectorIteratorState &r) : /*mData(r.mData),*/ mIndex(r.mIndex), mBucketList(r.mBucketList) {}
        explicit DeallocatingVectorIteratorState(const std::size_t idx, std::vector<ElementT *> & input_list) : /*mData(DEALLOCATION_VECTOR_NULL_PTR),*/ mIndex(idx), mBucketList(input_list) {
        }
        std::size_t mIndex;
        std::vector<ElementT *> & mBucketList;

        inline bool operator!=(const DeallocatingVectorIteratorState &other) {
            return mIndex != other.mIndex;
        }

        inline bool operator==(const DeallocatingVectorIteratorState &other) {
            return mIndex == other.mIndex;
        }

        bool operator<(const DeallocatingVectorIteratorState &other) const {
            return mIndex < other.mIndex;
        }

        bool operator>(const DeallocatingVectorIteratorState &other) const {
            return mIndex > other.mIndex;
        }

        bool operator>=(const DeallocatingVectorIteratorState &other) const {
            return mIndex >= other.mIndex;
        }

        //This is a hack to make assignment operator possible with reference member
        inline DeallocatingVectorIteratorState& operator= (const DeallocatingVectorIteratorState &a) {
            if (this != &a) {
                this->DeallocatingVectorIteratorState::~DeallocatingVectorIteratorState(); // explicit non-virtual destructor
                new (this) DeallocatingVectorIteratorState(a); // placement new
            }
            return *this;
        }
    };

    DeallocatingVectorIteratorState mState;

public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef typename std::iterator<std::random_access_iterator_tag, ElementT>::value_type value_type;
    typedef typename std::iterator<std::random_access_iterator_tag, ElementT>::difference_type difference_type;
    typedef typename std::iterator<std::random_access_iterator_tag, ElementT>::reference reference;
    typedef typename std::iterator<std::random_access_iterator_tag, ElementT>::pointer pointer;

    DeallocatingVectorIterator() {}

    template<typename T2>
    DeallocatingVectorIterator(const DeallocatingVectorIterator<T2> & r) : mState(r.mState) {}

    DeallocatingVectorIterator(std::size_t idx, std::vector<ElementT *> & input_list) : mState(idx, input_list) {}
    DeallocatingVectorIterator(const DeallocatingVectorIteratorState & r) : mState(r) {}

    template<typename T2>
    DeallocatingVectorIterator& operator=(const DeallocatingVectorIterator<T2> &r) {
        if(DeallocateC) BOOST_ASSERT(false);
        mState = r.mState; return *this;
    }

    inline DeallocatingVectorIterator& operator++() { //prefix
        ++mState.mIndex;
        return *this;
    }

    inline DeallocatingVectorIterator& operator--() { //prefix
        if(DeallocateC) BOOST_ASSERT(false);
        --mState.mIndex;
        return *this;
    }

    inline DeallocatingVectorIterator operator++(int) { //postfix
        DeallocatingVectorIteratorState _myState(mState);
        mState.mIndex++;
        return DeallocatingVectorIterator(_myState);
    }
    inline DeallocatingVectorIterator operator--(int) { //postfix
        if(DeallocateC) BOOST_ASSERT(false);
        DeallocatingVectorIteratorState _myState(mState);
        mState.mIndex--;
        return DeallocatingVectorIterator(_myState);
    }

    inline DeallocatingVectorIterator operator+(const difference_type& n) const {
        DeallocatingVectorIteratorState _myState(mState);
        _myState.mIndex+=n;
        return DeallocatingVectorIterator(_myState);
    }

    inline DeallocatingVectorIterator& operator+=(const difference_type& n) {
        mState.mIndex+=n; return *this;
    }

    inline DeallocatingVectorIterator operator-(const difference_type& n) const {
        if(DeallocateC) BOOST_ASSERT(false);
        DeallocatingVectorIteratorState _myState(mState);
        _myState.mIndex-=n;
        return DeallocatingVectorIterator(_myState);
    }

    inline DeallocatingVectorIterator& operator-=(const difference_type &n) const {
        if(DeallocateC) BOOST_ASSERT(false);
        mState.mIndex-=n; return *this;
    }

    inline reference operator*() const {
        std::size_t _bucket = mState.mIndex/bucketSizeC;
        std::size_t _index = mState.mIndex%bucketSizeC;
        return (mState.mBucketList[_bucket][_index]);
    }

    inline pointer operator->() const {
        std::size_t _bucket = mState.mIndex/bucketSizeC;
        std::size_t _index = mState.mIndex%bucketSizeC;
        return &(mState.mBucketList[_bucket][_index]);
    }

    inline bool operator!=(const DeallocatingVectorIterator & other) {
        return mState != other.mState;
    }

    inline bool operator==(const DeallocatingVectorIterator & other) {
        return mState == other.mState;
    }

    inline bool operator<(const DeallocatingVectorIterator & other) const {
        return mState < other.mState;
    }

    inline bool operator>(const DeallocatingVectorIterator & other) const {
        return mState > other.mState;
    }

    inline bool operator>=(const DeallocatingVectorIterator & other) const {
        return mState >= other.mState;
    }

    difference_type operator-(const DeallocatingVectorIterator & other) {
        if(DeallocateC) BOOST_ASSERT(false);
        return mState.mIndex-other.mState.mIndex;
    }
};

template<typename ElementT, std::size_t bucketSizeC = 8388608/sizeof(ElementT) >
class DeallocatingVector {
private:
    std::size_t mCurrentSize;
    std::vector<ElementT *> mBucketList;

public:
    typedef ElementT value_type;
    typedef DeallocatingVectorIterator<ElementT, bucketSizeC, false> iterator;
    typedef DeallocatingVectorIterator<ElementT, bucketSizeC, false> const_iterator;

    //this iterator deallocates all buckets that have been visited. Iterators to visited objects become invalid.
    typedef DeallocatingVectorIterator<ElementT, bucketSizeC, true> deallocation_iterator;

    DeallocatingVector() : mCurrentSize(0) {
        //initial bucket
        mBucketList.push_back(new ElementT[bucketSizeC]);
    }

    ~DeallocatingVector() {
        clear();
    }

    inline void swap(DeallocatingVector<ElementT, bucketSizeC> & other) {
        std::swap(mCurrentSize, other.mCurrentSize);
        mBucketList.swap(other.mBucketList);
    }

    inline void clear() {
        //Delete[]'ing ptr's to all Buckets
        for(unsigned i = 0; i < mBucketList.size(); ++i) {
            if(DEALLOCATION_VECTOR_NULL_PTR != mBucketList[i]) {
                delete[] mBucketList[i];
                mBucketList[i] = DEALLOCATION_VECTOR_NULL_PTR;
            }
        }
        //Removing all ptrs from vector
        std::vector<ElementT *>().swap(mBucketList);
        mCurrentSize = 0;
    }

    inline void push_back(const ElementT & element) {
        std::size_t _capacity = capacity();
        if(mCurrentSize == _capacity) {
            mBucketList.push_back(new ElementT[bucketSizeC]);
        }

        std::size_t _index = size()%bucketSizeC;
        mBucketList.back()[_index] = element;
        ++mCurrentSize;
    }

    inline void reserve(const std::size_t) const {
        //don't do anything
    }

    inline void resize(const std::size_t new_size) {
        if(new_size > mCurrentSize) {
            while(capacity() < new_size) {
                mBucketList.push_back(new ElementT[bucketSizeC]);
            }
            mCurrentSize = new_size;
        }
        if(new_size < mCurrentSize) {
            std::size_t number_of_necessary_buckets = 1+(new_size / bucketSizeC);

            for(unsigned i = number_of_necessary_buckets; i < mBucketList.size(); ++i) {
                delete[] mBucketList[i];
            }
            mBucketList.resize(number_of_necessary_buckets);
            mCurrentSize = new_size;
        }
    }

    inline std::size_t size() const {
        return mCurrentSize;
    }

    inline std::size_t capacity() const {
        return mBucketList.size() * bucketSizeC;
    }

    inline iterator begin() {
        return iterator(static_cast<std::size_t>(0), mBucketList);
    }

    inline iterator end() {
        return iterator(size(), mBucketList);
    }

    inline deallocation_iterator dbegin() {
        return deallocation_iterator(static_cast<std::size_t>(0), mBucketList);
    }

    inline deallocation_iterator dend() {
        return deallocation_iterator(size(), mBucketList);
    }

    inline const_iterator begin() const {
        return const_iterator(static_cast<std::size_t>(0), mBucketList);
    }

    inline const_iterator end() const {
        return const_iterator(size(), mBucketList);
    }

    inline ElementT & operator[](const std::size_t index) {
        std::size_t _bucket = index / bucketSizeC;
        std::size_t _index = index % bucketSizeC;
        return (mBucketList[_bucket][_index]);
    }

    const inline ElementT & operator[](const std::size_t index) const {
        std::size_t _bucket = index / bucketSizeC;
        std::size_t _index = index % bucketSizeC;
        return (mBucketList[_bucket][_index]);
    }

    inline ElementT & back() {
        std::size_t _bucket = mCurrentSize / bucketSizeC;
    	std::size_t _index = mCurrentSize % bucketSizeC;
    	return (mBucketList[_bucket][_index]);
    }

    const inline ElementT & back() const {
        std::size_t _bucket = mCurrentSize / bucketSizeC;
        std::size_t _index = mCurrentSize % bucketSizeC;
    	return (mBucketList[_bucket][_index]);
    }
};

#endif /* DEALLOCATINGVECTOR_H_ */

/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef DEALLOCATINGVECTOR_H_
#define DEALLOCATINGVECTOR_H_

#include <cassert>
#include <cstdlib>
#include <vector>

#if __cplusplus > 199711L
#define DEALLOCATION_VECTOR_NULL_PTR nullptr
#else
#define DEALLOCATION_VECTOR_NULL_PTR NULL
#endif


template<typename ElementT, size_t bucketSizeC = 10485760/sizeof(ElementT), bool DeallocateC = false>
class DeallocatingVectorIterator : public std::iterator<std::random_access_iterator_tag, ElementT> {
protected:

    struct DeallocatingVectorIteratorState {
        //make constructors explicit, so we do not mix random access and deallocation iterators.
        explicit DeallocatingVectorIteratorState() : mData(DEALLOCATION_VECTOR_NULL_PTR), mIndex(-1) { assert(false); }
        explicit DeallocatingVectorIteratorState(const DeallocatingVectorIteratorState &r) : mData(r.mData), mIndex(r.mIndex), mBucketList(r.mBucketList) {}
        //explicit DeallocatingVectorIteratorState(const ElementT * ptr, const size_t idx, const std::vector<ElementT *> & input_list) : mData(ptr), mIndex(idx), mBucketList(input_list) {}
        explicit DeallocatingVectorIteratorState(const size_t idx, std::vector<ElementT *> & input_list) : mData(DEALLOCATION_VECTOR_NULL_PTR), mIndex(idx), mBucketList(input_list) {
            setPointerForIndex();
        }
        ElementT * mData;
        size_t mIndex;
        std::vector<ElementT *> & mBucketList;
        inline void setPointerForIndex() {
            if(bucketSizeC*mBucketList.size() <= mIndex) {
                mData = DEALLOCATION_VECTOR_NULL_PTR;
                return;
            }
            size_t _bucket = mIndex/bucketSizeC;
            size_t _index = mIndex%bucketSizeC;
            mData = &(mBucketList[_bucket][_index]);

            if(DeallocateC) {
                //if we hopped over the border of the previous bucket, then delete that bucket.
                if(0 == _index && _bucket) {
                    delete[] mBucketList[_bucket-1];
                    mBucketList[_bucket-1] = DEALLOCATION_VECTOR_NULL_PTR;
                }
            }

        }
        inline bool operator!=(const DeallocatingVectorIteratorState &other) {
            return (mData != other.mData) || (mIndex != other.mIndex) || (mBucketList != other.mBucketList);
        }

        inline bool operator==(const DeallocatingVectorIteratorState &other) {
            return (mData == other.mData) && (mIndex == other.mIndex) && (mBucketList == other.mBucketList);
        }

        inline bool operator<(const DeallocatingVectorIteratorState &other) {
            return mIndex < other.mIndex;
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
    DeallocatingVectorIterator(size_t idx, std::vector<ElementT *> & input_list) : mState(idx, input_list) {}
    DeallocatingVectorIterator(const DeallocatingVectorIteratorState & r) : mState(r) {}

    template<typename T2>
    DeallocatingVectorIterator& operator=(const DeallocatingVectorIterator<T2> &r) {
        if(DeallocateC) assert(false);
        mState = r.mState; return *this;
    }

    inline DeallocatingVectorIterator& operator++() { //prefix
//        if(DeallocateC) assert(false);
        ++mState.mIndex; mState.setPointerForIndex(); return *this;
    }

    inline DeallocatingVectorIterator& operator--() { //prefix
        if(DeallocateC) assert(false);
        --mState.mIndex; mState.setPointerForIndex(); return *this;
    }

    inline DeallocatingVectorIterator operator++(int) { //postfix
        DeallocatingVectorIteratorState _myState(mState);
        _myState.mIndex++; _myState.setPointerForIndex();
        return DeallocatingVectorIterator(_myState);
    }
    inline DeallocatingVectorIterator operator --(int) { //postfix
        if(DeallocateC) assert(false);
        DeallocatingVectorIteratorState _myState(mState);
        _myState.mIndex--; _myState.setPointerForIndex();
        return DeallocatingVectorIterator(_myState);
    }

    inline DeallocatingVectorIterator operator+(const difference_type& n) const {
        DeallocatingVectorIteratorState _myState(mState);
        _myState.mIndex+=n; _myState.setPointerForIndex();
        return DeallocatingVectorIterator(_myState);
    }

    inline DeallocatingVectorIterator& operator+=(const difference_type& n) const {
        mState.mIndex+=n; return *this;
    }

    inline DeallocatingVectorIterator operator-(const difference_type& n) const {
        if(DeallocateC) assert(false);
        DeallocatingVectorIteratorState _myState(mState);
        _myState.mIndex-=n; _myState.setPointerForIndex();
        return DeallocatingVectorIterator(_myState);
    }

    inline DeallocatingVectorIterator& operator-=(const difference_type &n) const {
        if(DeallocateC) assert(false);
        mState.mIndex-=n; return *this;
    }
    inline reference operator*() const { return *mState.mData; }
    inline pointer operator->() const { return mState.mData; }
    inline reference operator[](const difference_type &n) const {
        if(DeallocateC) assert(false);
        DeallocatingVectorIteratorState _myState(mState);
        _myState.mIndex += n;
        _myState.setPointerForIndex;
        return _myState.mData;
    }

    inline bool operator!=(const DeallocatingVectorIterator & other) {
        return mState != other.mState;
    }

    inline bool operator==(const DeallocatingVectorIterator & other) {
        return mState == other.mState;
    }

    bool operator<(const DeallocatingVectorIterator & other) {
        return mState < other.mState;
    }

    difference_type operator-(const DeallocatingVectorIterator & other) {
        if(DeallocateC) assert(false);
        return mState.mIndex-other.mState.mIndex;
    }
};

template<typename ElementT, size_t bucketSizeC = 10485760/sizeof(ElementT) >
class DeallocatingVector {
private:
    size_t mCurrentSize;
    std::vector<ElementT *> mBucketList;

public:
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
                delete[] (mBucketList[i]);
            }
        }
        //Removing all ptrs from vector
        std::vector<ElementT *>().swap(mBucketList);
    }

    inline void push_back(const ElementT & element) {
        size_t _capacity = capacity();
        if(mCurrentSize == _capacity) {
            mBucketList.push_back(new ElementT[bucketSizeC]);
        }

        size_t _index = size()%bucketSizeC;
        mBucketList.back()[_index] = element;
        ++mCurrentSize;
    }

    inline size_t size() const {
        return mCurrentSize;
    }

    inline size_t capacity() const {
        return mBucketList.size() * bucketSizeC;
    }

    inline iterator begin() {
        return iterator((size_t)0, mBucketList);
    }

    inline iterator end() {
        return iterator(size(), mBucketList);
    }

    inline deallocation_iterator dbegin() {
        return deallocation_iterator((size_t)0, mBucketList);
    }

    inline deallocation_iterator dend() {
        return deallocation_iterator(size(), mBucketList);
    }

    inline const_iterator begin() const {
        return const_iterator((size_t)0, mBucketList);
    }

    inline const_iterator end() const {
        return const_iterator(size(), mBucketList);
    }

    inline ElementT & operator[](const size_t index) {
        size_t _bucket = index / bucketSizeC;
        size_t _index = index % bucketSizeC;
        return (mBucketList[_bucket][_index]);
    }

    const inline ElementT & operator[](const size_t index) const {
        size_t _bucket = index / bucketSizeC;
        size_t _index = index % bucketSizeC;
        return (mBucketList[_bucket][_index]);
    }
};



#endif /* DEALLOCATINGVECTOR_H_ */

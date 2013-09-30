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

#ifndef SHARED_MEMORY_VECTOR_WRAPPER_H
#define SHARED_MEMORY_VECTOR_WRAPPER_H

#include "../Util/SimpleLogger.h"

#include <boost/assert.hpp>
#include <boost/type_traits.hpp>

#include <algorithm>
#include <iterator>
#include <vector>

template<typename DataT>
class ShMemIterator : public std::iterator<std::input_iterator_tag, DataT> {
    DataT * p;
public:
    ShMemIterator(DataT * x) : p(x) {}
    ShMemIterator(const ShMemIterator & mit) : p(mit.p) {}
    ShMemIterator& operator++() {
        ++p;
        return *this;
    }
    ShMemIterator operator++(int) {
        ShMemIterator tmp(*this);
        operator++();
        return tmp;
    }
    ShMemIterator operator+(std::ptrdiff_t diff) {
        ShMemIterator tmp(p+diff);
        return tmp;
    }
    bool operator==(const ShMemIterator& rhs) {
        return p==rhs.p;
    }
    bool operator!=(const ShMemIterator& rhs) {
        return p!=rhs.p;
    }
    DataT& operator*() {
        return *p;
    }
};

template<typename DataT>
class SharedMemoryWrapper {
private:
    DataT * m_ptr;
    std::size_t m_size;

public:
    SharedMemoryWrapper() :
        m_size(0)
    { }

    SharedMemoryWrapper(DataT * ptr, std::size_t size) :
        m_ptr(ptr),
        m_size(size)
    { }

    void swap( SharedMemoryWrapper<DataT> & other ) {
        BOOST_ASSERT_MSG(m_size != 0 || other.size() != 0, "size invalid");
        std::swap( m_size, other.m_size);
        std::swap( m_ptr , other.m_ptr );
    }

    // void SetData(const DataT * ptr, const std::size_t size) {
    //     BOOST_ASSERT_MSG( 0 == m_size, "vector not empty");
    //     BOOST_ASSERT_MSG( 0 < size   , "new vector empty");
    //     m_ptr.reset(ptr);
    //     m_size = size;
    // }

    DataT & at(const std::size_t index) {
        return m_ptr[index];
    }

    const DataT & at(const std::size_t index) const {
        return m_ptr[index];
    }

    ShMemIterator<DataT> begin() const {
        return ShMemIterator<DataT>(m_ptr);
    }

    ShMemIterator<DataT> end() const {
        return ShMemIterator<DataT>(m_ptr+m_size);
    }

    std::size_t size() const { return m_size; }

    DataT & operator[](const unsigned index) {
        BOOST_ASSERT_MSG(index < m_size, "invalid size");
        return m_ptr[index];
    }

    const DataT & operator[](const unsigned index) const {
        BOOST_ASSERT_MSG(index < m_size, "invalid size");
        return m_ptr[index];
    }
};

template<typename DataT, bool UseSharedMemory>
struct ShM {
    typedef typename boost::conditional<
                     UseSharedMemory,
                     SharedMemoryWrapper<DataT>,
                     std::vector<DataT>
                 >::type vector;
 };


#endif //SHARED_MEMORY_VECTOR_WRAPPER_H

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

#include <boost/assert.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits.hpp>

#include <iterator>
#include <vector>

template<typename DataT>
class ShMemIterator : public std::iterator<std::input_iterator_tag, DataT> {
    boost::shared_ptr<DataT> p;
public:
    ShMemIterator(boost::shared_ptr<DataT> & x) :p(x) {}
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
    boost::shared_ptr<DataT> m_ptr;
    std::size_t m_size;

    SharedMemoryWrapper() {};
public:
    SharedMemoryWrapper(const DataT * ptr, std::size_t size) :
        m_ptr(ptr),
        m_size(size)
    { }

    DataT & at(const std::size_t index) {
        return m_ptr[index];
    }

    ShMemIterator<DataT> begin() const {
        return ShMemIterator<DataT>(m_ptr);
    }

    ShMemIterator<DataT> end() const {
        return ShMemIterator<DataT>(m_ptr+m_size);
    }

    std::size_t size() const { return m_size; }

    DataT & operator[](const int index) {
        BOOST_ASSERT_MSG(index < m_size, "invalid size");
        return m_ptr[index];
    }
};

template<typename DataT, bool SharedMemory = false>
class ShMemVector : public
    boost::conditional<
        SharedMemory,
        SharedMemoryWrapper<DataT>,
        std::vector<DataT>
    >::type
{ };


#endif //SHARED_MEMORY_VECTOR_WRAPPER_H

/*

Copyright (c) 2015, Project OSRM contributors
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

#ifndef DIST_TABLE_WRAPPER_H
#define DIST_TABLE_WRAPPER_H

#include <vector>
#include <utility>
#include <boost/assert.hpp>

//This Wrapper provides an easier access to a distance table that is given as an linear vector

template <typename T> class DistTableWrapper {
public:

    using Iterator = typename std::vector<T>::iterator;
    using ConstIterator = typename std::vector<T>::const_iterator;

    DistTableWrapper(std::vector<T> table, std::size_t number_of_nodes)
                     : table_(std::move(table)), number_of_nodes_(number_of_nodes) {
        BOOST_ASSERT_MSG(table.size() == 0, "table is empty");
        BOOST_ASSERT_MSG(number_of_nodes_ * number_of_nodes_ <= table_.size(), "number_of_nodes_ is invalid");
    };

    std::size_t GetNumberOfNodes() const {
        return number_of_nodes_;
    }

    std::size_t size() const {
        return table_.size();
    }

    EdgeWeight operator() (NodeID from, NodeID to) const {
        BOOST_ASSERT_MSG(from < number_of_nodes_, "from ID is out of bound");
        BOOST_ASSERT_MSG(to < number_of_nodes_, "to ID is out of bound");

        const auto index = from * number_of_nodes_ + to;

        BOOST_ASSERT_MSG(index < table_.size(), "index is out of bound");

        return  table_[index];
    }

    ConstIterator begin() const{
        return std::begin(table_);
    }

    Iterator begin() {
        return std::begin(table_);
    }

    ConstIterator end() const{
        return std::end(table_);
    }

    Iterator end() {
        return std::end(table_);
    }

    NodeID GetIndexOfMaxValue() const {
        return std::distance(table_.begin(), std::max_element(table_.begin(), table_.end()));
    }

    std::vector<T> GetTable() const {
        return table_;
    }

private:
    std::vector<T> table_;
    const std::size_t number_of_nodes_;
};


#endif // DIST_TABLE_WRAPPER_H

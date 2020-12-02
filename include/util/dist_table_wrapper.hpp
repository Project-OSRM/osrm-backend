#ifndef DIST_TABLE_WRAPPER_H
#define DIST_TABLE_WRAPPER_H

#include "util/typedefs.hpp"

#include <algorithm>
#include <boost/assert.hpp>
#include <cstddef>
#include <utility>
#include <vector>

namespace osrm
{
namespace util
{

// This Wrapper provides an easier access to a distance table that is given as an linear vector

template <typename T> class DistTableWrapper
{
  public:
    using Iterator = typename std::vector<T>::iterator;
    using ConstIterator = typename std::vector<T>::const_iterator;

    DistTableWrapper(std::vector<T> table, std::size_t number_of_nodes)
        : table_(std::move(table)), number_of_nodes_(number_of_nodes)
    {
        BOOST_ASSERT_MSG(table.size() == 0, "table is empty");
        BOOST_ASSERT_MSG(number_of_nodes_ * number_of_nodes_ <= table_.size(),
                         "number_of_nodes_ is invalid");
    }

    std::size_t GetNumberOfNodes() const { return number_of_nodes_; }

    std::size_t size() const { return table_.size(); }

    EdgeWeight operator()(NodeID from, NodeID to) const
    {
        BOOST_ASSERT_MSG(from < number_of_nodes_, "from ID is out of bound");
        BOOST_ASSERT_MSG(to < number_of_nodes_, "to ID is out of bound");

        const auto index = from * number_of_nodes_ + to;

        BOOST_ASSERT_MSG(index < table_.size(), "index is out of bound");

        return table_[index];
    }

    void SetValue(NodeID from, NodeID to, EdgeWeight value)
    {
        BOOST_ASSERT_MSG(from < number_of_nodes_, "from ID is out of bound");
        BOOST_ASSERT_MSG(to < number_of_nodes_, "to ID is out of bound");

        const auto index = from * number_of_nodes_ + to;

        BOOST_ASSERT_MSG(index < table_.size(), "index is out of bound");

        table_[index] = value;
    }

    ConstIterator begin() const { return std::begin(table_); }

    Iterator begin() { return std::begin(table_); }

    ConstIterator end() const { return std::end(table_); }

    Iterator end() { return std::end(table_); }

    NodeID GetIndexOfMaxValue() const
    {
        return std::distance(table_.begin(), std::max_element(table_.begin(), table_.end()));
    }

    std::vector<T> GetTable() const { return table_; }

  private:
    std::vector<T> table_;
    const std::size_t number_of_nodes_;
};
} // namespace util
} // namespace osrm

#endif // DIST_TABLE_WRAPPER_H

#ifndef OSRM_STATIC_ASSERT_HPP
#define OSRM_STATIC_ASSERT_HPP

#include <type_traits>

namespace osrm
{
namespace util
{

template <typename It, typename Value> inline void static_assert_iter_value()
{
    using IterValueType = typename std::iterator_traits<It>::value_type;
    static_assert(std::is_same<IterValueType, Value>::value, "");
}

template <typename It, typename Category> inline void static_assert_iter_category()
{
    using IterCategoryType = typename std::iterator_traits<It>::iterator_category;
    static_assert(std::is_base_of<Category, IterCategoryType>::value, "");
}

} // ns util
} // ns osrm

#endif // OSRM_STATIC_ASSERT_HPP

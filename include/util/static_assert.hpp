#ifndef OSRM_STATIC_ASSERT_HPP
#define OSRM_STATIC_ASSERT_HPP

#include <iterator>
#include <type_traits>

namespace osrm::util
{

template <typename It, typename Value> inline void static_assert_iter_value()
{
    static_assert(std::is_same_v<std::iter_value_t<It>, Value>, "");
}

template <typename It, typename Category> inline void static_assert_iter_category()
{
    using IterCategoryType = typename std::iterator_traits<It>::iterator_category;
    static_assert(std::is_base_of_v<Category, IterCategoryType>, "");
}

} // namespace osrm::util

#endif // OSRM_STATIC_ASSERT_HPP

#ifndef OSRM_UTIL_FOR_EACH_RANGE_HPP
#define OSRM_UTIL_FOR_EACH_RANGE_HPP

namespace osrm
{
namespace util
{

template <typename Iter, typename Func> void for_each_range(Iter begin, Iter end, Func f)
{
    auto iter = begin;
    while (iter != end)
    {
        const auto key = iter->first;
        auto begin_range = iter;
        while (iter != end && iter->first == key)
        {
            iter++;
        }
        f(begin_range, iter);
    }
}
} // namespace util
} // namespace osrm

#endif

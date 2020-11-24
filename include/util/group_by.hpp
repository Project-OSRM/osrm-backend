#ifndef OSRM_GROUP_BY
#define OSRM_GROUP_BY

#include <algorithm>
#include <utility>

namespace osrm
{
namespace util
{

// Runs fn on consecutive items in sub-ranges determined by pred.
//
// Example:
//   vector<int> v{1,2,2,2,3,4,4};
//   group_by(first, last, even, print);
//   >>> 2,2,2
//   >>> 4,4
//
// Note: this mimics Python's itertools.groupby
template <typename Iter, typename Pred, typename Fn>
Fn group_by(Iter first, Iter last, Pred pred, Fn fn)
{
    while (first != last)
    {
        first = std::find_if(first, last, pred);
        auto next = std::find_if_not(first, last, pred);

        (void)fn(std::make_pair(first, next));

        first = next;
    }

    return fn;
}

} // namespace util
} // namespace osrm

#endif

#ifndef OSRM_STRING_VIEW_HPP
#define OSRM_STRING_VIEW_HPP

#include <boost/functional/hash.hpp>
#include <boost/utility/string_ref.hpp>

namespace osrm
{
namespace util
{
// Convenience typedef: boost::string_ref, boost::string_view or C++17's string_view
using StringView = boost::string_ref;

} // namespace util
} // namespace osrm

// Specializing hash<> for user-defined type in std namespace, this is standard conforming.
namespace std
{
template <> struct hash<::osrm::util::StringView> final
{
    std::size_t operator()(::osrm::util::StringView v) const noexcept
    {
        // Bring into scope and call un-qualified for ADL:
        // remember we want to be able to switch impl. above.
        using std::begin;
        using std::end;

        return boost::hash_range(begin(v), end(v));
    }
};
} // namespace std

#endif /* OSRM_STRING_VIEW_HPP */

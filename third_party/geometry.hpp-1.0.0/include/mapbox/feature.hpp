#pragma once

#include <mapbox/geometry.hpp>

#include <mapbox/variant.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace mapbox {
namespace feature {

struct value;

struct null_value_t
{
};

constexpr bool operator==(const null_value_t&, const null_value_t&) { return true; }
constexpr bool operator!=(const null_value_t&, const null_value_t&) { return false; }
constexpr bool operator<(const null_value_t&, const null_value_t&) { return false; }

constexpr null_value_t null_value = null_value_t();

// Multiple numeric types (uint64_t, int64_t, double) are present in order to support
// the widest possible range of JSON numbers, which do not have a maximum range.
// Implementations that produce `value`s should use that order for type preference,
// using uint64_t for positive integers, int64_t for negative integers, and double
// for non-integers and integers outside the range of 64 bits.
using value_base = mapbox::util::variant<null_value_t, bool, uint64_t, int64_t, double, std::string,
                                         mapbox::util::recursive_wrapper<std::vector<value>>,
                                         mapbox::util::recursive_wrapper<std::unordered_map<std::string, value>>>;

struct value : value_base
{
    using value_base::value_base;
};

using property_map = std::unordered_map<std::string, value>;

// The same considerations and requirement for numeric types apply as for `value_base`.
using identifier = mapbox::util::variant<null_value_t, uint64_t, int64_t, double, std::string>;

template <class T>
struct feature
{
    using coordinate_type = T;
    using geometry_type = mapbox::geometry::geometry<T>; // Fully qualified to avoid GCC -fpermissive error.

    geometry_type geometry;
    property_map properties;
    identifier id;

    feature()
        : geometry(),
          properties(),
          id() {}
    feature(geometry_type const& geom_)
        : geometry(geom_),
          properties(),
          id() {}
    feature(geometry_type&& geom_)
        : geometry(std::move(geom_)),
          properties(),
          id() {}
    feature(geometry_type const& geom_, property_map const& prop_)
        : geometry(geom_), properties(prop_), id() {}
    feature(geometry_type&& geom_, property_map&& prop_)
        : geometry(std::move(geom_)),
          properties(std::move(prop_)),
          id() {}
    feature(geometry_type const& geom_, property_map const& prop_, identifier const& id_)
        : geometry(geom_),
          properties(prop_),
          id(id_) {}
    feature(geometry_type&& geom_, property_map&& prop_, identifier&& id_)
        : geometry(std::move(geom_)),
          properties(std::move(prop_)),
          id(std::move(id_)) {}
};

template <class T>
constexpr bool operator==(feature<T> const& lhs, feature<T> const& rhs)
{
    return lhs.id == rhs.id && lhs.geometry == rhs.geometry && lhs.properties == rhs.properties;
}

template <class T>
constexpr bool operator!=(feature<T> const& lhs, feature<T> const& rhs)
{
    return !(lhs == rhs);
}

template <class T, template <typename...> class Cont = std::vector>
struct feature_collection : Cont<feature<T>>
{
    using coordinate_type = T;
    using feature_type = feature<T>;
    using container_type = Cont<feature_type>;
    using size_type = typename container_type::size_type;

    template <class... Args>
    feature_collection(Args&&... args) : container_type(std::forward<Args>(args)...)
    {
    }
    feature_collection(std::initializer_list<feature_type> args)
        : container_type(std::move(args)) {}
};

} // namespace feature
} // namespace mapbox

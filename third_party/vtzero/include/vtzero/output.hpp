#ifndef VTZERO_OUTPUT_HPP
#define VTZERO_OUTPUT_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file output.hpp
 *
 * @brief Contains overloads of operator<< for basic vtzero types.
 */

#include <vtzero/geometry.hpp>
#include <vtzero/types.hpp>

#include <iosfwd>

namespace vtzero {

    /// Overload of the << operator for GeomType
    template <typename TChar, typename TTraits>
    std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& out, const GeomType type) {
        return out << geom_type_name(type);
    }

    /// Overload of the << operator for property_value_type
    template <typename TChar, typename TTraits>
    std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& out, const property_value_type type) {
        return out << property_value_type_name(type);
    }

    /// Overload of the << operator for index_value
    template <typename TChar, typename TTraits>
    std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& out, const index_value index) {
        if (index.valid()) {
            return out << index.value();
        }
        return out << "invalid";
    }

    /// Overload of the << operator for index_value_pair
    template <typename TChar, typename TTraits>
    std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& out, const index_value_pair index_pair) {
        if (index_pair.valid()) {
            return out << '[' << index_pair.key() << ',' << index_pair.value() << ']';
        }
        return out << "invalid";
    }

    /// Overload of the << operator for points
    template <typename TChar, typename TTraits>
    std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& out, const point p) {
        return out << '(' << p.x << ',' << p.y << ')';
    }

} // namespace vtzero

#endif // VTZERO_OUTPUT_HPP

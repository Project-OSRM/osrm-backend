#ifndef VTZERO_PROPERTY_HPP
#define VTZERO_PROPERTY_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file property.hpp
 *
 * @brief Contains the property class.
 */

#include "property_value.hpp"
#include "types.hpp"

namespace vtzero {

    /**
     * A view of a vector tile property (key and value).
     *
     * Doesn't hold any data itself, just views of the key and value.
     */
    class property {

        data_view m_key{};
        property_value m_value{};

    public:

        /**
         * The default constructor creates an invalid (empty) property.
         */
        constexpr property() noexcept = default;

        /**
         * Create a (valid) property from a key and value.
         */
        constexpr property(const data_view key, const property_value value) noexcept :
            m_key(key),
            m_value(value) {
        }

        /**
         * Is this a valid property? Properties are valid if they were
         * constructed using the non-default constructor.
         */
        constexpr bool valid() const noexcept {
            return m_key.data() != nullptr;
        }

        /**
         * Is this a valid property? Properties are valid if they were
         * constructed using the non-default constructor.
         */
        explicit constexpr operator bool() const noexcept {
            return valid();
        }

        /// Return the property key.
        constexpr data_view key() const noexcept {
            return m_key;
        }

        /// Return the property value.
        constexpr property_value value() const noexcept {
            return m_value;
        }

    }; // class property

    /// properties are equal if they contain the same key & value data.
    inline constexpr bool operator==(const property& lhs, const property& rhs) noexcept {
        return lhs.key() == rhs.key() && lhs.value() == rhs.value();
    }

    /// properties are unequal if they do not contain them same key and value data.
    inline constexpr bool operator!=(const property& lhs, const property& rhs) noexcept {
        return !(lhs == rhs);
    }

} // namespace vtzero

#endif // VTZERO_PROPERTY_HPP

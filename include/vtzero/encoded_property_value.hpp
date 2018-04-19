#ifndef VTZERO_ENCODED_PROPERTY_VALUE_HPP
#define VTZERO_ENCODED_PROPERTY_VALUE_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file encoded_property_value.hpp
 *
 * @brief Contains the encoded_property_value class.
 */

#include "types.hpp"

#include <protozero/pbf_builder.hpp>

#include <functional>
#include <string>

namespace vtzero {

    /**
     * A property value encoded in the vector_tile internal format. Can be
     * created from values of many different types and then later added to
     * a layer/feature.
     */
    class encoded_property_value {

        std::string m_data;

    public:

        /// Construct from vtzero::string_value_type.
        explicit encoded_property_value(string_value_type value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_string(detail::pbf_value::string_value, value.value);
        }

        /// Construct from const char*.
        explicit encoded_property_value(const char* value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_string(detail::pbf_value::string_value, value);
        }

        /// Construct from const char* and size_t.
        explicit encoded_property_value(const char* value, std::size_t size) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_string(detail::pbf_value::string_value, value, size);
        }

        /// Construct from std::string.
        explicit encoded_property_value(const std::string& value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_string(detail::pbf_value::string_value, value);
        }

        /// Construct from vtzero::data_view.
        explicit encoded_property_value(const data_view& value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_string(detail::pbf_value::string_value, value);
        }

        // ------------------

        /// Construct from vtzero::float_value_type.
        explicit encoded_property_value(float_value_type value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_float(detail::pbf_value::float_value, value.value);
        }

        /// Construct from float.
        explicit encoded_property_value(float value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_float(detail::pbf_value::float_value, value);
        }

        // ------------------

        /// Construct from vtzero::double_value_type.
        explicit encoded_property_value(double_value_type value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_double(detail::pbf_value::double_value, value.value);
        }

        /// Construct from double.
        explicit encoded_property_value(double value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_double(detail::pbf_value::double_value, value);
        }

        // ------------------

        /// Construct from vtzero::int_value_type.
        explicit encoded_property_value(int_value_type value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_int64(detail::pbf_value::int_value, value.value);
        }

        /// Construct from int64_t.
        explicit encoded_property_value(int64_t value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_int64(detail::pbf_value::int_value, value);
        }

        /// Construct from int32_t.
        explicit encoded_property_value(int32_t value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_int64(detail::pbf_value::int_value, static_cast<int64_t>(value));
        }

        /// Construct from int16_t.
        explicit encoded_property_value(int16_t value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_int64(detail::pbf_value::int_value, static_cast<int64_t>(value));
        }

        // ------------------

        /// Construct from vtzero::uint_value_type.
        explicit encoded_property_value(uint_value_type value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_uint64(detail::pbf_value::uint_value, value.value);
        }

        /// Construct from uint64_t.
        explicit encoded_property_value(uint64_t value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_uint64(detail::pbf_value::uint_value, value);
        }

        /// Construct from uint32_t.
        explicit encoded_property_value(uint32_t value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_uint64(detail::pbf_value::uint_value, static_cast<uint64_t>(value));
        }

        /// Construct from uint16_t.
        explicit encoded_property_value(uint16_t value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_uint64(detail::pbf_value::uint_value, static_cast<uint64_t>(value));
        }

        // ------------------

        /// Construct from vtzero::sint_value_type.
        explicit encoded_property_value(sint_value_type value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_sint64(detail::pbf_value::sint_value, value.value);
        }

        // ------------------

        /// Construct from vtzero::bool_value_type.
        explicit encoded_property_value(bool_value_type value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_bool(detail::pbf_value::bool_value, value.value);
        }

        /// Construct from bool.
        explicit encoded_property_value(bool value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_bool(detail::pbf_value::bool_value, value);
        }

        // ------------------

        /**
         * Get view of the raw data stored inside.
         */
        data_view data() const noexcept {
            return {m_data.data(), m_data.size()};
        }

        /**
         * Hash function compatible with std::hash.
         */
        std::size_t hash() const noexcept {
            return std::hash<std::string>{}(m_data);
        }

    }; // class encoded_property_value

    /// Encoded property values are equal if they contain the same data.
    inline bool operator==(const encoded_property_value& lhs, const encoded_property_value& rhs) noexcept {
        return lhs.data() == rhs.data();
    }

    /// Encoded property values are unequal if they are not equal.
    inline bool operator!=(const encoded_property_value& lhs, const encoded_property_value& rhs) noexcept {
        return !(lhs == rhs);
    }

    /// Arbitrary ordering based on internal data.
    inline bool operator<(const encoded_property_value& lhs, const encoded_property_value& rhs) noexcept {
        return lhs.data() < rhs.data();
    }

    /// Arbitrary ordering based on internal data.
    inline bool operator<=(const encoded_property_value& lhs, const encoded_property_value& rhs) noexcept {
        return lhs.data() <= rhs.data();
    }

    /// Arbitrary ordering based on internal data.
    inline bool operator>(const encoded_property_value& lhs, const encoded_property_value& rhs) noexcept {
        return lhs.data() > rhs.data();
    }

    /// Arbitrary ordering based on internal data.
    inline bool operator>=(const encoded_property_value& lhs, const encoded_property_value& rhs) noexcept {
        return lhs.data() >= rhs.data();
    }

} // namespace vtzero

namespace std {

    /**
     * Specialization of std::hash for encoded_property_value.
     */
    template <>
    struct hash<vtzero::encoded_property_value> {

        /// key vtzero::encoded_property_value
        using argument_type = vtzero::encoded_property_value;

        /// hash result: size_t
        using result_type = std::size_t;

        /// calculate the hash of the argument
        std::size_t operator()(const vtzero::encoded_property_value& value) const noexcept {
            return value.hash();
        }

    }; // struct hash

} // namespace std

#endif // VTZERO_ENCODED_PROPERTY_VALUE_HPP

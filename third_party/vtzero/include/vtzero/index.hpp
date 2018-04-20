#ifndef VTZERO_INDEX_HPP
#define VTZERO_INDEX_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file index.hpp
 *
 * @brief Contains classes for indexing the key/value tables inside layers.
 */

#include "builder.hpp"
#include "types.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace vtzero {

    /**
     * Used to store the mapping between property keys and the index value
     * in the table stored in a layer.
     *
     * @tparam TMap The map class to use (std::map, std::unordered_map or
     *         something compatible).
     */
    template <template <typename...> class TMap>
    class key_index {

        layer_builder& m_builder;

        TMap<std::string, index_value> m_index;

    public:

        /**
         * Construct index.
         *
         * @param builder The layer we are building containing the key table
         *        we are creating the index for.
         */
        explicit key_index(layer_builder& builder) :
            m_builder(builder) {
        }

        /**
         * Get the index value for the specified key. If the key was not in
         * the table, it will be added.
         *
         * @param key The key to store.
         * @returns The index value of they key.
         */
        index_value operator()(const data_view key) {
            std::string text(key);
            const auto it = m_index.find(text);
            if (it == m_index.end()) {
                const auto idx = m_builder.add_key_without_dup_check(key);
                m_index.emplace(std::move(text), idx);
                return idx;
            }
            return it->second;
        }

    }; // class key_index

    /**
     * Used to store the mapping between property values and the index value
     * in the table stored in a layer. Stores the values in their original
     * form (as type TExternal) which is more efficient than the way
     * value_index_internal does it, but you need an instance of this class
     * for each value type you use.
     *
     * @tparam TInternal The type used in the vector tile to encode the value.
     *         Must be one of string/float/double/int/uint/sint/bool_value_type.
     * @tparam TExternal The type for the value used by the user of this class.
     * @tparam TMap The map class to use (std::map, std::unordered_map or
     *         something compatible).
     */
    template <typename TInternal, typename TExternal, template <typename...> class TMap>
    class value_index {

        layer_builder& m_builder;

        TMap<TExternal, index_value> m_index;

    public:

        /**
         * Construct index.
         *
         * @param builder The layer we are building containing the value
         *        table we are creating the index for.
         */
        explicit value_index(layer_builder& builder) :
            m_builder(builder) {
        }

        /**
         * Get the index value for the specified value. If the value was not in
         * the table, it will be added.
         *
         * @param value The value to store.
         * @returns The index value of they value.
         */
        index_value operator()(const TExternal& value) {
            const auto it = m_index.find(value);
            if (it == m_index.end()) {
                const auto idx = m_builder.add_value_without_dup_check(encoded_property_value{TInternal{value}});
                m_index.emplace(value, idx);
                return idx;
            }
            return it->second;
        }

    }; // class value_index

    /**
     * Used to store the mapping between bool property values and the index
     * value in the table stored in a layer.
     *
     * This is the most efficient index if you know that your property values
     * are bools.
     */
    class value_index_bool {

        layer_builder& m_builder;

        std::array<index_value, 2> m_index;

    public:

        /**
         * Construct index.
         *
         * @param builder The layer we are building containing the value
         *        table we are creating the index for.
         */
        explicit value_index_bool(layer_builder& builder) :
            m_builder(builder) {
        }

        /**
         * Get the index value for the specified value. If the value was not in
         * the table, it will be added.
         *
         * @param value The value to store.
         * @returns The index value of they value.
         */
        index_value operator()(const bool value) {
            auto& idx = m_index[static_cast<std::size_t>(value)]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            if (!idx.valid()) {
                idx = m_builder.add_value_without_dup_check(encoded_property_value{value});
            }
            return idx;
        }

    }; // class value_index_bool

    /**
     * Used to store the mapping between small unsigned int property values and
     * the index value in the table stored in a layer.
     *
     * This is the most efficient index if you know that your property values
     * are densly used small unsigned integers. This is usually the case for
     * enums.
     *
     * Interally a simple vector<index_value> is used and the value is used
     * as is to look up the index value in the vector.
     */
    class value_index_small_uint {

        layer_builder& m_builder;

        std::vector<index_value> m_index;

    public:

        /**
         * Construct index.
         *
         * @param builder The layer we are building containing the value
         *        table we are creating the index for.
         */
        explicit value_index_small_uint(layer_builder& builder) :
            m_builder(builder) {
        }

        /**
         * Get the index value for the specified value. If the value was not in
         * the table, it will be added.
         *
         * @param value The value to store.
         * @returns The index value of they value.
         */
        index_value operator()(const uint16_t value) {
            if (value >= m_index.size()) {
                m_index.resize(value + 1);
            }
            if (!m_index[value].valid()) {
                m_index[value] = m_builder.add_value_without_dup_check(encoded_property_value{value});
            }
            return m_index[value];
        }

    }; // class value_index_small_uint

    /**
     * Used to store the mapping between property values and the index value
     * in the table stored in a layer. Stores the values in the already
     * encoded form. This is simpler to use than the value_index class, but
     * has a higher overhead.
     *
     * @tparam TMap The map class to use (std::map, std::unordered_map or
     *         something compatible).
     */
    template <template <typename...> class TMap>
    class value_index_internal {

        layer_builder& m_builder;

        TMap<encoded_property_value, index_value> m_index;

    public:

        /**
         * Construct index.
         *
         * @param builder The layer we are building containing the value
         *        table we are creating the index for.
         */
        explicit value_index_internal(layer_builder& builder) :
            m_builder(builder) {
        }

        /**
         * Get the index value for the specified value. If the value was not in
         * the table, it will be added.
         *
         * @param value The value to store.
         * @returns The index value of they value.
         */
        index_value operator()(const encoded_property_value& value) {
            const auto it = m_index.find(value);
            if (it == m_index.end()) {
                const auto idx = m_builder.add_value_without_dup_check(value);
                m_index.emplace(value, idx);
                return idx;
            }
            return it->second;
        }

    }; // class value_index_internal

} // namespace vtzero

#endif // VTZERO_INDEX_HPP

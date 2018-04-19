#ifndef VTZERO_PROPERTY_MAPPER_HPP
#define VTZERO_PROPERTY_MAPPER_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file property_mapper.hpp
 *
 * @brief Contains the property_mapper class.
 */

#include "builder.hpp"
#include "layer.hpp"

#include <vector>

namespace vtzero {

    /**
     * Establishes a mapping between index values of properties of an existing
     * layer and a new layer. Can be used when copying some features from an
     * existing layer to a new layer.
     */
    class property_mapper {

        const layer& m_layer;
        layer_builder& m_layer_builder;

        std::vector<index_value> m_keys;
        std::vector<index_value> m_values;

    public:

        /**
         * Construct the mapping between the specified layers
         *
         * @param layer The existing layer from which (some) properties will
         *        be copied.
         * @param layer_builder The new layer that is being created.
         */
        property_mapper(const layer& layer, layer_builder& layer_builder) :
            m_layer(layer),
            m_layer_builder(layer_builder) {
            m_keys.resize(layer.key_table().size());
            m_values.resize(layer.value_table().size());
        }

        /**
         * Map the value index of a key.
         *
         * @param index The value index of the key in the existing table.
         * @returns The value index of the same key in the new table.
         */
        index_value map_key(index_value index) {
            auto& k = m_keys[index.value()];

            if (!k.valid()) {
                k = m_layer_builder.add_key_without_dup_check(m_layer.key(index));
            }

            return k;
        }

        /**
         * Map the value index of a value.
         *
         * @param index The value index of the value in the existing table.
         * @returns The value index of the same value in the new table.
         */
        index_value map_value(index_value index) {
            auto& v = m_values[index.value()];

            if (!v.valid()) {
                v = m_layer_builder.add_value_without_dup_check(m_layer.value(index));
            }

            return v;
        }

        /**
         * Map the value indexes of a key/value pair.
         *
         * @param idxs The value indexes of the key/value pair in the existing
         *        table.
         * @returns The value indexes of the same key/value pair in the new
         *          table.
         */
        index_value_pair operator()(index_value_pair idxs) {
            return {map_key(idxs.key()), map_value(idxs.value())};
        }

    }; // class property_mapper

} // namespace vtzero

#endif // VTZERO_PROPERTY_MAPPER_HPP

#ifndef VTZERO_LAYER_HPP
#define VTZERO_LAYER_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file layer.hpp
 *
 * @brief Contains the layer class.
 */

#include "exception.hpp"
#include "feature.hpp"
#include "geometry.hpp"
#include "property_value.hpp"
#include "types.hpp"

#include <protozero/pbf_message.hpp>

#include <cstdint>
#include <iterator>
#include <vector>

namespace vtzero {

    /**
     * A layer according to spec 4.1. It contains a version, the extent,
     * and a name. For the most efficient way to access the features in this
     * layer call next_feature() until it returns an invalid feature:
     *
     * @code
     *   std::string data = ...;
     *   vector_tile tile{data};
     *   layer = tile.next_layer();
     *   while (auto feature = layer.next_feature()) {
     *     ...
     *   }
     * @endcode
     *
     * If you know the ID of a feature, you can get it directly with
     * @code
     *   layer.get_feature_by_id(7);
     * @endcode
     */
    class layer {

        data_view m_data{};
        uint32_t m_version = 1; // defaults to 1, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L55
        uint32_t m_extent = 4096; // defaults to 4096, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L70
        std::size_t m_num_features = 0;
        data_view m_name{};
        protozero::pbf_message<detail::pbf_layer> m_layer_reader{m_data};
        mutable std::vector<data_view> m_key_table;
        mutable std::vector<property_value> m_value_table;
        mutable std::size_t m_key_table_size = 0;
        mutable std::size_t m_value_table_size = 0;

        void initialize_tables() const {
            m_key_table.reserve(m_key_table_size);
            m_key_table_size = 0;

            m_value_table.reserve(m_value_table_size);
            m_value_table_size = 0;

            protozero::pbf_message<detail::pbf_layer> reader{m_data};
            while (reader.next()) {
                switch (reader.tag_and_type()) {
                    case protozero::tag_and_type(detail::pbf_layer::keys, protozero::pbf_wire_type::length_delimited):
                        m_key_table.push_back(reader.get_view());
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::values, protozero::pbf_wire_type::length_delimited):
                        m_value_table.emplace_back(reader.get_view());
                        break;
                    default:
                        reader.skip(); // ignore unknown fields
                }
            }
        }

    public:

        /**
         * Construct an invalid layer object.
         */
        layer() = default;

        /**
         * Construct a layer object. This is usually not something done in
         * user code, because layers are created by the tile_iterator.
         *
         * @throws format_exception if the layer data is ill-formed.
         * @throws version_exception if the layer contains an unsupported version
         *                           number (only version 1 and 2 are supported)
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        explicit layer(const data_view data) :
            m_data(data) {
            protozero::pbf_message<detail::pbf_layer> reader{data};
            while (reader.next()) {
                switch (reader.tag_and_type()) {
                    case protozero::tag_and_type(detail::pbf_layer::version, protozero::pbf_wire_type::varint):
                        m_version = reader.get_uint32();
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::name, protozero::pbf_wire_type::length_delimited):
                        m_name = reader.get_view();
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::features, protozero::pbf_wire_type::length_delimited):
                        reader.skip(); // ignore features for now
                        ++m_num_features;
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::keys, protozero::pbf_wire_type::length_delimited):
                        reader.skip();
                        ++m_key_table_size;
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::values, protozero::pbf_wire_type::length_delimited):
                        reader.skip();
                        ++m_value_table_size;
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::extent, protozero::pbf_wire_type::varint):
                        m_extent = reader.get_uint32();
                        break;
                    default:
                        throw format_exception{"unknown field in layer (tag=" +
                                               std::to_string(static_cast<uint32_t>(reader.tag())) +
                                               ", type=" +
                                               std::to_string(static_cast<uint32_t>(reader.wire_type())) +
                                               ")"};
                }
            }

            // This library can only handle version 1 and 2.
            if (m_version < 1 || m_version > 2) {
                throw version_exception{m_version};
            }

            // 4.1 "A layer MUST contain a name field."
            if (m_name.data() == nullptr) {
                throw format_exception{"missing name field in layer (spec 4.1)"};
            }
        }

        /**
         * Is this a valid layer? Valid layers are those not created from the
         * default constructor.
         */
        bool valid() const noexcept {
            return m_data.data() != nullptr;
        }

        /**
         * Is this a valid layer? Valid layers are those not created from the
         * default constructor.
         */
        explicit operator bool() const noexcept {
            return valid();
        }

        /**
         * Get a reference to the raw data this layer is created from.
         */
        data_view data() const noexcept {
            return m_data;
        }

        /**
         * Return the name of the layer.
         *
         * @pre @code valid() @endcode
         */
        data_view name() const noexcept {
            vtzero_assert_in_noexcept_function(valid());

            return m_name;
        }

        /**
         * Return the version of this layer.
         *
         * @pre @code valid() @endcode
         */
        std::uint32_t version() const noexcept {
            vtzero_assert_in_noexcept_function(valid());

            return m_version;
        }

        /**
         * Return the extent of this layer.
         *
         * @pre @code valid() @endcode
         */
        std::uint32_t extent() const noexcept {
            vtzero_assert_in_noexcept_function(valid());

            return m_extent;
        }

        /**
         * Does this layer contain any features?
         *
         * Complexity: Constant.
         */
        bool empty() const noexcept {
            return m_num_features == 0;
        }

        /**
         * The number of features in this layer.
         *
         * Complexity: Constant.
         */
        std::size_t num_features() const noexcept {
            return m_num_features;
        }

        /**
         * Return a reference to the key table.
         *
         * Complexity: Amortized constant. First time the table is needed
         *             it needs to be created.
         *
         * @pre @code valid() @endcode
         */
        const std::vector<data_view>& key_table() const {
            vtzero_assert(valid());

            if (m_key_table_size > 0) {
                initialize_tables();
            }
            return m_key_table;
        }

        /**
         * Return a reference to the value table.
         *
         * Complexity: Amortized constant. First time the table is needed
         *             it needs to be created.
         *
         * @pre @code valid() @endcode
         */
        const std::vector<property_value>& value_table() const {
            vtzero_assert(valid());

            if (m_value_table_size > 0) {
                initialize_tables();
            }
            return m_value_table;
        }

        /**
         * Return the size of the key table. This returns the correct value
         * whether the key table was already built or not.
         *
         * Complexity: Constant.
         *
         * @returns Size of the key table.
         */
        std::size_t key_table_size() const noexcept {
            return m_key_table_size > 0 ? m_key_table_size : m_key_table.size();
        }

        /**
         * Return the size of the value table. This returns the correct value
         * whether the value table was already built or not.
         *
         * Complexity: Constant.
         *
         * @returns Size of the value table.
         */
        std::size_t value_table_size() const noexcept {
            return m_value_table_size > 0 ? m_value_table_size : m_value_table.size();
        }

        /**
         * Get the property key with the given index.
         *
         * Complexity: Amortized constant. First time the table is needed
         *             it needs to be created.
         *
         * @throws out_of_range_exception if the index is out of range.
         * @pre @code valid() @endcode
         */
        data_view key(index_value index) const {
            vtzero_assert(valid());

            const auto& table = key_table();
            if (index.value() >= table.size()) {
                throw out_of_range_exception{index.value()};
            }

            return table[index.value()];
        }

        /**
         * Get the property value with the given index.
         *
         * Complexity: Amortized constant. First time the table is needed
         *             it needs to be created.
         *
         * @throws out_of_range_exception if the index is out of range.
         * @pre @code valid() @endcode
         */
        property_value value(index_value index) const {
            vtzero_assert(valid());

            const auto& table = value_table();
            if (index.value() >= table.size()) {
                throw out_of_range_exception{index.value()};
            }

            return table[index.value()];
        }

        /**
         * Get the next feature in this layer.
         *
         * Note that the feature returned will internally contain a pointer to
         * the layer it came from. The layer has to stay valid as long as the
         * feature is used.
         *
         * Complexity: Constant.
         *
         * @returns The next feature or the invalid feature if there are no
         *          more features.
         * @throws format_exception if the layer data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         * @pre @code valid() @endcode
         */
        feature next_feature() {
            vtzero_assert(valid());

            const bool has_next = m_layer_reader.next(detail::pbf_layer::features,
                                                      protozero::pbf_wire_type::length_delimited);

            return has_next ? feature{this, m_layer_reader.get_view()} : feature{};
        }

        /**
         * Reset the feature iterator. The next time next_feature() is called,
         * it will begin from the first feature again.
         *
         * Complexity: Constant.
         *
         * @pre @code valid() @endcode
         */
        void reset_feature() noexcept {
            vtzero_assert_in_noexcept_function(valid());

            m_layer_reader = protozero::pbf_message<detail::pbf_layer>{m_data};
        }

        /**
         * Call a function for each feature in this layer.
         *
         * @tparam The type of the function. It must take a single argument
         *         of type feature&& and return a bool. If the function returns
         *         false, the iteration will be stopped.
         * @param func The function to call.
         * @returns true if the iteration was completed and false otherwise.
         * @pre @code valid() @endcode
         */
        template <typename TFunc>
        bool for_each_feature(TFunc&& func) const {
            vtzero_assert(valid());

            protozero::pbf_message<detail::pbf_layer> layer_reader{m_data};
            while (layer_reader.next(detail::pbf_layer::features,
                                     protozero::pbf_wire_type::length_delimited)) {
                if (!std::forward<TFunc>(func)(feature{this, layer_reader.get_view()})) {
                    return false;
                }
            }

            return true;
        }

        /**
         * Get the feature with the specified ID. If there are several features
         * with the same ID, it is undefined which one you'll get.
         *
         * Note that the feature returned will internally contain a pointer to
         * the layer it came from. The layer has to stay valid as long as the
         * feature is used.
         *
         * Complexity: Linear in the number of features.
         *
         * @param id The ID to look for.
         * @returns Feature with the specified ID or the invalid feature if
         *          there is no feature with this ID.
         * @throws format_exception if the layer data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         * @pre @code valid() @endcode
         */
        feature get_feature_by_id(uint64_t id) const {
            vtzero_assert(valid());

            protozero::pbf_message<detail::pbf_layer> layer_reader{m_data};
            while (layer_reader.next(detail::pbf_layer::features, protozero::pbf_wire_type::length_delimited)) {
                const auto feature_data = layer_reader.get_view();
                protozero::pbf_message<detail::pbf_feature> feature_reader{feature_data};
                if (feature_reader.next(detail::pbf_feature::id, protozero::pbf_wire_type::varint)) {
                    if (feature_reader.get_uint64() == id) {
                        return feature{this, feature_data};
                    }
                }
            }

            return feature{};
        }

    }; // class layer

    inline property feature::next_property() {
        const auto idxs = next_property_indexes();
        property p{};
        if (idxs.valid()) {
            p = {m_layer->key(idxs.key()),
                 m_layer->value(idxs.value())};
        }
        return p;
    }

    inline index_value_pair feature::next_property_indexes() {
        vtzero_assert(valid());
        if (m_property_iterator == m_properties.end()) {
            return {};
        }

        const auto ki = *m_property_iterator++;
        if (!index_value{ki}.valid()) {
            throw out_of_range_exception{ki};
        }

        assert(m_property_iterator != m_properties.end());
        const auto vi = *m_property_iterator++;
        if (!index_value{vi}.valid()) {
            throw out_of_range_exception{vi};
        }

        if (ki >= m_layer->key_table_size()) {
            throw out_of_range_exception{ki};
        }

        if (vi >= m_layer->value_table_size()) {
            throw out_of_range_exception{vi};
        }

        return {ki, vi};
    }

    template <typename TFunc>
    bool feature::for_each_property(TFunc&& func) const {
        vtzero_assert(valid());

        for (auto it = m_properties.begin(); it != m_properties.end();) {
            const uint32_t ki = *it++;
            if (!index_value{ki}.valid()) {
                throw out_of_range_exception{ki};
            }

            assert(m_property_iterator != m_properties.end());
            const uint32_t vi = *it++;
            if (!index_value{vi}.valid()) {
                throw out_of_range_exception{vi};
            }

            if (!std::forward<TFunc>(func)(property{m_layer->key(ki), m_layer->value(vi)})) {
                return false;
            }
        }

        return true;
    }

} // namespace vtzero

#endif // VTZERO_LAYER_HPP

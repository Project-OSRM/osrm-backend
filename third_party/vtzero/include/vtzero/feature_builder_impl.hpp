#ifndef VTZERO_FEATURE_BUILDER_IMPL_HPP
#define VTZERO_FEATURE_BUILDER_IMPL_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file feature_builder_impl.hpp
 *
 * @brief Contains classes internal to the builder.
 */

#include "builder_impl.hpp"
#include "encoded_property_value.hpp"
#include "geometry.hpp"
#include "property.hpp"
#include "property_value.hpp"

#include <utility>

namespace vtzero {

    namespace detail {

        class feature_builder_base {

            layer_builder_impl* m_layer;

            void add_key_internal(index_value idx) {
                vtzero_assert(idx.valid());
                m_pbf_tags.add_element(idx.value());
            }

            template <typename T>
            void add_key_internal(T&& key) {
                add_key_internal(m_layer->add_key(data_view{std::forward<T>(key)}));
            }

            void add_value_internal(index_value idx) {
                vtzero_assert(idx.valid());
                m_pbf_tags.add_element(idx.value());
            }

            void add_value_internal(property_value value) {
                add_value_internal(m_layer->add_value(value));
            }

            template <typename T>
            void add_value_internal(T&& value) {
                encoded_property_value v{std::forward<T>(value)};
                add_value_internal(m_layer->add_value(v));
            }

        protected:

            protozero::pbf_builder<detail::pbf_feature> m_feature_writer;
            protozero::packed_field_uint32 m_pbf_tags;

            explicit feature_builder_base(layer_builder_impl* layer) :
                m_layer(layer),
                m_feature_writer(layer->message(), detail::pbf_layer::features) {
            }

            ~feature_builder_base() noexcept = default;

            feature_builder_base(const feature_builder_base&) = delete; // NOLINT(hicpp-use-equals-delete, modernize-use-equals-delete)

            feature_builder_base& operator=(const feature_builder_base&) = delete; // NOLINT(hicpp-use-equals-delete, modernize-use-equals-delete)
                                                                                   // The check wants these functions to be public...

            feature_builder_base(feature_builder_base&&) noexcept = default;

            feature_builder_base& operator=(feature_builder_base&&) noexcept = default;

            void add_property_impl(const property& property) {
                add_key_internal(property.key());
                add_value_internal(property.value());
            }

            void add_property_impl(const index_value_pair idxs) {
                add_key_internal(idxs.key());
                add_value_internal(idxs.value());
            }

            template <typename TKey, typename TValue>
            void add_property_impl(TKey&& key, TValue&& value) {
                add_key_internal(std::forward<TKey>(key));
                add_value_internal(std::forward<TValue>(value));
            }

            void do_commit() {
                if (m_pbf_tags.valid()) {
                    m_pbf_tags.commit();
                }
                m_feature_writer.commit();
                m_layer->increment_feature_count();
            }

            void do_rollback() {
                if (m_pbf_tags.valid()) {
                    m_pbf_tags.rollback();
                }
                m_feature_writer.rollback();
            }

        }; // class feature_builder_base

    } // namespace detail

} // namespace vtzero

#endif // VTZERO_FEATURE_BUILDER_IMPL_HPP

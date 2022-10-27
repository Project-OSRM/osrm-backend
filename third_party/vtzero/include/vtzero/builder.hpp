#ifndef VTZERO_BUILDER_HPP
#define VTZERO_BUILDER_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file builder.hpp
 *
 * @brief Contains the classes and functions to build vector tiles.
 */

#include "builder_impl.hpp"
#include "feature_builder_impl.hpp"
#include "geometry.hpp"
#include "types.hpp"
#include "vector_tile.hpp"

#include <protozero/pbf_builder.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace vtzero {

    /**
     * Used to build vector tiles. Whenever you are building a new vector
     * tile, start with an object of this class and add layers. After all
     * the data is added, call serialize().
     *
     * @code
     * layer some_existing_layer = ...;
     *
     * tile_builder builder;
     * layer_builder layer_roads{builder, "roads"};
     * builder.add_existing_layer(some_existing_layer);
     * ...
     * std::string data = builder.serialize();
     * @endcode
     */
    class tile_builder {

        friend class layer_builder;

        std::vector<std::unique_ptr<detail::layer_builder_base>> m_layers;

        /**
         * Add a new layer to the vector tile based on an existing layer. The
         * new layer will have the same name, version, and extent as the
         * existing layer. The new layer will not contain any features. This
         * method is handy when copying some (but not all) data from an
         * existing layer.
         */
        detail::layer_builder_impl* add_layer(const layer& layer) {
            const auto ptr = new detail::layer_builder_impl{layer.name(), layer.version(), layer.extent()};
            m_layers.emplace_back(ptr);
            return ptr;
        }

        /**
         * Add a new layer to the vector tile with the specified name, version,
         * and extent.
         *
         * @tparam TString Some string type (const char*, std::string,
         *         vtzero::data_view) or something that converts to one of
         *         these types.
         * @param name Name of this layer.
         * @param version Version of this layer (only version 1 and 2 are
         *                supported)
         * @param extent Extent used for this layer.
         */
        template <typename TString>
        detail::layer_builder_impl* add_layer(TString&& name, uint32_t version, uint32_t extent) {
            const auto ptr = new detail::layer_builder_impl{std::forward<TString>(name), version, extent};
            m_layers.emplace_back(ptr);
            return ptr;
        }

    public:

        /// Constructor
        tile_builder() = default;

        /// Destructor
        ~tile_builder() noexcept = default;

        /// Tile builders can not be copied.
        tile_builder(const tile_builder&) = delete;

        /// Tile builders can not be copied.
        tile_builder& operator=(const tile_builder&) = delete;

        /// Tile builders can be moved.
        tile_builder(tile_builder&&) = default;

        /// Tile builders can be moved.
        tile_builder& operator=(tile_builder&&) = default;

        /**
         * Add an existing layer to the vector tile. The layer data will be
         * copied over into the new vector_tile when the serialize() method
         * is called. Until then, the data referenced here must stay available.
         *
         * @param data Reference to some data that must be a valid encoded
         *        layer.
         */
        void add_existing_layer(data_view&& data) {
            m_layers.emplace_back(new detail::layer_builder_existing{std::forward<data_view>(data)});
        }

        /**
         * Add an existing layer to the vector tile. The layer data will be
         * copied over into the new vector_tile when the serialize() method
         * is called. Until then, the data referenced here must stay available.
         *
         * @param layer Reference to the layer to be copied.
         */
        void add_existing_layer(const layer& layer) {
            add_existing_layer(layer.data());
        }

        /**
         * Serialize the data accumulated in this builder into a vector tile.
         * The data will be appended to the specified buffer. The buffer
         * doesn't have to be empty.
         *
         * @param buffer Buffer to append the encoded vector tile to.
         */
        void serialize(std::string& buffer) const {
            std::size_t estimated_size = 0;
            for (const auto& layer : m_layers) {
                estimated_size += layer->estimated_size();
            }

            buffer.reserve(buffer.size() + estimated_size);

            protozero::pbf_builder<detail::pbf_tile> pbf_tile_builder{buffer};
            for (const auto& layer : m_layers) {
                layer->build(pbf_tile_builder);
            }
        }

        /**
         * Serialize the data accumulated in this builder into a vector_tile
         * and return it.
         *
         * If you want to use an existing buffer instead, use the serialize()
         * method taking a std::string& as parameter.
         *
         * @returns std::string Buffer with encoded vector_tile data.
         */
        std::string serialize() const {
            std::string data;
            serialize(data);
            return data;
        }

    }; // class tile_builder

    /**
     * The layer_builder is used to add a new layer to a vector tile that is
     * being built.
     */
    class layer_builder {

        vtzero::detail::layer_builder_impl* m_layer;

        friend class geometry_feature_builder;
        friend class point_feature_builder;
        friend class linestring_feature_builder;
        friend class polygon_feature_builder;

        vtzero::detail::layer_builder_impl& get_layer_impl() noexcept {
            return *m_layer;
        }

        template <typename T>
        using is_layer = std::is_same<typename std::remove_cv<typename std::remove_reference<T>::type>::type, layer>;

    public:

        /**
         * Construct a layer_builder to build a new layer with the same name,
         * version, and extent as an existing layer.
         *
         * @param tile The tile builder we want to create this layer in.
         * @param layer Existing layer we want to use the name, version, and
         *        extent from
         */
        layer_builder(vtzero::tile_builder& tile, const layer& layer) :
            m_layer(tile.add_layer(layer)) {
        }

        /**
         * Construct a layer_builder to build a completely new layer.
         *
         * @tparam TString Some string type (such as std::string or const char*)
         * @param tile The tile builder we want to create this layer in.
         * @param name The name of the new layer.
         * @param version The vector tile spec version of the new layer.
         * @param extent The extent of the new layer.
         */
        template <typename TString, typename std::enable_if<!is_layer<TString>::value, int>::type = 0>
        layer_builder(vtzero::tile_builder& tile, TString&& name, uint32_t version = 2, uint32_t extent = 4096) :
            m_layer(tile.add_layer(std::forward<TString>(name), version, extent)) {
        }

        /**
         * Add key to the keys table without checking for duplicates. This
         * function is usually used when an external index is used which takes
         * care of the duplication check.
         *
         * @param text The key.
         * @returns The index value of this key.
         */
        index_value add_key_without_dup_check(const data_view text) {
            return m_layer->add_key_without_dup_check(text);
        }

        /**
         * Add key to the keys table. This function will consult the internal
         * index in the layer to make sure the key is only in the table once.
         * It will either return the index value of an existing key or add the
         * new key and return its index value.
         *
         * @param text The key.
         * @returns The index value of this key.
         */
        index_value add_key(const data_view text) {
            return m_layer->add_key(text);
        }

        /**
         * Add value to the values table without checking for duplicates. This
         * function is usually used when an external index is used which takes
         * care of the duplication check.
         *
         * @param value The property value.
         * @returns The index value of this value.
         */
        index_value add_value_without_dup_check(const property_value value) {
            return m_layer->add_value_without_dup_check(value);
        }

        /**
         * Add value to the values table without checking for duplicates. This
         * function is usually used when an external index is used which takes
         * care of the duplication check.
         *
         * @param value The property value.
         * @returns The index value of this value.
         */
        index_value add_value_without_dup_check(const encoded_property_value& value) {
            return m_layer->add_value_without_dup_check(value);
        }

        /**
         * Add value to the values table. This function will consult the
         * internal index in the layer to make sure the value is only in the
         * table once. It will either return the index value of an existing
         * value or add the new value and return its index value.
         *
         * @param value The property value.
         * @returns The index value of this value.
         */
        index_value add_value(const property_value value) {
            return m_layer->add_value(value);
        }

        /**
         * Add value to the values table. This function will consult the
         * internal index in the layer to make sure the value is only in the
         * table once. It will either return the index value of an existing
         * value or add the new value and return its index value.
         *
         * @param value The property value.
         * @returns The index value of this value.
         */
        index_value add_value(const encoded_property_value& value) {
            return m_layer->add_value(value);
        }

        /**
         * Add a feature from an existing layer to the new layer. The feature
         * will be copied completely over to the new layer including its
         * geometry and all its properties.
         */
        void add_feature(const feature& feature);

    }; // class layer_builder

    /**
     * Parent class for the point_feature_builder, linestring_feature_builder
     * and polygon_feature_builder classes. You can not instantiate this class
     * directly, use it through its derived classes.
     */
    class feature_builder : public detail::feature_builder_base {

        class countdown_value {

            uint32_t m_value = 0;

        public:

            countdown_value() noexcept = default;

            ~countdown_value() noexcept {
                assert_is_zero();
            }

            countdown_value(const countdown_value&) = delete;

            countdown_value& operator=(const countdown_value&) = delete;

            countdown_value(countdown_value&& other) noexcept :
                m_value(other.m_value) {
                other.m_value = 0;
            }

            countdown_value& operator=(countdown_value&& other) noexcept {
                m_value = other.m_value;
                other.m_value = 0;
                return *this;
            }

            uint32_t value() const noexcept {
                return m_value;
            }

            void set(const uint32_t value) noexcept {
                m_value = value;
            }

            void decrement() {
                vtzero_assert(m_value > 0 && "too many calls to set_point()");
                --m_value;
            }

            void assert_is_zero() const noexcept {
                vtzero_assert_in_noexcept_function(m_value == 0 &&
                                                   "not enough calls to set_point()");
            }

        }; // countdown_value

    protected:

        /// Encoded geometry.
        protozero::packed_field_uint32 m_pbf_geometry{};

        /// Number of points still to be set for the geometry to be complete.
        countdown_value m_num_points;

        /// Last point (used to calculate delta between coordinates)
        point m_cursor{0, 0};

        /// Constructor.
        explicit feature_builder(detail::layer_builder_impl* layer) :
            feature_builder_base(layer) {
        }

        /// Helper function to check size isn't too large
        template <typename T>
        uint32_t check_num_points(T size) {
            if (size >= (1ul << 29u)) {
                throw geometry_exception{"Maximum of 2^29 - 1 points allowed in geometry"};
            }
            return static_cast<uint32_t>(size);
        }

        /// Helper function to make sure we have everything before adding a property
        void prepare_to_add_property() {
            if (m_pbf_geometry.valid()) {
                m_num_points.assert_is_zero();
                m_pbf_geometry.commit();
            }
            if (!m_pbf_tags.valid()) {
                m_pbf_tags = {m_feature_writer, detail::pbf_feature::tags};
            }
        }

    public:

        /**
         * If the feature was not committed, the destructor will roll back all
         * the changes.
         */
        ~feature_builder() {
            try {
                rollback();
            } catch (...) {
                // ignore exceptions
            }
        }

        /// Builder classes can not be copied
        feature_builder(const feature_builder&) = delete;

        /// Builder classes can not be copied
        feature_builder& operator=(const feature_builder&) = delete;

        /// Builder classes can be moved
        feature_builder(feature_builder&& other) noexcept = default;

        /// Builder classes can be moved
        feature_builder& operator=(feature_builder&& other) noexcept = default;

        /**
         * Set the ID of this feature.
         *
         * You can only call this method once and it must be before calling
         * any other method manipulating the geometry.
         *
         * @param id The ID.
         */
        void set_id(uint64_t id) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call set_id() after commit() or rollback()");
            vtzero_assert(!m_pbf_geometry.valid() &&
                          !m_pbf_tags.valid() &&
                          "Call set_id() before setting the geometry or adding properties");
            m_feature_writer.add_uint64(detail::pbf_feature::id, id);
        }

        /**
         * Add a property to this feature. Can only be called after all the
         * methods manipulating the geometry.
         *
         * @tparam TProp Can be type index_value_pair or property.
         * @param prop The property to add.
         */
        template <typename TProp>
        void add_property(TProp&& prop) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call add_property() after commit() or rollback()");
            prepare_to_add_property();
            add_property_impl(std::forward<TProp>(prop));
        }

        /**
         * Add a property to this feature. Can only be called after all the
         * methods manipulating the geometry.
         *
         * @tparam TKey Can be type index_value or data_view or anything that
         *         converts to it.
         * @tparam TValue Can be type index_value or property_value or
         *         encoded_property or anything that converts to it.
         * @param key The key.
         * @param value The value.
         */
        template <typename TKey, typename TValue>
        void add_property(TKey&& key, TValue&& value) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call add_property() after commit() or rollback()");
            prepare_to_add_property();
            add_property_impl(std::forward<TKey>(key), std::forward<TValue>(value));
        }

        /**
         * Commit this feature. Call this after all the details of this
         * feature have been added. If this is not called, the feature
         * will be rolled back when the destructor of the feature_builder is
         * called.
         *
         * Once a feature has been committed or rolled back, further calls
         * to commit() or rollback() don't do anything.
         */
        void commit() {
            if (m_feature_writer.valid()) {
                vtzero_assert((m_pbf_geometry.valid() || m_pbf_tags.valid()) &&
                              "Can not call commit before geometry was added");
                if (m_pbf_geometry.valid()) {
                    m_pbf_geometry.commit();
                }
                do_commit();
            }
        }

        /**
         * Rollback this feature. Removed all traces of this feature from
         * the layer_builder. Useful when you started creating a feature
         * but then find out that its geometry is invalid or something like
         * it. This will also happen automatically when the feature_builder
         * is destructed and commit() hasn't been called on it.
         *
         * Once a feature has been committed or rolled back, further calls
         * to commit() or rollback() don't do anything.
         */
        void rollback() {
            if (m_feature_writer.valid()) {
                if (m_pbf_geometry.valid()) {
                    m_pbf_geometry.rollback();
                }
                do_rollback();
            }
        }

    }; // class feature_builder

    /**
     * Used for adding a feature with a point geometry to a layer. After
     * creating an object of this class you can add data to the feature in a
     * specific order:
     *
     * * Optionally add the ID using set_id().
     * * Add the (multi)point geometry using add_point(), add_points() and
     *   set_point(), or add_points_from_container().
     * * Optionally add any number of properties using add_property().
     *
     * @code
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::point_feature_builder fb{lb};
     * fb.set_id(123); // optionally set ID
     * fb.add_point(10, 20) // add point geometry
     * fb.add_property("foo", "bar"); // add property
     * @endcode
     */
    class point_feature_builder : public feature_builder {

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         */
        explicit point_feature_builder(layer_builder layer) :
            feature_builder(&layer.get_layer_impl()) {
            m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(GeomType::POINT));
        }

        /**
         * Add a single point as the geometry to this feature.
         *
         * @param p The point to add.
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void add_point(const point p) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(!m_pbf_geometry.valid() &&
                          !m_pbf_tags.valid() &&
                          "add_point() can only be called once");
            m_pbf_geometry = {m_feature_writer, detail::pbf_feature::geometry};
            m_pbf_geometry.add_element(detail::command_move_to(1));
            m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x));
            m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y));
        }

        /**
         * Add a single point as the geometry to this feature.
         *
         * @param x X coordinate of the point to add.
         * @param y Y coordinate of the point to add.
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void add_point(const int32_t x, const int32_t y) {
            add_point(point{x, y});
        }

        /**
         * Add a single point as the geometry to this feature.
         *
         * @tparam TPoint A type that can be converted to vtzero::point using
         *         the create_vtzero_point function.
         * @param p The point to add.
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        template <typename TPoint>
        void add_point(TPoint&& p) {
            add_point(create_vtzero_point(std::forward<TPoint>(p)));
        }

        /**
         * Declare the intent to add a multipoint geometry with *count* points
         * to this feature.
         *
         * @param count The number of points in the multipoint geometry.
         *
         * @pre @code count > 0 && count < 2^29 @endcode
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void add_points(uint32_t count) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(!m_pbf_geometry.valid() &&
                          "can not call add_points() twice or mix with add_point()");
            vtzero_assert(!m_pbf_tags.valid() &&
                          "add_points() has to be called before properties are added");
            vtzero_assert(count > 0 && count < (1ul << 29u) && "add_points() must be called with 0 < count < 2^29");
            m_num_points.set(count);
            m_pbf_geometry = {m_feature_writer, detail::pbf_feature::geometry};
            m_pbf_geometry.add_element(detail::command_move_to(count));
        }

        /**
         * Set a point in the multipoint geometry.
         *
         * @param p The point.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_points(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void set_point(const point p) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(m_pbf_geometry.valid() &&
                          "call add_points() before set_point()");
            vtzero_assert(!m_pbf_tags.valid() &&
                          "set_point() has to be called before properties are added");
            m_num_points.decrement();
            m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - m_cursor.x));
            m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - m_cursor.y));
            m_cursor = p;
        }

        /**
         * Set a point in the multipoint geometry.
         *
         * @param x X coordinate of the point to set.
         * @param y Y coordinate of the point to set.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_points(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void set_point(const int32_t x, const int32_t y) {
            set_point(point{x, y});
        }

        /**
         * Set a point in the multipoint geometry.
         *
         * @tparam TPoint A type that can be converted to vtzero::point using
         *         the create_vtzero_point function.
         * @param p The point to add.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_points(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        template <typename TPoint>
        void set_point(TPoint&& p) {
            set_point(create_vtzero_point(std::forward<TPoint>(p)));
        }

        /**
         * Add the points from the specified container as multipoint geometry
         * to this feature.
         *
         * @tparam TContainer The container type. Must support the size()
         *         method, be iterable using a range for loop, and contain
         *         objects of type vtzero::point or something convertible to
         *         it.
         * @param container The container to read the points from.
         *
         * @throws geometry_exception If there are more than 2^32-1 members in
         *         the container.
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        template <typename TContainer>
        void add_points_from_container(const TContainer& container) {
            add_points(check_num_points(container.size()));
            for (const auto& element : container) {
                set_point(element);
            }
        }

    }; // class point_feature_builder

    /**
     * Used for adding a feature with a (multi)linestring geometry to a layer.
     * After creating an object of this class you can add data to the
     * feature in a specific order:
     *
     * * Optionally add the ID using set_id().
     * * Add the (multi)linestring geometry using add_linestring() or
     *   add_linestring_from_container().
     * * Optionally add any number of properties using add_property().
     *
     * @code
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::linestring_feature_builder fb{lb};
     * fb.set_id(123); // optionally set ID
     * fb.add_linestring(2);
     * fb.set_point(10, 10);
     * fb.set_point(10, 20);
     * fb.add_property("foo", "bar"); // add property
     * @endcode
     */
    class linestring_feature_builder : public feature_builder {

        bool m_start_line = false;

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         */
        explicit linestring_feature_builder(layer_builder layer) :
            feature_builder(&layer.get_layer_impl()) {
            m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(GeomType::LINESTRING));
        }

        /**
         * Declare the intent to add a linestring geometry with *count* points
         * to this feature.
         *
         * @param count The number of points in the linestring.
         *
         * @pre @code count > 1 && count < 2^29 @endcode
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void add_linestring(const uint32_t count) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(!m_pbf_tags.valid() &&
                          "add_linestring() has to be called before properties are added");
            vtzero_assert(count > 1 && count < (1ul << 29u) && "add_linestring() must be called with 1 < count < 2^29");
            m_num_points.assert_is_zero();
            if (!m_pbf_geometry.valid()) {
                m_pbf_geometry = {m_feature_writer, detail::pbf_feature::geometry};
            }
            m_num_points.set(count);
            m_start_line = true;
        }

        /**
         * Set a point in the multilinestring geometry opened with
         * add_linestring().
         *
         * @param p The point.
         *
         * @throws geometry_exception if the point set is the same as the
         *         previous point. This would create zero-length segments
         *         which are not allowed according to the vector tile spec.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_linestring(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void set_point(const point p) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(m_pbf_geometry.valid() &&
                          "call add_linestring() before set_point()");
            vtzero_assert(!m_pbf_tags.valid() &&
                          "set_point() has to be called before properties are added");
            m_num_points.decrement();
            if (m_start_line) {
                m_pbf_geometry.add_element(detail::command_move_to(1));
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - m_cursor.x));
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - m_cursor.y));
                m_pbf_geometry.add_element(detail::command_line_to(m_num_points.value()));
                m_start_line = false;
            } else {
                if (p == m_cursor) {
                    throw geometry_exception{"Zero-length segments in linestrings are not allowed."};
                }
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - m_cursor.x));
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - m_cursor.y));
            }
            m_cursor = p;
        }

        /**
         * Set a point in the multilinestring geometry opened with
         * add_linestring().
         *
         * @param x X coordinate of the point to set.
         * @param y Y coordinate of the point to set.
         *
         * @throws geometry_exception if the point set is the same as the
         *         previous point. This would create zero-length segments
         *         which are not allowed according to the vector tile spec.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_linestring(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void set_point(const int32_t x, const int32_t y) {
            set_point(point{x, y});
        }

        /**
         * Set a point in the multilinestring geometry opened with
         * add_linestring().
         *
         * @tparam TPoint A type that can be converted to vtzero::point using
         *         the create_vtzero_point function.
         * @param p The point to add.
         *
         * @throws geometry_exception if the point set is the same as the
         *         previous point. This would create zero-length segments
         *         which are not allowed according to the vector tile spec.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_linestring(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        template <typename TPoint>
        void set_point(TPoint&& p) {
            set_point(create_vtzero_point(std::forward<TPoint>(p)));
        }

        /**
         * Add the points from the specified container as a linestring geometry
         * to this feature.
         *
         * @tparam TContainer The container type. Must support the size()
         *         method, be iterable using a range for loop, and contain
         *         objects of type vtzero::point or something convertible to
         *         it.
         * @param container The container to read the points from.
         *
         * @throws geometry_exception If there are more than 2^32-1 members in
         *         the container or if two consecutive points in the container
         *         are identical.
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        template <typename TContainer>
        void add_linestring_from_container(const TContainer& container) {
            add_linestring(check_num_points(container.size()));
            for (const auto& element : container) {
                set_point(element);
            }
        }

    }; // class linestring_feature_builder

    /**
     * Used for adding a feature with a (multi)polygon geometry to a layer.
     * After creating an object of this class you can add data to the
     * feature in a specific order:
     *
     * * Optionally add the ID using set_id().
     * * Add the (multi)polygon geometry using add_ring() or
     *   add_ring_from_container().
     * * Optionally add any number of properties using add_property().
     *
     * @code
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::polygon_feature_builder fb{lb};
     * fb.set_id(123); // optionally set ID
     * fb.add_ring(5);
     * fb.set_point(10, 10);
     * ...
     * fb.add_property("foo", "bar"); // add property
     * @endcode
     */
    class polygon_feature_builder : public feature_builder {

        point m_first_point{0, 0};
        bool m_start_ring = false;

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         */
        explicit polygon_feature_builder(layer_builder layer) :
            feature_builder(&layer.get_layer_impl()) {
            m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(GeomType::POLYGON));
        }

        /**
         * Declare the intent to add a ring with *count* points to this
         * feature.
         *
         * @param count The number of points in the ring.
         *
         * @pre @code count > 3 && count < 2^29 @endcode
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void add_ring(const uint32_t count) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(!m_pbf_tags.valid() &&
                          "add_ring() has to be called before properties are added");
            vtzero_assert(count > 3 && count < (1ul << 29u) && "add_ring() must be called with 3 < count < 2^29");
            m_num_points.assert_is_zero();
            if (!m_pbf_geometry.valid()) {
                m_pbf_geometry = {m_feature_writer, detail::pbf_feature::geometry};
            }
            m_num_points.set(count);
            m_start_ring = true;
        }

        /**
         * Set a point in the ring opened with add_ring().
         *
         * @param p The point.
         *
         * @throws geometry_exception if the point set is the same as the
         *         previous point. This would create zero-length segments
         *         which are not allowed according to the vector tile spec.
         *         This exception is also thrown when the last point in the
         *         ring is not equal to the first point, because this would
         *         not create a closed ring.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_ring(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void set_point(const point p) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(m_pbf_geometry.valid() &&
                          "call add_ring() before set_point()");
            vtzero_assert(!m_pbf_tags.valid() &&
                          "set_point() has to be called before properties are added");
            m_num_points.decrement();
            if (m_start_ring) {
                m_first_point = p;
                m_pbf_geometry.add_element(detail::command_move_to(1));
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - m_cursor.x));
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - m_cursor.y));
                m_pbf_geometry.add_element(detail::command_line_to(m_num_points.value() - 1));
                m_start_ring = false;
                m_cursor = p;
            } else if (m_num_points.value() == 0) {
                if (p != m_first_point) {
                    throw geometry_exception{"Last point in a ring must be the same as the first point."};
                }
                // spec 4.3.3.3 "A ClosePath command MUST have a command count of 1"
                m_pbf_geometry.add_element(detail::command_close_path());
            } else {
                if (p == m_cursor) {
                    throw geometry_exception{"Zero-length segments in linestrings are not allowed."};
                }
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - m_cursor.x));
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - m_cursor.y));
                m_cursor = p;
            }
        }

        /**
         * Set a point in the ring opened with add_ring().
         *
         * @param x X coordinate of the point to set.
         * @param y Y coordinate of the point to set.
         *
         * @throws geometry_exception if the point set is the same as the
         *         previous point. This would create zero-length segments
         *         which are not allowed according to the vector tile spec.
         *         This exception is also thrown when the last point in the
         *         ring is not equal to the first point, because this would
         *         not create a closed ring.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_ring(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void set_point(const int32_t x, const int32_t y) {
            set_point(point{x, y});
        }

        /**
         * Set a point in the ring opened with add_ring().
         *
         * @tparam TPoint A type that can be converted to vtzero::point using
         *         the create_vtzero_point function.
         * @param p The point to add.
         *
         * @throws geometry_exception if the point set is the same as the
         *         previous point. This would create zero-length segments
         *         which are not allowed according to the vector tile spec.
         *         This exception is also thrown when the last point in the
         *         ring is not equal to the first point, because this would
         *         not create a closed ring.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_ring(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        template <typename TPoint>
        void set_point(TPoint&& p) {
            set_point(create_vtzero_point(std::forward<TPoint>(p)));
        }

        /**
         * Close a ring opened with add_ring(). This can be called for the
         * last point (which will be equal to the first point) in the ring
         * instead of calling set_point().
         *
         * @pre There must have been *count* - 1 calls to set_point()
         *      already after a call to add_ring(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void close_ring() {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(m_pbf_geometry.valid() &&
                          "Call add_ring() before you can call close_ring()");
            vtzero_assert(!m_pbf_tags.valid() &&
                          "close_ring() has to be called before properties are added");
            vtzero_assert(m_num_points.value() == 1 &&
                          "wrong number of points in ring");
            m_pbf_geometry.add_element(detail::command_close_path());
            m_num_points.decrement();
        }

        /**
         * Add the points from the specified container as a ring to this
         * feature.
         *
         * @tparam TContainer The container type. Must support the size()
         *         method, be iterable using a range for loop, and contain
         *         objects of type vtzero::point or something convertible to
         *         it.
         * @param container The container to read the points from.
         *
         * @throws geometry_exception If there are more than 2^32-1 members in
         *         the container or if two consecutive points in the container
         *         are identical or if the last point is not the same as the
         *         first point.
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        template <typename TContainer>
        void add_ring_from_container(const TContainer& container) {
            add_ring(check_num_points(container.size()));
            for (const auto& element : container) {
                set_point(element);
            }
        }

    }; // class polygon_feature_builder

    /**
     * Used for adding a feature to a layer using an existing geometry. After
     * creating an object of this class you can add data to the feature in a
     * specific order:
     *
     * * Optionally add the ID using set_id().
     * * Add the geometry using set_geometry().
     * * Optionally add any number of properties using add_property().
     *
     * @code
     * auto geom = ... // get geometry from a feature you are reading
     * ...
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::geometry_feature_builder fb{lb};
     * fb.set_id(123); // optionally set ID
     * fb.set_geometry(geom) // add geometry
     * fb.add_property("foo", "bar"); // add property
     * @endcode
     */
    class geometry_feature_builder : public detail::feature_builder_base {

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         */
        explicit geometry_feature_builder(layer_builder layer) :
            feature_builder_base(&layer.get_layer_impl()) {
        }

        /**
         * If the feature was not committed, the destructor will roll back all
         * the changes.
         */
        ~geometry_feature_builder() noexcept {
            try {
                rollback();
            } catch (...) {
                // ignore exceptions
            }
        }

        /// Feature builders can not be copied.
        geometry_feature_builder(const geometry_feature_builder&) = delete;

        /// Feature builders can not be copied.
        geometry_feature_builder& operator=(const geometry_feature_builder&) = delete;

        /// Feature builders can be moved.
        geometry_feature_builder(geometry_feature_builder&&) noexcept = default;

        /// Feature builders can be moved.
        geometry_feature_builder& operator=(geometry_feature_builder&&) noexcept = default;

        /**
         * Set the ID of this feature.
         *
         * You can only call this function once and it must be before calling
         * set_geometry().
         *
         * @param id The ID.
         */
        void set_id(uint64_t id) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call set_id() after commit() or rollback()");
            vtzero_assert(!m_pbf_tags.valid());
            m_feature_writer.add_uint64(detail::pbf_feature::id, id);
        }

        /**
         * Set the geometry of this feature.
         *
         * You can only call this method once and it must be before calling the
         * add_property() method.
         *
         * @param geometry The geometry.
         */
        void set_geometry(const geometry& geometry) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(!m_pbf_tags.valid());
            m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(geometry.type()));
            m_feature_writer.add_string(detail::pbf_feature::geometry, geometry.data());
            m_pbf_tags = {m_feature_writer, detail::pbf_feature::tags};
        }

        /**
         * Add a property to this feature. Can only be called after the
         * set_geometry method.
         *
         * @tparam TProp Can be type index_value_pair or property.
         * @param prop The property to add.
         */
        template <typename TProp>
        void add_property(TProp&& prop) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call add_property() after commit() or rollback()");
            add_property_impl(std::forward<TProp>(prop));
        }

        /**
         * Add a property to this feature. Can only be called after the
         * set_geometry method.
         *
         * @tparam TKey Can be type index_value or data_view or anything that
         *         converts to it.
         * @tparam TValue Can be type index_value or property_value or
         *         encoded_property or anything that converts to it.
         * @param key The key.
         * @param value The value.
         */
        template <typename TKey, typename TValue>
        void add_property(TKey&& key, TValue&& value) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call add_property() after commit() or rollback()");
            add_property_impl(std::forward<TKey>(key), std::forward<TValue>(value));
        }

        /**
         * Commit this feature. Call this after all the details of this
         * feature have been added. If this is not called, the feature
         * will be rolled back when the destructor of the feature_builder is
         * called.
         *
         * Once a feature has been committed or rolled back, further calls
         * to commit() or rollback() don't do anything.
         */
        void commit() {
            if (m_feature_writer.valid()) {
                vtzero_assert(m_pbf_tags.valid() &&
                              "Can not call commit before geometry was added");
                do_commit();
            }
        }

        /**
         * Rollback this feature. Removed all traces of this feature from
         * the layer_builder. Useful when you started creating a feature
         * but then find out that its geometry is invalid or something like
         * it. This will also happen automatically when the feature_builder
         * is destructed and commit() hasn't been called on it.
         *
         * Once a feature has been committed or rolled back, further calls
         * to commit() or rollback() don't do anything.
         */
        void rollback() {
            if (m_feature_writer.valid()) {
                do_rollback();
            }
        }

    }; // class geometry_feature_builder

    inline void layer_builder::add_feature(const feature& feature) {
        geometry_feature_builder feature_builder{*this};
        if (feature.has_id()) {
            feature_builder.set_id(feature.id());
        }
        feature_builder.set_geometry(feature.geometry());
        feature.for_each_property([&feature_builder](const property& p) {
            feature_builder.add_property(p);
            return true;
        });
    }

} // namespace vtzero

#endif // VTZERO_BUILDER_HPP

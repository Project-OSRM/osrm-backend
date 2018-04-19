#ifndef VTZERO_VECTOR_TILE_HPP
#define VTZERO_VECTOR_TILE_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file vector_tile.hpp
 *
 * @brief Contains the vector_tile class.
 */

#include "exception.hpp"
#include "layer.hpp"
#include "types.hpp"

#include <protozero/pbf_message.hpp>

#include <cstddef>
#include <cstring>
#include <iterator>
#include <string>

namespace vtzero {

    /**
     * A vector tile is basically nothing more than an ordered collection
     * of named layers. For the most efficient way to access the layers,
     * call next_layer() until it returns an invalid layer:
     *
     * @code
     *   std::string data = ...;
     *   vector_tile tile{data};
     *   while (auto layer = tile.next_layer()) {
     *     ...
     *   }
     * @endcode
     *
     * If you know the index of the layer, you can get it directly with
     * @code
     *   tile.get_layer(4);
     * @endcode
     *
     * You can also access the layer by name:
     * @code
     *   tile.get_layer_by_name("foobar");
     * @endcode
     */
    class vector_tile {

        data_view m_data;
        protozero::pbf_message<detail::pbf_tile> m_tile_reader;

    public:

        /**
         * Construct the vector_tile from a data_view. The vector_tile object
         * will keep a reference to the data referenced by the data_view. No
         * copy of the data is created.
         */
        explicit vector_tile(const data_view data) noexcept :
            m_data(data),
            m_tile_reader(m_data) {
        }

        /**
         * Construct the vector_tile from a string. The vector_tile object
         * will keep a reference to the data referenced by the string. No
         * copy of the data is created.
         */
        explicit vector_tile(const std::string& data) noexcept :
            m_data(data.data(), data.size()),
            m_tile_reader(m_data) {
        }

        /**
         * Construct the vector_tile from a ptr and size. The vector_tile
         * object will keep a reference to the data. No copy of the data is
         * created.
         */
        vector_tile(const char* data, std::size_t size) noexcept :
            m_data(data, size),
            m_tile_reader(m_data) {
        }

        /**
         * Is this vector tile empty?
         *
         * @returns true if there are no layers in this vector tile, false
         *          otherwise
         * Complexity: Constant.
         */
        bool empty() const noexcept {
            return m_data.empty();
        }

        /**
         * Return the number of layers in this tile.
         *
         * Complexity: Linear in the number of layers.
         *
         * @returns the number of layers in this tile
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        std::size_t count_layers() const {
            std::size_t size = 0;

            protozero::pbf_message<detail::pbf_tile> tile_reader{m_data};
            while (tile_reader.next(detail::pbf_tile::layers,
                                    protozero::pbf_wire_type::length_delimited)) {
                tile_reader.skip();
                ++size;
            }

            return size;
        }

        /**
         * Get the next layer in this tile.
         *
         * Complexity: Constant.
         *
         * @returns layer The next layer or the invalid layer if there are no
         *                more layers.
         * @throws format_exception if the tile data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        layer next_layer() {
            const bool has_next = m_tile_reader.next(detail::pbf_tile::layers,
                                                     protozero::pbf_wire_type::length_delimited);

            return has_next ? layer{m_tile_reader.get_view()} : layer{};
        }

        /**
         * Reset the layer iterator. The next time next_layer() is called,
         * it will begin from the first layer again.
         *
         * Complexity: Constant.
         */
        void reset_layer() noexcept {
            m_tile_reader = protozero::pbf_message<detail::pbf_tile>{m_data};
        }

        /**
         * Call a function for each layer in this tile.
         *
         * @tparam The type of the function. It must take a single argument
         *         of type layer&& and return a bool. If the function returns
         *         false, the iteration will be stopped.
         * @param func The function to call.
         * @returns true if the iteration was completed and false otherwise.
         */
        template <typename TFunc>
        bool for_each_layer(TFunc&& func) const {
            protozero::pbf_message<detail::pbf_tile> tile_reader{m_data};

            while (tile_reader.next(detail::pbf_tile::layers,
                                    protozero::pbf_wire_type::length_delimited)) {
                if (!std::forward<TFunc>(func)(layer{tile_reader.get_view()})) {
                    return false;
                }
            }

            return true;
        }

        /**
         * Returns the layer with the specified zero-based index.
         *
         * Complexity: Linear in the number of layers.
         *
         * @returns The specified layer or the invalid layer if index is
         *          larger than the number of layers.
         * @throws format_exception if the tile data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        layer get_layer(std::size_t index) const {
            protozero::pbf_message<detail::pbf_tile> tile_reader{m_data};

            while (tile_reader.next(detail::pbf_tile::layers,
                                    protozero::pbf_wire_type::length_delimited)) {
                if (index == 0) {
                    return layer{tile_reader.get_view()};
                }
                tile_reader.skip();
                --index;
            }

            return layer{};
        }

        /**
         * Returns the layer with the specified name.
         *
         * Complexity: Linear in the number of layers.
         *
         * If there are several layers with the same name (which is against
         * the spec 4.1 "A Vector Tile MUST NOT contain two or more layers
         * whose name values are byte-for-byte identical.") it is unspecified
         * which will be returned.
         *
         * @returns The specified layer or the invalid layer if there is no
         *          layer with this name.
         * @throws format_exception if the tile data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        layer get_layer_by_name(const data_view name) const {
            protozero::pbf_message<detail::pbf_tile> tile_reader{m_data};

            while (tile_reader.next(detail::pbf_tile::layers,
                                    protozero::pbf_wire_type::length_delimited)) {
                const auto layer_data = tile_reader.get_view();
                protozero::pbf_message<detail::pbf_layer> layer_reader{layer_data};
                if (layer_reader.next(detail::pbf_layer::name,
                                      protozero::pbf_wire_type::length_delimited)) {
                    if (layer_reader.get_view() == name) {
                        return layer{layer_data};
                    }
                } else {
                    // 4.1 "A layer MUST contain a name field."
                    throw format_exception{"missing name in layer (spec 4.1)"};
                }
            }

            return layer{};
        }

        /**
         * Returns the layer with the specified name.
         *
         * Complexity: Linear in the number of layers.
         *
         * If there are several layers with the same name (which is against
         * the spec 4.1 "A Vector Tile MUST NOT contain two or more layers
         * whose name values are byte-for-byte identical.") it is unspecified
         * which will be returned.
         *
         * @returns The specified layer or the invalid layer if there is no
         *          layer with this name.
         * @throws format_exception if the tile data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        layer get_layer_by_name(const std::string& name) const {
            return get_layer_by_name(data_view{name.data(), name.size()});
        }

        /**
         * Returns the layer with the specified name.
         *
         * Complexity: Linear in the number of layers.
         *
         * If there are several layers with the same name (which is against
         * the spec 4.1 "A Vector Tile MUST NOT contain two or more layers
         * whose name values are byte-for-byte identical.") it is unspecified
         * which will be returned.
         *
         * @returns The specified layer or the invalid layer if there is no
         *          layer with this name.
         * @throws format_exception if the tile data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        layer get_layer_by_name(const char* name) const {
            return get_layer_by_name(data_view{name, std::strlen(name)});
        }

    }; // class vector_tile

    /**
     * Helper function to determine whether some data could represent a
     * vector tile. This takes advantage of the fact that the first byte of
     * a vector tile is always 0x1a. It can't be 100% reliable though, because
     * some other data could still contain that byte.
     *
     * @returns false if this is definitely no vector tile
     *          true if this could be a vector tile
     */
    inline bool is_vector_tile(const data_view data) noexcept {
        return !data.empty() && data.data()[0] == 0x1a;
    }

} // namespace vtzero

#endif // VTZERO_VECTOR_TILE_HPP

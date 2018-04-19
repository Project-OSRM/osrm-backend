#ifndef VTZERO_GEOMETRY_HPP
#define VTZERO_GEOMETRY_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file geometry.hpp
 *
 * @brief Contains classes and functions related to geometry handling.
 */

#include "exception.hpp"
#include "types.hpp"

#include <protozero/pbf_reader.hpp>

#include <cstdint>
#include <limits>
#include <utility>

namespace vtzero {

    /// A simple point class
    struct point {

        /// X coordinate
        int32_t x = 0;

        /// Y coordinate
        int32_t y = 0;

        /// Default construct to 0 coordinates
        constexpr point() noexcept = default;

        /// Constructor
        constexpr point(int32_t x_, int32_t y_) noexcept :
            x(x_),
            y(y_) {
        }

    }; // struct point

    /**
     * Type of a polygon ring. This can either be "outer", "inner", or
     * "invalid". Invalid is used when the area of the ring is 0.
     */
    enum class ring_type {
        outer = 0,
        inner = 1,
        invalid = 2
    }; // enum class ring_type

    /**
     * Helper function to create a point from any type that has members x
     * and y.
     *
     * If your point type doesn't have members x any y, you can overload this
     * function for your type and it will be used by vtzero.
     */
    template <typename TPoint>
    point create_vtzero_point(const TPoint& p) noexcept {
        return {p.x, p.y};
    }

    /// Points are equal if their coordinates are
    inline constexpr bool operator==(const point a, const point b) noexcept {
        return a.x == b.x && a.y == b.y;
    }

    /// Points are not equal if their coordinates aren't
    inline constexpr bool operator!=(const point a, const point b) noexcept {
        return !(a == b);
    }

    namespace detail {

        /// The command id type as specified in the vector tile spec
        enum class CommandId : uint32_t {
            MOVE_TO = 1,
            LINE_TO = 2,
            CLOSE_PATH = 7
        };

        inline constexpr uint32_t command_integer(CommandId id, const uint32_t count) noexcept {
            return (static_cast<uint32_t>(id) & 0x7u) | (count << 3u);
        }

        inline constexpr uint32_t command_move_to(const uint32_t count) noexcept {
            return command_integer(CommandId::MOVE_TO, count);
        }

        inline constexpr uint32_t command_line_to(const uint32_t count) noexcept {
            return command_integer(CommandId::LINE_TO, count);
        }

        inline constexpr uint32_t command_close_path() noexcept {
            return command_integer(CommandId::CLOSE_PATH, 1);
        }

        inline constexpr uint32_t get_command_id(const uint32_t command_integer) noexcept {
            return command_integer & 0x7u;
        }

        inline constexpr uint32_t get_command_count(const uint32_t command_integer) noexcept {
            return command_integer >> 3u;
        }

        // The maximum value for the command count according to the spec.
        inline constexpr uint32_t max_command_count() noexcept {
            return get_command_count(std::numeric_limits<uint32_t>::max());
        }

        inline constexpr int64_t det(const point a, const point b) noexcept {
            return static_cast<int64_t>(a.x) * static_cast<int64_t>(b.y) -
                   static_cast<int64_t>(b.x) * static_cast<int64_t>(a.y);
        }

        template <typename T, typename Enable = void>
        struct get_result {

            using type = void;

            template <typename TGeomHandler>
            void operator()(TGeomHandler&& /*geom_handler*/) const noexcept {
            }

        };

        template <typename T>
        struct get_result<T, typename std::enable_if<!std::is_same<decltype(std::declval<T>().result()), void>::value>::type> {

            using type = decltype(std::declval<T>().result());

            template <typename TGeomHandler>
            type operator()(TGeomHandler&& geom_handler) {
                return std::forward<TGeomHandler>(geom_handler).result();
            }

        };

        /**
         * Decode a geometry as specified in spec 4.3 from a sequence of 32 bit
         * unsigned integers. This templated base class can be instantiated
         * with a different iterator type for testing than for normal use.
         */
        template <typename TIterator>
        class geometry_decoder {

        public:

            using iterator_type = TIterator;

        private:

            iterator_type m_it;
            iterator_type m_end;

            point m_cursor{0, 0};

            // maximum value for m_count before we throw an exception
            uint32_t m_max_count;

            /**
             * The current count value is set from the CommandInteger and
             * then counted down with each next_point() call. So it must be
             * greater than 0 when next_point() is called and 0 when
             * next_command() is called.
             */
            uint32_t m_count = 0;

        public:

            geometry_decoder(iterator_type begin, iterator_type end, std::size_t max) :
                m_it(begin),
                m_end(end),
                m_max_count(static_cast<uint32_t>(max)) {
                vtzero_assert(max <= detail::max_command_count());
            }

            uint32_t count() const noexcept {
                return m_count;
            }

            bool done() const noexcept {
                return m_it == m_end;
            }

            bool next_command(const CommandId expected_command_id) {
                vtzero_assert(m_count == 0);

                if (m_it == m_end) {
                    return false;
                }

                const auto command_id = get_command_id(*m_it);
                if (command_id != static_cast<uint32_t>(expected_command_id)) {
                    throw geometry_exception{std::string{"expected command "} +
                                             std::to_string(static_cast<uint32_t>(expected_command_id)) +
                                             " but got " +
                                             std::to_string(command_id)};
                }

                if (expected_command_id == CommandId::CLOSE_PATH) {
                    // spec 4.3.3.3 "A ClosePath command MUST have a command count of 1"
                    if (get_command_count(*m_it) != 1) {
                        throw geometry_exception{"ClosePath command count is not 1"};
                    }
                } else {
                    m_count = get_command_count(*m_it);
                    if (m_count > m_max_count) {
                        throw geometry_exception{"count too large"};
                    }
                }

                ++m_it;

                return true;
            }

            point next_point() {
                vtzero_assert(m_count > 0);

                if (m_it == m_end || std::next(m_it) == m_end) {
                    throw geometry_exception{"too few points in geometry"};
                }

                // spec 4.3.2 "A ParameterInteger is zigzag encoded"
                int64_t x = protozero::decode_zigzag32(*m_it++);
                int64_t y = protozero::decode_zigzag32(*m_it++);

                // x and y are int64_t so this addition can never overflow
                x += m_cursor.x;
                y += m_cursor.y;

                // The cast is okay, because a valid vector tile can never
                // contain values that would overflow here and we don't care
                // what happens to invalid tiles here.
                m_cursor.x = static_cast<int32_t>(x);
                m_cursor.y = static_cast<int32_t>(y);

                --m_count;

                return m_cursor;
            }

            template <typename TGeomHandler>
            typename detail::get_result<TGeomHandler>::type decode_point(TGeomHandler&& geom_handler) {
                // spec 4.3.4.2 "MUST consist of a single MoveTo command"
                if (!next_command(CommandId::MOVE_TO)) {
                    throw geometry_exception{"expected MoveTo command (spec 4.3.4.2)"};
                }

                // spec 4.3.4.2 "command count greater than 0"
                if (count() == 0) {
                    throw geometry_exception{"MoveTo command count is zero (spec 4.3.4.2)"};
                }

                std::forward<TGeomHandler>(geom_handler).points_begin(count());
                while (count() > 0) {
                    std::forward<TGeomHandler>(geom_handler).points_point(next_point());
                }

                // spec 4.3.4.2 "MUST consist of of a single ... command"
                if (!done()) {
                    throw geometry_exception{"additional data after end of geometry (spec 4.3.4.2)"};
                }

                std::forward<TGeomHandler>(geom_handler).points_end();

                return detail::get_result<TGeomHandler>{}(std::forward<TGeomHandler>(geom_handler));
            }

            template <typename TGeomHandler>
            typename detail::get_result<TGeomHandler>::type decode_linestring(TGeomHandler&& geom_handler) {
                // spec 4.3.4.3 "1. A MoveTo command"
                while (next_command(CommandId::MOVE_TO)) {
                    // spec 4.3.4.3 "with a command count of 1"
                    if (count() != 1) {
                        throw geometry_exception{"MoveTo command count is not 1 (spec 4.3.4.3)"};
                    }

                    const auto first_point = next_point();

                    // spec 4.3.4.3 "2. A LineTo command"
                    if (!next_command(CommandId::LINE_TO)) {
                        throw geometry_exception{"expected LineTo command (spec 4.3.4.3)"};
                    }

                    // spec 4.3.4.3 "with a command count greater than 0"
                    if (count() == 0) {
                        throw geometry_exception{"LineTo command count is zero (spec 4.3.4.3)"};
                    }

                    std::forward<TGeomHandler>(geom_handler).linestring_begin(count() + 1);

                    std::forward<TGeomHandler>(geom_handler).linestring_point(first_point);
                    while (count() > 0) {
                        std::forward<TGeomHandler>(geom_handler).linestring_point(next_point());
                    }

                    std::forward<TGeomHandler>(geom_handler).linestring_end();
                }

                return detail::get_result<TGeomHandler>{}(std::forward<TGeomHandler>(geom_handler));
            }

            template <typename TGeomHandler>
            typename detail::get_result<TGeomHandler>::type decode_polygon(TGeomHandler&& geom_handler) {
                // spec 4.3.4.4 "1. A MoveTo command"
                while (next_command(CommandId::MOVE_TO)) {
                    // spec 4.3.4.4 "with a command count of 1"
                    if (count() != 1) {
                        throw geometry_exception{"MoveTo command count is not 1 (spec 4.3.4.4)"};
                    }

                    int64_t sum = 0;
                    const point start_point = next_point();
                    point last_point = start_point;

                    // spec 4.3.4.4 "2. A LineTo command"
                    if (!next_command(CommandId::LINE_TO)) {
                        throw geometry_exception{"expected LineTo command (spec 4.3.4.4)"};
                    }

                    std::forward<TGeomHandler>(geom_handler).ring_begin(count() + 2);

                    std::forward<TGeomHandler>(geom_handler).ring_point(start_point);

                    while (count() > 0) {
                        const point p = next_point();
                        sum += detail::det(last_point, p);
                        last_point = p;
                        std::forward<TGeomHandler>(geom_handler).ring_point(p);
                    }

                    // spec 4.3.4.4 "3. A ClosePath command"
                    if (!next_command(CommandId::CLOSE_PATH)) {
                        throw geometry_exception{"expected ClosePath command (4.3.4.4)"};
                    }

                    sum += detail::det(last_point, start_point);

                    std::forward<TGeomHandler>(geom_handler).ring_point(start_point);

                    std::forward<TGeomHandler>(geom_handler).ring_end(sum > 0 ? ring_type::outer :
                                                                      sum < 0 ? ring_type::inner : ring_type::invalid);
                }

                return detail::get_result<TGeomHandler>{}(std::forward<TGeomHandler>(geom_handler));
            }

        }; // class geometry_decoder

    } // namespace detail

    /**
     * Decode a point geometry.
     *
     * @tparam TGeomHandler Handler class. See tutorial for details.
     * @param geometry The geometry as returned by feature.geometry().
     * @param geom_handler An object of TGeomHandler.
     * @throws geometry_error If there is a problem with the geometry.
     * @pre Geometry must be a point geometry.
     */
    template <typename TGeomHandler>
    typename detail::get_result<TGeomHandler>::type decode_point_geometry(const geometry& geometry, TGeomHandler&& geom_handler) {
        vtzero_assert(geometry.type() == GeomType::POINT);
        detail::geometry_decoder<decltype(geometry.begin())> decoder{geometry.begin(), geometry.end(), geometry.data().size() / 2};
        return decoder.decode_point(std::forward<TGeomHandler>(geom_handler));
    }

    /**
     * Decode a linestring geometry.
     *
     * @tparam TGeomHandler Handler class. See tutorial for details.
     * @param geometry The geometry as returned by feature.geometry().
     * @param geom_handler An object of TGeomHandler.
     * @returns whatever geom_handler.result() returns if that function exists,
     *          void otherwise
     * @throws geometry_error If there is a problem with the geometry.
     * @pre Geometry must be a linestring geometry.
     */
    template <typename TGeomHandler>
    typename detail::get_result<TGeomHandler>::type decode_linestring_geometry(const geometry& geometry, TGeomHandler&& geom_handler) {
        vtzero_assert(geometry.type() == GeomType::LINESTRING);
        detail::geometry_decoder<decltype(geometry.begin())> decoder{geometry.begin(), geometry.end(), geometry.data().size() / 2};
        return decoder.decode_linestring(std::forward<TGeomHandler>(geom_handler));
    }

    /**
     * Decode a polygon geometry.
     *
     * @tparam TGeomHandler Handler class. See tutorial for details.
     * @param geometry The geometry as returned by feature.geometry().
     * @param geom_handler An object of TGeomHandler.
     * @returns whatever geom_handler.result() returns if that function exists,
     *          void otherwise
     * @throws geometry_error If there is a problem with the geometry.
     * @pre Geometry must be a polygon geometry.
     */
    template <typename TGeomHandler>
    typename detail::get_result<TGeomHandler>::type decode_polygon_geometry(const geometry& geometry, TGeomHandler&& geom_handler) {
        vtzero_assert(geometry.type() == GeomType::POLYGON);
        detail::geometry_decoder<decltype(geometry.begin())> decoder{geometry.begin(), geometry.end(), geometry.data().size() / 2};
        return decoder.decode_polygon(std::forward<TGeomHandler>(geom_handler));
    }

    /**
     * Decode a geometry.
     *
     * @tparam TGeomHandler Handler class. See tutorial for details.
     * @param geometry The geometry as returned by feature.geometry().
     * @param geom_handler An object of TGeomHandler.
     * @returns whatever geom_handler.result() returns if that function exists,
     *          void otherwise
     * @throws geometry_error If the geometry has type UNKNOWN of if there is
     *                        a problem with the geometry.
     */
    template <typename TGeomHandler>
    typename detail::get_result<TGeomHandler>::type decode_geometry(const geometry& geometry, TGeomHandler&& geom_handler) {
        detail::geometry_decoder<decltype(geometry.begin())> decoder{geometry.begin(), geometry.end(), geometry.data().size() / 2};
        switch (geometry.type()) {
            case GeomType::POINT:
                return decoder.decode_point(std::forward<TGeomHandler>(geom_handler));
            case GeomType::LINESTRING:
                return decoder.decode_linestring(std::forward<TGeomHandler>(geom_handler));
            case GeomType::POLYGON:
                return decoder.decode_polygon(std::forward<TGeomHandler>(geom_handler));
            default:
                break;
        }
        throw geometry_exception{"unknown geometry type"};
    }

} // namespace vtzero

#endif // VTZERO_GEOMETRY_HPP

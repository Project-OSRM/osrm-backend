#ifndef PROTOZERO_PBF_BUILDER_HPP
#define PROTOZERO_PBF_BUILDER_HPP

/*****************************************************************************

protozero - Minimalistic protocol buffer decoder and encoder in C++.

This file is from https://github.com/mapbox/protozero where you can find more
documentation.

*****************************************************************************/

/**
 * @file pbf_builder.hpp
 *
 * @brief Contains the pbf_builder template class.
 */

#include <type_traits>

#include <protozero/pbf_writer.hpp>
#include <protozero/types.hpp>

namespace protozero {

/**
 * The pbf_builder is used to write PBF formatted messages into a buffer. It
 * is based on the pbf_writer class and has all the same methods. The
 * difference is that while the pbf_writer class takes an integer tag,
 * this template class takes a tag of the template type T. The idea is that
 * T will be an enumeration value and this helps reduce the possibility of
 * programming errors.
 *
 * Almost all methods in this class can throw an std::bad_alloc exception if
 * the std::string used as a buffer wants to resize.
 *
 * Read the tutorial to understand how this class is used.
 */
template <typename T>
class pbf_builder : public pbf_writer {

    static_assert(std::is_same<pbf_tag_type, typename std::underlying_type<T>::type>::value,
                  "T must be enum with underlying type protozero::pbf_tag_type");

public:

    using enum_type = T;

    explicit pbf_builder(std::string& data) noexcept :
        pbf_writer(data) {
    }

    template <typename P>
    pbf_builder(pbf_writer& parent_writer, P tag) noexcept :
        pbf_writer(parent_writer, pbf_tag_type(tag)) {
    }

/// @cond INTERNAL
#define PROTOZERO_WRITER_WRAP_ADD_SCALAR(name, type) \
    void add_##name(T tag, type value) { \
        pbf_writer::add_##name(pbf_tag_type(tag), value); \
    }

    PROTOZERO_WRITER_WRAP_ADD_SCALAR(bool, bool)
    PROTOZERO_WRITER_WRAP_ADD_SCALAR(enum, int32_t)
    PROTOZERO_WRITER_WRAP_ADD_SCALAR(int32, int32_t)
    PROTOZERO_WRITER_WRAP_ADD_SCALAR(sint32, int32_t)
    PROTOZERO_WRITER_WRAP_ADD_SCALAR(uint32, uint32_t)
    PROTOZERO_WRITER_WRAP_ADD_SCALAR(int64, int64_t)
    PROTOZERO_WRITER_WRAP_ADD_SCALAR(sint64, int64_t)
    PROTOZERO_WRITER_WRAP_ADD_SCALAR(uint64, uint64_t)
    PROTOZERO_WRITER_WRAP_ADD_SCALAR(fixed32, uint32_t)
    PROTOZERO_WRITER_WRAP_ADD_SCALAR(sfixed32, int32_t)
    PROTOZERO_WRITER_WRAP_ADD_SCALAR(fixed64, uint64_t)
    PROTOZERO_WRITER_WRAP_ADD_SCALAR(sfixed64, int64_t)
    PROTOZERO_WRITER_WRAP_ADD_SCALAR(float, float)
    PROTOZERO_WRITER_WRAP_ADD_SCALAR(double, double)

#undef PROTOZERO_WRITER_WRAP_ADD_SCALAR
/// @endcond

    void add_bytes(T tag, const char* value, std::size_t size) {
        pbf_writer::add_bytes(pbf_tag_type(tag), value, size);
    }

    void add_bytes(T tag, const data_view& value) {
        pbf_writer::add_bytes(pbf_tag_type(tag), value);
    }

    void add_bytes(T tag, const std::string& value) {
        pbf_writer::add_bytes(pbf_tag_type(tag), value);
    }

    void add_bytes(T tag, const char* value) {
        pbf_writer::add_bytes(pbf_tag_type(tag), value);
    }

    template <typename... Ts>
    void add_bytes_vectored(T tag, Ts&&... values) {
        pbf_writer::add_bytes_vectored(pbf_tag_type(tag), std::forward<Ts>(values)...);
    }

    void add_string(T tag, const char* value, std::size_t size) {
        pbf_writer::add_string(pbf_tag_type(tag), value, size);
    }

    void add_string(T tag, const data_view& value) {
        pbf_writer::add_string(pbf_tag_type(tag), value);
    }

    void add_string(T tag, const std::string& value) {
        pbf_writer::add_string(pbf_tag_type(tag), value);
    }

    void add_string(T tag, const char* value) {
        pbf_writer::add_string(pbf_tag_type(tag), value);
    }

    void add_message(T tag, const char* value, std::size_t size) {
        pbf_writer::add_message(pbf_tag_type(tag), value, size);
    }

    void add_message(T tag, const data_view& value) {
        pbf_writer::add_message(pbf_tag_type(tag), value);
    }

    void add_message(T tag, const std::string& value) {
        pbf_writer::add_message(pbf_tag_type(tag), value);
    }

/// @cond INTERNAL
#define PROTOZERO_WRITER_WRAP_ADD_PACKED(name) \
    template <typename InputIterator> \
    void add_packed_##name(T tag, InputIterator first, InputIterator last) { \
        pbf_writer::add_packed_##name(pbf_tag_type(tag), first, last); \
    }

    PROTOZERO_WRITER_WRAP_ADD_PACKED(bool)
    PROTOZERO_WRITER_WRAP_ADD_PACKED(enum)
    PROTOZERO_WRITER_WRAP_ADD_PACKED(int32)
    PROTOZERO_WRITER_WRAP_ADD_PACKED(sint32)
    PROTOZERO_WRITER_WRAP_ADD_PACKED(uint32)
    PROTOZERO_WRITER_WRAP_ADD_PACKED(int64)
    PROTOZERO_WRITER_WRAP_ADD_PACKED(sint64)
    PROTOZERO_WRITER_WRAP_ADD_PACKED(uint64)
    PROTOZERO_WRITER_WRAP_ADD_PACKED(fixed32)
    PROTOZERO_WRITER_WRAP_ADD_PACKED(sfixed32)
    PROTOZERO_WRITER_WRAP_ADD_PACKED(fixed64)
    PROTOZERO_WRITER_WRAP_ADD_PACKED(sfixed64)
    PROTOZERO_WRITER_WRAP_ADD_PACKED(float)
    PROTOZERO_WRITER_WRAP_ADD_PACKED(double)

#undef PROTOZERO_WRITER_WRAP_ADD_PACKED
/// @endcond

}; // class pbf_builder

} // end namespace protozero

#endif // PROTOZERO_PBF_BUILDER_HPP

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

#include <protozero/pbf_writer.hpp>
#include <protozero/types.hpp>

#include <type_traits>

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

    /// The type of messages this class will build.
    using enum_type = T;

    pbf_builder() = default;

    /**
     * Create a builder using the given string as a data store. The object
     * stores a reference to that string and adds all data to it. The string
     * doesn't have to be empty. The pbf_message object will just append data.
     */
    explicit pbf_builder(std::string& data) noexcept :
        pbf_writer(data) {
    }

    /**
     * Construct a pbf_builder for a submessage from the pbf_message or
     * pbf_writer of the parent message.
     *
     * @param parent_writer The parent pbf_message or pbf_writer
     * @param tag Tag of the field that will be written
     */
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

    /**
     * Add "bytes" field to data.
     *
     * @param tag Tag of the field
     * @param value Pointer to value to be written
     * @param size Number of bytes to be written
     */
    void add_bytes(T tag, const char* value, std::size_t size) {
        pbf_writer::add_bytes(pbf_tag_type(tag), value, size);
    }

    /**
     * Add "bytes" field to data.
     *
     * @param tag Tag of the field
     * @param value Value to be written
     */
    void add_bytes(T tag, const data_view& value) {
        pbf_writer::add_bytes(pbf_tag_type(tag), value);
    }

    /**
     * Add "bytes" field to data.
     *
     * @param tag Tag of the field
     * @param value Value to be written
     */
    void add_bytes(T tag, const std::string& value) {
        pbf_writer::add_bytes(pbf_tag_type(tag), value);
    }

    /**
     * Add "bytes" field to data. Bytes from the value are written until
     * a null byte is encountered. The null byte is not added.
     *
     * @param tag Tag of the field
     * @param value Pointer to zero-delimited value to be written
     */
    void add_bytes(T tag, const char* value) {
        pbf_writer::add_bytes(pbf_tag_type(tag), value);
    }

    /**
     * Add "bytes" field to data using vectored input. All the data in the
     * 2nd and further arguments is "concatenated" with only a single copy
     * into the final buffer.
     *
     * This will work with objects of any type supporting the data() and
     * size() methods like std::string or protozero::data_view.
     *
     * Example:
     * @code
     * std::string data1 = "abc";
     * std::string data2 = "xyz";
     * builder.add_bytes_vectored(1, data1, data2);
     * @endcode
     *
     * @tparam Ts List of types supporting data() and size() methods.
     * @param tag Tag of the field
     * @param values List of objects of types Ts with data to be appended.
     */
    template <typename... Ts>
    void add_bytes_vectored(T tag, Ts&&... values) {
        pbf_writer::add_bytes_vectored(pbf_tag_type(tag), std::forward<Ts>(values)...);
    }

    /**
     * Add "string" field to data.
     *
     * @param tag Tag of the field
     * @param value Pointer to value to be written
     * @param size Number of bytes to be written
     */
    void add_string(T tag, const char* value, std::size_t size) {
        pbf_writer::add_string(pbf_tag_type(tag), value, size);
    }

    /**
     * Add "string" field to data.
     *
     * @param tag Tag of the field
     * @param value Value to be written
     */
    void add_string(T tag, const data_view& value) {
        pbf_writer::add_string(pbf_tag_type(tag), value);
    }

    /**
     * Add "string" field to data.
     *
     * @param tag Tag of the field
     * @param value Value to be written
     */
    void add_string(T tag, const std::string& value) {
        pbf_writer::add_string(pbf_tag_type(tag), value);
    }

    /**
     * Add "string" field to data. Bytes from the value are written until
     * a null byte is encountered. The null byte is not added.
     *
     * @param tag Tag of the field
     * @param value Pointer to value to be written
     */
    void add_string(T tag, const char* value) {
        pbf_writer::add_string(pbf_tag_type(tag), value);
    }

    /**
     * Add "message" field to data.
     *
     * @param tag Tag of the field
     * @param value Pointer to message to be written
     * @param size Length of the message
     */
    void add_message(T tag, const char* value, std::size_t size) {
        pbf_writer::add_message(pbf_tag_type(tag), value, size);
    }

    /**
     * Add "message" field to data.
     *
     * @param tag Tag of the field
     * @param value Value to be written. The value must be a complete message.
     */
    void add_message(T tag, const data_view& value) {
        pbf_writer::add_message(pbf_tag_type(tag), value);
    }

    /**
     * Add "message" field to data.
     *
     * @param tag Tag of the field
     * @param value Value to be written. The value must be a complete message.
     */
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

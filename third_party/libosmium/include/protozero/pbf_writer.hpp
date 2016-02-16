#ifndef PROTOZERO_PBF_WRITER_HPP
#define PROTOZERO_PBF_WRITER_HPP

/*****************************************************************************

protozero - Minimalistic protocol buffer decoder and encoder in C++.

This file is from https://github.com/mapbox/protozero where you can find more
documentation.

*****************************************************************************/

/**
 * @file pbf_writer.hpp
 *
 * @brief Contains the pbf_writer class.
 */

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <string>

#include <protozero/config.hpp>
#include <protozero/pbf_types.hpp>
#include <protozero/varint.hpp>

#if PROTOZERO_BYTE_ORDER != PROTOZERO_LITTLE_ENDIAN
# include <protozero/byteswap.hpp>
#endif

namespace protozero {

namespace detail {

    template <typename T> class packed_field_varint;
    template <typename T> class packed_field_svarint;
    template <typename T> class packed_field_fixed;

} // end namespace detail

/**
 * The pbf_writer is used to write PBF formatted messages into a buffer.
 *
 * Almost all methods in this class can throw an std::bad_alloc exception if
 * the std::string used as a buffer wants to resize.
 */
class pbf_writer {

    // A pointer to a string buffer holding the data already written to the
    // PBF message. For default constructed writers or writers that have been
    // rolled back, this is a nullptr.
    std::string* m_data;

    // A pointer to a parent writer object if this is a submessage. If this
    // is a top-level writer, it is a nullptr.
    pbf_writer* m_parent_writer;

    // This is usually 0. If there is an open submessage, this is set in the
    // parent to the rollback position, ie. the last position before the
    // submessage was started. This is the position where the header of the
    // submessage starts.
    std::size_t m_rollback_pos = 0;

    // This is usually 0. If there is an open submessage, this is set in the
    // parent to the position where the data of the submessage is written to.
    std::size_t m_pos = 0;

    inline void add_varint(uint64_t value) {
        protozero_assert(m_pos == 0 && "you can't add fields to a parent pbf_writer if there is an existing pbf_writer for a submessage");
        protozero_assert(m_data);
        write_varint(std::back_inserter(*m_data), value);
    }

    inline void add_field(pbf_tag_type tag, pbf_wire_type type) {
        protozero_assert(((tag > 0 && tag < 19000) || (tag > 19999 && tag <= ((1 << 29) - 1))) && "tag out of range");
        uint32_t b = (tag << 3) | uint32_t(type);
        add_varint(b);
    }

    inline void add_tagged_varint(pbf_tag_type tag, uint64_t value) {
        add_field(tag, pbf_wire_type::varint);
        add_varint(value);
    }

    template <typename T>
    inline void add_fixed(T value) {
        protozero_assert(m_pos == 0 && "you can't add fields to a parent pbf_writer if there is an existing pbf_writer for a submessage");
        protozero_assert(m_data);
#if PROTOZERO_BYTE_ORDER == PROTOZERO_LITTLE_ENDIAN
        m_data->append(reinterpret_cast<const char*>(&value), sizeof(T));
#else
        auto size = m_data->size();
        m_data->resize(size + sizeof(T));
        byteswap<sizeof(T)>(reinterpret_cast<const char*>(&value), const_cast<char*>(m_data->data() + size));
#endif
    }

    template <typename T, typename It>
    inline void add_packed_fixed(pbf_tag_type tag, It first, It last, std::input_iterator_tag) {
        if (first == last) {
            return;
        }

        pbf_writer sw(*this, tag);

        while (first != last) {
            sw.add_fixed<T>(*first++);
        }
    }

    template <typename T, typename It>
    inline void add_packed_fixed(pbf_tag_type tag, It first, It last, std::forward_iterator_tag) {
        if (first == last) {
            return;
        }

        auto length = std::distance(first, last);
        add_length_varint(tag, sizeof(T) * pbf_length_type(length));
        reserve(sizeof(T) * std::size_t(length));

        while (first != last) {
            add_fixed<T>(*first++);
        }
    }

    template <typename It>
    inline void add_packed_varint(pbf_tag_type tag, It first, It last) {
        if (first == last) {
            return;
        }

        pbf_writer sw(*this, tag);

        while (first != last) {
            sw.add_varint(uint64_t(*first++));
        }
    }

    template <typename It>
    inline void add_packed_svarint(pbf_tag_type tag, It first, It last) {
        if (first == last) {
            return;
        }

        pbf_writer sw(*this, tag);

        while (first != last) {
            sw.add_varint(encode_zigzag64(*first++));
        }
    }

    // The number of bytes to reserve for the varint holding the length of
    // a length-delimited field. The length has to fit into pbf_length_type,
    // and a varint needs 8 bit for every 7 bit.
    static const int reserve_bytes = sizeof(pbf_length_type) * 8 / 7 + 1;

    // If m_rollpack_pos is set to this special value, it means that when
    // the submessage is closed, nothing needs to be done, because the length
    // of the submessage has already been written correctly.
    static const std::size_t size_is_known = std::numeric_limits<std::size_t>::max();

    inline void open_submessage(pbf_tag_type tag, std::size_t size) {
        protozero_assert(m_pos == 0);
        protozero_assert(m_data);
        if (size == 0) {
            m_rollback_pos = m_data->size();
            add_field(tag, pbf_wire_type::length_delimited);
            m_data->append(std::size_t(reserve_bytes), '\0');
        } else {
            m_rollback_pos = size_is_known;
            add_length_varint(tag, pbf_length_type(size));
            reserve(size);
        }
        m_pos = m_data->size();
    }

    inline void rollback_submessage() {
        protozero_assert(m_pos != 0);
        protozero_assert(m_rollback_pos != size_is_known);
        protozero_assert(m_data);
        m_data->resize(m_rollback_pos);
        m_pos = 0;
    }

    inline void commit_submessage() {
        protozero_assert(m_pos != 0);
        protozero_assert(m_rollback_pos != size_is_known);
        protozero_assert(m_data);
        auto length = pbf_length_type(m_data->size() - m_pos);

        protozero_assert(m_data->size() >= m_pos - reserve_bytes);
        auto n = write_varint(m_data->begin() + long(m_pos) - reserve_bytes, length);

        m_data->erase(m_data->begin() + long(m_pos) - reserve_bytes + n, m_data->begin() + long(m_pos));
        m_pos = 0;
    }

    inline void close_submessage() {
        protozero_assert(m_data);
        if (m_pos == 0 || m_rollback_pos == size_is_known) {
            return;
        }
        if (m_data->size() - m_pos == 0) {
            rollback_submessage();
        } else {
            commit_submessage();
        }
    }

    inline void add_length_varint(pbf_tag_type tag, pbf_length_type length) {
        add_field(tag, pbf_wire_type::length_delimited);
        add_varint(length);
    }

public:

    /**
     * Create a writer using the given string as a data store. The pbf_writer
     * stores a reference to that string and adds all data to it. The string
     * doesn't have to be empty. The pbf_writer will just append data.
     */
    inline explicit pbf_writer(std::string& data) noexcept :
        m_data(&data),
        m_parent_writer(nullptr),
        m_pos(0) {
    }

    /**
     * Create a writer without a data store. In this form the writer can not
     * be used!
     */
    inline pbf_writer() noexcept :
        m_data(nullptr),
        m_parent_writer(nullptr),
        m_pos(0) {
    }

    /**
     * Construct a pbf_writer for a submessage from the pbf_writer of the
     * parent message.
     *
     * @param parent_writer The pbf_writer
     * @param tag Tag (field number) of the field that will be written
     * @param size Optional size of the submessage in bytes (use 0 for unknown).
     *        Setting this allows some optimizations but is only possible in
     *        a few very specific cases.
     */
    inline pbf_writer(pbf_writer& parent_writer, pbf_tag_type tag, std::size_t size=0) :
        m_data(parent_writer.m_data),
        m_parent_writer(&parent_writer),
        m_pos(0) {
        m_parent_writer->open_submessage(tag, size);
    }

    /// A pbf_writer object can be copied
    pbf_writer(const pbf_writer&) noexcept = default;

    /// A pbf_writer object can be copied
    pbf_writer& operator=(const pbf_writer&) noexcept = default;

    /// A pbf_writer object can be moved
    inline pbf_writer(pbf_writer&&) noexcept = default;

    /// A pbf_writer object can be moved
    inline pbf_writer& operator=(pbf_writer&&) noexcept = default;

    inline ~pbf_writer() {
        if (m_parent_writer) {
            m_parent_writer->close_submessage();
        }
    }

    /**
     * Reserve size bytes in the underlying message store in addition to
     * whatever the message store already holds. So unlike
     * the `std::string::reserve()` method this is not an absolute size,
     * but additional memory that should be reserved.
     *
     * @param size Number of bytes to reserve in underlying message store.
     */
    void reserve(std::size_t size) {
        protozero_assert(m_data);
        m_data->reserve(m_data->size() + size);
    }

    inline void rollback() {
        protozero_assert(m_parent_writer && "you can't call rollback() on a pbf_writer without a parent");
        protozero_assert(m_pos == 0 && "you can't call rollback() on a pbf_writer that has an open nested submessage");
        m_parent_writer->rollback_submessage();
        m_data = nullptr;
    }

    ///@{
    /**
     * @name Scalar field writer functions
     */

    /**
     * Add "bool" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    inline void add_bool(pbf_tag_type tag, bool value) {
        add_field(tag, pbf_wire_type::varint);
        protozero_assert(m_pos == 0 && "you can't add fields to a parent pbf_writer if there is an existing pbf_writer for a submessage");
        protozero_assert(m_data);
        m_data->append(1, value);
    }

    /**
     * Add "enum" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    inline void add_enum(pbf_tag_type tag, int32_t value) {
        add_tagged_varint(tag, uint64_t(value));
    }

    /**
     * Add "int32" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    inline void add_int32(pbf_tag_type tag, int32_t value) {
        add_tagged_varint(tag, uint64_t(value));
    }

    /**
     * Add "sint32" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    inline void add_sint32(pbf_tag_type tag, int32_t value) {
        add_tagged_varint(tag, encode_zigzag32(value));
    }

    /**
     * Add "uint32" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    inline void add_uint32(pbf_tag_type tag, uint32_t value) {
        add_tagged_varint(tag, value);
    }

    /**
     * Add "int64" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    inline void add_int64(pbf_tag_type tag, int64_t value) {
        add_tagged_varint(tag, uint64_t(value));
    }

    /**
     * Add "sint64" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    inline void add_sint64(pbf_tag_type tag, int64_t value) {
        add_tagged_varint(tag, encode_zigzag64(value));
    }

    /**
     * Add "uint64" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    inline void add_uint64(pbf_tag_type tag, uint64_t value) {
        add_tagged_varint(tag, value);
    }

    /**
     * Add "fixed32" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    inline void add_fixed32(pbf_tag_type tag, uint32_t value) {
        add_field(tag, pbf_wire_type::fixed32);
        add_fixed<uint32_t>(value);
    }

    /**
     * Add "sfixed32" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    inline void add_sfixed32(pbf_tag_type tag, int32_t value) {
        add_field(tag, pbf_wire_type::fixed32);
        add_fixed<int32_t>(value);
    }

    /**
     * Add "fixed64" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    inline void add_fixed64(pbf_tag_type tag, uint64_t value) {
        add_field(tag, pbf_wire_type::fixed64);
        add_fixed<uint64_t>(value);
    }

    /**
     * Add "sfixed64" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    inline void add_sfixed64(pbf_tag_type tag, int64_t value) {
        add_field(tag, pbf_wire_type::fixed64);
        add_fixed<int64_t>(value);
    }

    /**
     * Add "float" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    inline void add_float(pbf_tag_type tag, float value) {
        add_field(tag, pbf_wire_type::fixed32);
        add_fixed<float>(value);
    }

    /**
     * Add "double" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    inline void add_double(pbf_tag_type tag, double value) {
        add_field(tag, pbf_wire_type::fixed64);
        add_fixed<double>(value);
    }

    /**
     * Add "bytes" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Pointer to value to be written
     * @param size Number of bytes to be written
     */
    inline void add_bytes(pbf_tag_type tag, const char* value, std::size_t size) {
        protozero_assert(m_pos == 0 && "you can't add fields to a parent pbf_writer if there is an existing pbf_writer for a submessage");
        protozero_assert(m_data);
        protozero_assert(size <= std::numeric_limits<pbf_length_type>::max());
        add_length_varint(tag, pbf_length_type(size));
        m_data->append(value, size);
    }

    /**
     * Add "bytes" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    inline void add_bytes(pbf_tag_type tag, const std::string& value) {
        add_bytes(tag, value.data(), value.size());
    }

    /**
     * Add "string" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Pointer to value to be written
     * @param size Number of bytes to be written
     */
    inline void add_string(pbf_tag_type tag, const char* value, std::size_t size) {
        add_bytes(tag, value, size);
    }

    /**
     * Add "string" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    inline void add_string(pbf_tag_type tag, const std::string& value) {
        add_bytes(tag, value.data(), value.size());
    }

    /**
     * Add "string" field to data. Bytes from the value are written until
     * a null byte is encountered. The null byte is not added.
     *
     * @param tag Tag (field number) of the field
     * @param value Pointer to value to be written
     */
    inline void add_string(pbf_tag_type tag, const char* value) {
        add_bytes(tag, value, std::strlen(value));
    }

    /**
     * Add "message" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Pointer to message to be written
     * @param size Length of the message
     */
    inline void add_message(pbf_tag_type tag, const char* value, std::size_t size) {
        add_bytes(tag, value, size);
    }

    /**
     * Add "message" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written. The value must be a complete message.
     */
    inline void add_message(pbf_tag_type tag, const std::string& value) {
        add_bytes(tag, value.data(), value.size());
    }

    ///@}

    ///@{
    /**
     * @name Repeated packed field writer functions
     */

    /**
     * Add "repeated packed bool" field to data.
     *
     * @tparam InputIterator An type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to bool.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    inline void add_packed_bool(pbf_tag_type tag, InputIterator first, InputIterator last) {
        add_packed_varint(tag, first, last);
    }

    /**
     * Add "repeated packed enum" field to data.
     *
     * @tparam InputIterator An type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to int32_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    inline void add_packed_enum(pbf_tag_type tag, InputIterator first, InputIterator last) {
        add_packed_varint(tag, first, last);
    }

    /**
     * Add "repeated packed int32" field to data.
     *
     * @tparam InputIterator An type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to int32_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    inline void add_packed_int32(pbf_tag_type tag, InputIterator first, InputIterator last) {
        add_packed_varint(tag, first, last);
    }

    /**
     * Add "repeated packed sint32" field to data.
     *
     * @tparam InputIterator An type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to int32_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    inline void add_packed_sint32(pbf_tag_type tag, InputIterator first, InputIterator last) {
        add_packed_svarint(tag, first, last);
    }

    /**
     * Add "repeated packed uint32" field to data.
     *
     * @tparam InputIterator An type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to uint32_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    inline void add_packed_uint32(pbf_tag_type tag, InputIterator first, InputIterator last) {
        add_packed_varint(tag, first, last);
    }

    /**
     * Add "repeated packed int64" field to data.
     *
     * @tparam InputIterator An type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to int64_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    inline void add_packed_int64(pbf_tag_type tag, InputIterator first, InputIterator last) {
        add_packed_varint(tag, first, last);
    }

    /**
     * Add "repeated packed sint64" field to data.
     *
     * @tparam InputIterator An type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to int64_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    inline void add_packed_sint64(pbf_tag_type tag, InputIterator first, InputIterator last) {
        add_packed_svarint(tag, first, last);
    }

    /**
     * Add "repeated packed uint64" field to data.
     *
     * @tparam InputIterator An type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to uint64_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    inline void add_packed_uint64(pbf_tag_type tag, InputIterator first, InputIterator last) {
        add_packed_varint(tag, first, last);
    }

    /**
     * Add "repeated packed fixed32" field to data.
     *
     * @tparam InputIterator An type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to uint32_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    inline void add_packed_fixed32(pbf_tag_type tag, InputIterator first, InputIterator last) {
        add_packed_fixed<uint32_t, InputIterator>(tag, first, last,
            typename std::iterator_traits<InputIterator>::iterator_category());
    }

    /**
     * Add "repeated packed sfixed32" field to data.
     *
     * @tparam InputIterator An type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to int32_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    inline void add_packed_sfixed32(pbf_tag_type tag, InputIterator first, InputIterator last) {
        add_packed_fixed<int32_t, InputIterator>(tag, first, last,
            typename std::iterator_traits<InputIterator>::iterator_category());
    }

    /**
     * Add "repeated packed fixed64" field to data.
     *
     * @tparam InputIterator An type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to uint64_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    inline void add_packed_fixed64(pbf_tag_type tag, InputIterator first, InputIterator last) {
        add_packed_fixed<uint64_t, InputIterator>(tag, first, last,
            typename std::iterator_traits<InputIterator>::iterator_category());
    }

    /**
     * Add "repeated packed sfixed64" field to data.
     *
     * @tparam InputIterator An type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to int64_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    inline void add_packed_sfixed64(pbf_tag_type tag, InputIterator first, InputIterator last) {
        add_packed_fixed<int64_t, InputIterator>(tag, first, last,
            typename std::iterator_traits<InputIterator>::iterator_category());
    }

    /**
     * Add "repeated packed float" field to data.
     *
     * @tparam InputIterator An type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to float.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    inline void add_packed_float(pbf_tag_type tag, InputIterator first, InputIterator last) {
        add_packed_fixed<float, InputIterator>(tag, first, last,
            typename std::iterator_traits<InputIterator>::iterator_category());
    }

    /**
     * Add "repeated packed double" field to data.
     *
     * @tparam InputIterator An type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to double.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    inline void add_packed_double(pbf_tag_type tag, InputIterator first, InputIterator last) {
        add_packed_fixed<double, InputIterator>(tag, first, last,
            typename std::iterator_traits<InputIterator>::iterator_category());
    }

    ///@}

    template <typename T> friend class detail::packed_field_varint;
    template <typename T> friend class detail::packed_field_svarint;
    template <typename T> friend class detail::packed_field_fixed;

}; // class pbf_writer

namespace detail {

    class packed_field {

    protected:

        pbf_writer m_writer;

    public:

        packed_field(pbf_writer& parent_writer, pbf_tag_type tag) :
            m_writer(parent_writer, tag) {
        }

        packed_field(pbf_writer& parent_writer, pbf_tag_type tag, std::size_t size) :
            m_writer(parent_writer, tag, size) {
        }

        void rollback() {
            m_writer.rollback();
        }

    }; // class packed_field

    template <typename T>
    class packed_field_fixed : public packed_field {

    public:

        packed_field_fixed(pbf_writer& parent_writer, pbf_tag_type tag) :
            packed_field(parent_writer, tag) {
        }

        packed_field_fixed(pbf_writer& parent_writer, pbf_tag_type tag, std::size_t size) :
            packed_field(parent_writer, tag, size * sizeof(T)) {
        }

        void add_element(T value) {
            m_writer.add_fixed<T>(value);
        }

    }; // class packed_field_fixed

    template <typename T>
    class packed_field_varint : public packed_field {

    public:

        packed_field_varint(pbf_writer& parent_writer, pbf_tag_type tag) :
            packed_field(parent_writer, tag) {
        }

        void add_element(T value) {
            m_writer.add_varint(uint64_t(value));
        }

    }; // class packed_field_varint

    template <typename T>
    class packed_field_svarint : public packed_field {

    public:

        packed_field_svarint(pbf_writer& parent_writer, pbf_tag_type tag) :
            packed_field(parent_writer, tag) {
        }

        void add_element(T value) {
            m_writer.add_varint(encode_zigzag64(value));
        }

    }; // class packed_field_svarint

} // end namespace detail

using packed_field_bool     = detail::packed_field_varint<bool>;
using packed_field_enum     = detail::packed_field_varint<int32_t>;
using packed_field_int32    = detail::packed_field_varint<int32_t>;
using packed_field_sint32   = detail::packed_field_svarint<int32_t>;
using packed_field_uint32   = detail::packed_field_varint<uint32_t>;
using packed_field_int64    = detail::packed_field_varint<int64_t>;
using packed_field_sint64   = detail::packed_field_svarint<int64_t>;
using packed_field_uint64   = detail::packed_field_varint<uint64_t>;
using packed_field_fixed32  = detail::packed_field_fixed<uint32_t>;
using packed_field_sfixed32 = detail::packed_field_fixed<int32_t>;
using packed_field_fixed64  = detail::packed_field_fixed<uint64_t>;
using packed_field_sfixed64 = detail::packed_field_fixed<int64_t>;
using packed_field_float    = detail::packed_field_fixed<float>;
using packed_field_double   = detail::packed_field_fixed<double>;

} // end namespace protozero

#endif // PROTOZERO_PBF_WRITER_HPP

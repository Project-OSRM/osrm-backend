#ifndef PROTOZERO_PBF_READER_HPP
#define PROTOZERO_PBF_READER_HPP

/*****************************************************************************

protozero - Minimalistic protocol buffer decoder and encoder in C++.

This file is from https://github.com/mapbox/protozero where you can find more
documentation.

*****************************************************************************/

/**
 * @file pbf_reader.hpp
 *
 * @brief Contains the pbf_reader class.
 */

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <string>
#include <utility>

#include <protozero/config.hpp>
#include <protozero/exception.hpp>
#include <protozero/pbf_types.hpp>
#include <protozero/varint.hpp>

#if PROTOZERO_BYTE_ORDER != PROTOZERO_LITTLE_ENDIAN
# include <protozero/byteswap.hpp>
#endif

namespace protozero {

/**
 * This class represents a protobuf message. Either a top-level message or
 * a nested sub-message. Top-level messages can be created from any buffer
 * with a pointer and length:
 *
 * @code
 *    std::string buffer;
 *    // fill buffer...
 *    pbf_reader message(buffer.data(), buffer.size());
 * @endcode
 *
 * Sub-messages are created using get_message():
 *
 * @code
 *    pbf_reader message(...);
 *    message.next();
 *    pbf_reader submessage = message.get_message();
 * @endcode
 *
 * All methods of the pbf_reader class except get_bytes() and get_string()
 * provide the strong exception guarantee, ie they either succeed or do not
 * change the pbf_reader object they are called on. Use the get_data() method
 * instead of get_bytes() or get_string(), if you need this guarantee.
 */
class pbf_reader {

    // A pointer to the next unread data.
    const char *m_data = nullptr;

    // A pointer to one past the end of data.
    const char *m_end = nullptr;

    // The wire type of the current field.
    pbf_wire_type m_wire_type = pbf_wire_type::unknown;

    // The tag of the current field.
    pbf_tag_type m_tag = 0;

    // Copy N bytes from src to dest on little endian machines, on big endian
    // swap the bytes in the process.
    template <int N>
    static void copy_or_byteswap(const char* src, void* dest) noexcept {
#if PROTOZERO_BYTE_ORDER == PROTOZERO_LITTLE_ENDIAN
        memcpy(dest, src, N);
#else
        byteswap<N>(src, reinterpret_cast<char*>(dest));
#endif
    }

    template <typename T>
    inline T get_fixed() {
        T result;
        skip_bytes(sizeof(T));
        copy_or_byteswap<sizeof(T)>(m_data - sizeof(T), &result);
        return result;
    }

#ifdef PROTOZERO_USE_BARE_POINTER_FOR_PACKED_FIXED

    template <typename T>
    inline std::pair<const T*, const T*> packed_fixed() {
        protozero_assert(tag() != 0 && "call next() before accessing field value");
        auto len = get_len_and_skip();
        protozero_assert(len % sizeof(T) == 0);
        return std::make_pair(reinterpret_cast<const T*>(m_data-len), reinterpret_cast<const T*>(m_data));
    }

#else

    template <typename T>
    class const_fixed_iterator : public std::iterator<std::forward_iterator_tag, T> {

        const char* m_data;
        const char* m_end;

    public:

        const_fixed_iterator() noexcept :
            m_data(nullptr),
            m_end(nullptr) {
        }

        const_fixed_iterator(const char *data, const char* end) noexcept :
            m_data(data),
            m_end(end) {
        }

        const_fixed_iterator(const const_fixed_iterator&) noexcept = default;
        const_fixed_iterator(const_fixed_iterator&&) noexcept = default;

        const_fixed_iterator& operator=(const const_fixed_iterator&) noexcept = default;
        const_fixed_iterator& operator=(const_fixed_iterator&&) noexcept = default;

        ~const_fixed_iterator() noexcept = default;

        T operator*() {
            T result;
            copy_or_byteswap<sizeof(T)>(m_data , &result);
            return result;
        }

        const_fixed_iterator& operator++() {
            m_data += sizeof(T);
            return *this;
        }

        const_fixed_iterator operator++(int) {
            const const_fixed_iterator tmp(*this);
            ++(*this);
            return tmp;
        }

        bool operator==(const const_fixed_iterator& rhs) const noexcept {
            return m_data == rhs.m_data && m_end == rhs.m_end;
        }

        bool operator!=(const const_fixed_iterator& rhs) const noexcept {
            return !(*this == rhs);
        }

    }; // class const_fixed_iterator

    template <typename T>
    inline std::pair<const_fixed_iterator<T>, const_fixed_iterator<T>> packed_fixed() {
        protozero_assert(tag() != 0 && "call next() before accessing field value");
        auto len = get_len_and_skip();
        protozero_assert(len % sizeof(T) == 0);
        return std::make_pair(const_fixed_iterator<T>(m_data-len, m_data),
                              const_fixed_iterator<T>(m_data, m_data));
    }

#endif

    template <typename T> inline T get_varint();
    template <typename T> inline T get_svarint();

    inline pbf_length_type get_length() { return get_varint<pbf_length_type>(); }

    inline void skip_bytes(pbf_length_type len);

    inline pbf_length_type get_len_and_skip();

public:

    /**
     * Construct a pbf_reader message from a data pointer and a length. The pointer
     * will be stored inside the pbf_reader object, no data is copied. So you must
     * make sure the buffer stays valid as long as the pbf_reader object is used.
     *
     * The buffer must contain a complete protobuf message.
     *
     * @post There is no current field.
     */
    inline pbf_reader(const char *data, size_t length) noexcept;

    /**
     * Construct a pbf_reader message from a data pointer and a length. The pointer
     * will be stored inside the pbf_reader object, no data is copied. So you must
     * make sure the buffer stays valid as long as the pbf_reader object is used.
     *
     * The buffer must contain a complete protobuf message.
     *
     * @post There is no current field.
     */
    inline pbf_reader(std::pair<const char *, size_t> data) noexcept;

    /**
     * Construct a pbf_reader message from a std::string. A pointer to the string
     * internals will be stored inside the pbf_reader object, no data is copied.
     * So you must make sure the string is unchanged as long as the pbf_reader
     * object is used.
     *
     * The string must contain a complete protobuf message.
     *
     * @post There is no current field.
     */
    inline pbf_reader(const std::string& data) noexcept;

    /**
     * pbf_reader can be default constructed and behaves like it has an empty
     * buffer.
     */
    inline pbf_reader() noexcept = default;

    /// pbf_reader messages can be copied trivially.
    inline pbf_reader(const pbf_reader&) noexcept = default;

    /// pbf_reader messages can be moved trivially.
    inline pbf_reader(pbf_reader&&) noexcept = default;

    /// pbf_reader messages can be copied trivially.
    inline pbf_reader& operator=(const pbf_reader& other) noexcept = default;

    /// pbf_reader messages can be moved trivially.
    inline pbf_reader& operator=(pbf_reader&& other) noexcept = default;

    inline ~pbf_reader() = default;

    /**
     * In a boolean context the pbf_reader class evaluates to `true` if there are
     * still fields available and to `false` if the last field has been read.
     */
    inline operator bool() const noexcept;

    /**
     * Return the length in bytes of the current message. If you have
     * already called next() and/or any of the get_*() functions, this will
     * return the remaining length.
     *
     * This can, for instance, be used to estimate the space needed for a
     * buffer. Of course you have to know reasonably well what data to expect
     * and how it is encoded for this number to have any meaning.
     */
    size_t length() const noexcept {
        return size_t(m_end - m_data);
    }

    /**
     * Set next field in the message as the current field. This is usually
     * called in a while loop:
     *
     * @code
     *    pbf_reader message(...);
     *    while (message.next()) {
     *        // handle field
     *    }
     * @endcode
     *
     * @returns `true` if there is a next field, `false` if not.
     * @pre There must be no current field.
     * @post If it returns `true` there is a current field now.
     */
    inline bool next();

    /**
     * Set next field with given tag in the message as the current field.
     * Fields with other tags are skipped. This is usually called in a while
     * loop for repeated fields:
     *
     * @code
     *    pbf_reader message(...);
     *    while (message.next(17)) {
     *        // handle field
     *    }
     * @endcode
     *
     * or you can call it just once to get the one field with this tag:
     *
     * @code
     *    pbf_reader message(...);
     *    if (message.next(17)) {
     *        // handle field
     *    }
     * @endcode
     *
     * @returns `true` if there is a next field with this tag.
     * @pre There must be no current field.
     * @post If it returns `true` there is a current field now with the given tag.
     */
    inline bool next(pbf_tag_type tag);

    /**
     * The tag of the current field. The tag is the field number from the
     * description in the .proto file.
     *
     * Call next() before calling this function to set the current field.
     *
     * @returns tag of the current field.
     * @pre There must be a current field (ie. next() must have returned `true`).
     */
    inline pbf_tag_type tag() const noexcept;

    /**
     * Get the wire type of the current field. The wire types are:
     *
     * * 0 - varint
     * * 1 - 64 bit
     * * 2 - length-delimited
     * * 5 - 32 bit
     *
     * All other types are illegal.
     *
     * Call next() before calling this function to set the current field.
     *
     * @returns wire type of the current field.
     * @pre There must be a current field (ie. next() must have returned `true`).
     */
    inline pbf_wire_type wire_type() const noexcept;

    /**
     * Check the wire type of the current field.
     *
     * @returns `true` if the current field has the given wire type.
     * @pre There must be a current field (ie. next() must have returned `true`).
     */
    inline bool has_wire_type(pbf_wire_type type) const noexcept;

    /**
     * Consume the current field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @post The current field was consumed and there is no current field now.
     */
    inline void skip();

    ///@{
    /**
     * @name Scalar field accessor functions
     */

    /**
     * Consume and return value of current "bool" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "bool".
     * @post The current field was consumed and there is no current field now.
     */
    inline bool get_bool();

    /**
     * Consume and return value of current "enum" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "enum".
     * @post The current field was consumed and there is no current field now.
     */
    inline int32_t get_enum() {
        protozero_assert(has_wire_type(pbf_wire_type::varint) && "not a varint");
        return get_varint<int32_t>();
    }

    /**
     * Consume and return value of current "int32" varint field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "int32".
     * @post The current field was consumed and there is no current field now.
     */
    inline int32_t get_int32() {
        protozero_assert(has_wire_type(pbf_wire_type::varint) && "not a varint");
        return get_varint<int32_t>();
    }

    /**
     * Consume and return value of current "sint32" varint field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "sint32".
     * @post The current field was consumed and there is no current field now.
     */
    inline int32_t get_sint32() {
        protozero_assert(has_wire_type(pbf_wire_type::varint) && "not a varint");
        return get_svarint<int32_t>();
    }

    /**
     * Consume and return value of current "uint32" varint field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "uint32".
     * @post The current field was consumed and there is no current field now.
     */
    inline uint32_t get_uint32() {
        protozero_assert(has_wire_type(pbf_wire_type::varint) && "not a varint");
        return get_varint<uint32_t>();
    }

    /**
     * Consume and return value of current "int64" varint field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "int64".
     * @post The current field was consumed and there is no current field now.
     */
    inline int64_t get_int64() {
        protozero_assert(has_wire_type(pbf_wire_type::varint) && "not a varint");
        return get_varint<int64_t>();
    }

    /**
     * Consume and return value of current "sint64" varint field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "sint64".
     * @post The current field was consumed and there is no current field now.
     */
    inline int64_t get_sint64() {
        protozero_assert(has_wire_type(pbf_wire_type::varint) && "not a varint");
        return get_svarint<int64_t>();
    }

    /**
     * Consume and return value of current "uint64" varint field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "uint64".
     * @post The current field was consumed and there is no current field now.
     */
    inline uint64_t get_uint64() {
        protozero_assert(has_wire_type(pbf_wire_type::varint) && "not a varint");
        return get_varint<uint64_t>();
    }

    /**
     * Consume and return value of current "fixed32" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "fixed32".
     * @post The current field was consumed and there is no current field now.
     */
    inline uint32_t get_fixed32();

    /**
     * Consume and return value of current "sfixed32" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "sfixed32".
     * @post The current field was consumed and there is no current field now.
     */
    inline int32_t get_sfixed32();

    /**
     * Consume and return value of current "fixed64" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "fixed64".
     * @post The current field was consumed and there is no current field now.
     */
    inline uint64_t get_fixed64();

    /**
     * Consume and return value of current "sfixed64" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "sfixed64".
     * @post The current field was consumed and there is no current field now.
     */
    inline int64_t get_sfixed64();

    /**
     * Consume and return value of current "float" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "float".
     * @post The current field was consumed and there is no current field now.
     */
    inline float get_float();

    /**
     * Consume and return value of current "double" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "double".
     * @post The current field was consumed and there is no current field now.
     */
    inline double get_double();

    /**
     * Consume and return value of current "bytes" or "string" field.
     *
     * @returns A pair with a pointer to the data and the length of the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "bytes" or "string".
     * @post The current field was consumed and there is no current field now.
     */
    inline std::pair<const char*, pbf_length_type> get_data();

    /**
     * Consume and return value of current "bytes" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "bytes".
     * @post The current field was consumed and there is no current field now.
     */
    inline std::string get_bytes();

    /**
     * Consume and return value of current "string" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "string".
     * @post The current field was consumed and there is no current field now.
     */
    inline std::string get_string();

    /**
     * Consume and return value of current "message" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "message".
     * @post The current field was consumed and there is no current field now.
     */
    inline pbf_reader get_message() {
        return pbf_reader(get_data());
    }

    ///@}

private:

    template <typename T>
    class const_varint_iterator : public std::iterator<std::forward_iterator_tag, T> {

    protected:

        const char* m_data;
        const char* m_end;

    public:

        const_varint_iterator() noexcept :
            m_data(nullptr),
            m_end(nullptr) {
        }

        const_varint_iterator(const char *data, const char* end) noexcept :
            m_data(data),
            m_end(end) {
        }

        const_varint_iterator(const const_varint_iterator&) noexcept = default;
        const_varint_iterator(const_varint_iterator&&) noexcept = default;

        const_varint_iterator& operator=(const const_varint_iterator&) noexcept = default;
        const_varint_iterator& operator=(const_varint_iterator&&) noexcept = default;

        ~const_varint_iterator() noexcept = default;

        T operator*() {
            const char* d = m_data; // will be thrown away
            return static_cast<T>(decode_varint(&d, m_end));
        }

        const_varint_iterator& operator++() {
            // Ignore the result, we call decode_varint() just for the
            // side-effect of updating m_data.
            decode_varint(&m_data, m_end);
            return *this;
        }

        const_varint_iterator operator++(int) {
            const const_varint_iterator tmp(*this);
            ++(*this);
            return tmp;
        }

        bool operator==(const const_varint_iterator& rhs) const noexcept {
            return m_data == rhs.m_data && m_end == rhs.m_end;
        }

        bool operator!=(const const_varint_iterator& rhs) const noexcept {
            return !(*this == rhs);
        }

    }; // class const_varint_iterator

    template <typename T>
    class const_svarint_iterator : public const_varint_iterator<T> {

    public:

        const_svarint_iterator() noexcept :
            const_varint_iterator<T>() {
        }

        const_svarint_iterator(const char *data, const char* end) noexcept :
            const_varint_iterator<T>(data, end) {
        }

        const_svarint_iterator(const const_svarint_iterator&) = default;
        const_svarint_iterator(const_svarint_iterator&&) = default;

        const_svarint_iterator& operator=(const const_svarint_iterator&) = default;
        const_svarint_iterator& operator=(const_svarint_iterator&&) = default;

        ~const_svarint_iterator() = default;

        T operator*() {
            const char* d = this->m_data; // will be thrown away
            return static_cast<T>(decode_zigzag64(decode_varint(&d, this->m_end)));
        }

        const_svarint_iterator& operator++() {
            // Ignore the result, we call decode_varint() just for the
            // side-effect of updating m_data.
            decode_varint(&this->m_data, this->m_end);
            return *this;
        }

        const_svarint_iterator operator++(int) {
            const const_svarint_iterator tmp(*this);
            ++(*this);
            return tmp;
        }

    }; // class const_svarint_iterator

public:

    /// Forward iterator for iterating over bool (int32 varint) values.
    typedef const_varint_iterator< int32_t> const_bool_iterator;

    /// Forward iterator for iterating over enum (int32 varint) values.
    typedef const_varint_iterator< int32_t> const_enum_iterator;

    /// Forward iterator for iterating over int32 (varint) values.
    typedef const_varint_iterator< int32_t> const_int32_iterator;

    /// Forward iterator for iterating over sint32 (varint) values.
    typedef const_svarint_iterator<int32_t> const_sint32_iterator;

    /// Forward iterator for iterating over uint32 (varint) values.
    typedef const_varint_iterator<uint32_t> const_uint32_iterator;

    /// Forward iterator for iterating over int64 (varint) values.
    typedef const_varint_iterator< int64_t> const_int64_iterator;

    /// Forward iterator for iterating over sint64 (varint) values.
    typedef const_svarint_iterator<int64_t> const_sint64_iterator;

    /// Forward iterator for iterating over uint64 (varint) values.
    typedef const_varint_iterator<uint64_t> const_uint64_iterator;

    ///@{
    /**
     * @name Repeated packed field accessor functions
     */

    /**
     * Consume current "repeated packed bool" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed bool".
     * @post The current field was consumed and there is no current field now.
     */
    inline std::pair<pbf_reader::const_bool_iterator, pbf_reader::const_bool_iterator> get_packed_bool();

    /**
     * Consume current "repeated packed enum" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed enum".
     * @post The current field was consumed and there is no current field now.
     */
    inline std::pair<pbf_reader::const_enum_iterator, pbf_reader::const_enum_iterator> get_packed_enum();

    /**
     * Consume current "repeated packed int32" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed int32".
     * @post The current field was consumed and there is no current field now.
     */
    inline std::pair<pbf_reader::const_int32_iterator, pbf_reader::const_int32_iterator> get_packed_int32();

    /**
     * Consume current "repeated packed sint32" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed sint32".
     * @post The current field was consumed and there is no current field now.
     */
    inline std::pair<pbf_reader::const_sint32_iterator, pbf_reader::const_sint32_iterator> get_packed_sint32();

    /**
     * Consume current "repeated packed uint32" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed uint32".
     * @post The current field was consumed and there is no current field now.
     */
    inline std::pair<pbf_reader::const_uint32_iterator, pbf_reader::const_uint32_iterator> get_packed_uint32();

    /**
     * Consume current "repeated packed int64" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed int64".
     * @post The current field was consumed and there is no current field now.
     */
    inline std::pair<pbf_reader::const_int64_iterator, pbf_reader::const_int64_iterator> get_packed_int64();

    /**
     * Consume current "repeated packed sint64" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed sint64".
     * @post The current field was consumed and there is no current field now.
     */
    inline std::pair<pbf_reader::const_sint64_iterator, pbf_reader::const_sint64_iterator> get_packed_sint64();

    /**
     * Consume current "repeated packed uint64" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed uint64".
     * @post The current field was consumed and there is no current field now.
     */
    inline std::pair<pbf_reader::const_uint64_iterator, pbf_reader::const_uint64_iterator> get_packed_uint64();

    /**
     * Consume current "repeated packed fixed32" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed fixed32".
     * @post The current field was consumed and there is no current field now.
     */
    inline auto get_packed_fixed32() -> decltype(packed_fixed<uint32_t>()) {
        return packed_fixed<uint32_t>();
    }

    /**
     * Consume current "repeated packed sfixed32" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed sfixed32".
     * @post The current field was consumed and there is no current field now.
     */
    inline auto get_packed_sfixed32() -> decltype(packed_fixed<int32_t>()) {
        return packed_fixed<int32_t>();
    }

    /**
     * Consume current "repeated packed fixed64" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed fixed64".
     * @post The current field was consumed and there is no current field now.
     */
    inline auto get_packed_fixed64() -> decltype(packed_fixed<uint64_t>()) {
        return packed_fixed<uint64_t>();
    }

    /**
     * Consume current "repeated packed sfixed64" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed sfixed64".
     * @post The current field was consumed and there is no current field now.
     */
    inline auto get_packed_sfixed64() -> decltype(packed_fixed<int64_t>()) {
        return packed_fixed<int64_t>();
    }

    /**
     * Consume current "repeated packed float" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed float".
     * @post The current field was consumed and there is no current field now.
     */
    inline auto get_packed_float() -> decltype(packed_fixed<float>()) {
        return packed_fixed<float>();
    }

    /**
     * Consume current "repeated packed double" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed double".
     * @post The current field was consumed and there is no current field now.
     */
    inline auto get_packed_double() -> decltype(packed_fixed<double>()) {
        return packed_fixed<double>();
    }

    ///@}

}; // class pbf_reader

pbf_reader::pbf_reader(const char *data, size_t length) noexcept
    : m_data(data),
      m_end(data + length),
      m_wire_type(pbf_wire_type::unknown),
      m_tag(0) {
}

pbf_reader::pbf_reader(std::pair<const char *, size_t> data) noexcept
    : m_data(data.first),
      m_end(data.first + data.second),
      m_wire_type(pbf_wire_type::unknown),
      m_tag(0) {
}

pbf_reader::pbf_reader(const std::string& data) noexcept
    : m_data(data.data()),
      m_end(data.data() + data.size()),
      m_wire_type(pbf_wire_type::unknown),
      m_tag(0) {
}

pbf_reader::operator bool() const noexcept {
    return m_data < m_end;
}

bool pbf_reader::next() {
    if (m_data == m_end) {
        return false;
    }

    auto value = get_varint<uint32_t>();
    m_tag = value >> 3;

    // tags 0 and 19000 to 19999 are not allowed as per
    // https://developers.google.com/protocol-buffers/docs/proto
    protozero_assert(((m_tag > 0 && m_tag < 19000) || (m_tag > 19999 && m_tag <= ((1 << 29) - 1))) && "tag out of range");

    m_wire_type = pbf_wire_type(value & 0x07);
    switch (m_wire_type) {
        case pbf_wire_type::varint:
        case pbf_wire_type::fixed64:
        case pbf_wire_type::length_delimited:
        case pbf_wire_type::fixed32:
            break;
        default:
            throw unknown_pbf_wire_type_exception();
    }

    return true;
}

bool pbf_reader::next(pbf_tag_type requested_tag) {
    while (next()) {
        if (m_tag == requested_tag) {
            return true;
        } else {
            skip();
        }
    }
    return false;
}

pbf_tag_type pbf_reader::tag() const noexcept {
    return m_tag;
}

pbf_wire_type pbf_reader::wire_type() const noexcept {
    return m_wire_type;
}

bool pbf_reader::has_wire_type(pbf_wire_type type) const noexcept {
    return wire_type() == type;
}

void pbf_reader::skip_bytes(pbf_length_type len) {
    if (m_data + len > m_end) {
        throw end_of_buffer_exception();
    }
    m_data += len;

// In debug builds reset the tag to zero so that we can detect (some)
// wrong code.
#ifndef NDEBUG
    m_tag = 0;
#endif
}

void pbf_reader::skip() {
    protozero_assert(tag() != 0 && "call next() before calling skip()");
    switch (wire_type()) {
        case pbf_wire_type::varint:
            (void)get_uint32(); // called for the side-effect of skipping value
            break;
        case pbf_wire_type::fixed64:
            skip_bytes(8);
            break;
        case pbf_wire_type::length_delimited:
            skip_bytes(get_length());
            break;
        case pbf_wire_type::fixed32:
            skip_bytes(4);
            break;
        default:
            throw unknown_pbf_wire_type_exception();
    }
}

pbf_length_type pbf_reader::get_len_and_skip() {
    auto len = get_length();
    skip_bytes(len);
    return len;
}

template <typename T>
T pbf_reader::get_varint() {
    return static_cast<T>(decode_varint(&m_data, m_end));
}

template <typename T>
T pbf_reader::get_svarint() {
    protozero_assert((has_wire_type(pbf_wire_type::varint) || has_wire_type(pbf_wire_type::length_delimited)) && "not a varint");
    return static_cast<T>(decode_zigzag64(decode_varint(&m_data, m_end)));
}

uint32_t pbf_reader::get_fixed32() {
    protozero_assert(tag() != 0 && "call next() before accessing field value");
    protozero_assert(has_wire_type(pbf_wire_type::fixed32) && "not a 32-bit fixed");
    return get_fixed<uint32_t>();
}

int32_t pbf_reader::get_sfixed32() {
    protozero_assert(tag() != 0 && "call next() before accessing field value");
    protozero_assert(has_wire_type(pbf_wire_type::fixed32) && "not a 32-bit fixed");
    return get_fixed<int32_t>();
}

uint64_t pbf_reader::get_fixed64() {
    protozero_assert(tag() != 0 && "call next() before accessing field value");
    protozero_assert(has_wire_type(pbf_wire_type::fixed64) && "not a 64-bit fixed");
    return get_fixed<uint64_t>();
}

int64_t pbf_reader::get_sfixed64() {
    protozero_assert(tag() != 0 && "call next() before accessing field value");
    protozero_assert(has_wire_type(pbf_wire_type::fixed64) && "not a 64-bit fixed");
    return get_fixed<int64_t>();
}

float pbf_reader::get_float() {
    protozero_assert(tag() != 0 && "call next() before accessing field value");
    protozero_assert(has_wire_type(pbf_wire_type::fixed32) && "not a 32-bit fixed");
    return get_fixed<float>();
}

double pbf_reader::get_double() {
    protozero_assert(tag() != 0 && "call next() before accessing field value");
    protozero_assert(has_wire_type(pbf_wire_type::fixed64) && "not a 64-bit fixed");
    return get_fixed<double>();
}

bool pbf_reader::get_bool() {
    protozero_assert(tag() != 0 && "call next() before accessing field value");
    protozero_assert(has_wire_type(pbf_wire_type::varint) && "not a varint");
    protozero_assert((*m_data & 0x80) == 0 && "not a 1 byte varint");
    skip_bytes(1);
    return m_data[-1] != 0; // -1 okay because we incremented m_data the line before
}

std::pair<const char*, pbf_length_type> pbf_reader::get_data() {
    protozero_assert(tag() != 0 && "call next() before accessing field value");
    protozero_assert(has_wire_type(pbf_wire_type::length_delimited) && "not of type string, bytes or message");
    auto len = get_len_and_skip();
    return std::make_pair(m_data-len, len);
}

std::string pbf_reader::get_bytes() {
    auto d = get_data();
    return std::string(d.first, d.second);
}

std::string pbf_reader::get_string() {
    return get_bytes();
}

std::pair<pbf_reader::const_bool_iterator, pbf_reader::const_bool_iterator> pbf_reader::get_packed_bool() {
    return get_packed_int32();
}

std::pair<pbf_reader::const_enum_iterator, pbf_reader::const_enum_iterator> pbf_reader::get_packed_enum() {
    return get_packed_int32();
}

std::pair<pbf_reader::const_int32_iterator, pbf_reader::const_int32_iterator> pbf_reader::get_packed_int32() {
    protozero_assert(tag() != 0 && "call next() before accessing field value");
    auto len = get_len_and_skip();
    return std::make_pair(pbf_reader::const_int32_iterator(m_data-len, m_data),
                          pbf_reader::const_int32_iterator(m_data, m_data));
}

std::pair<pbf_reader::const_uint32_iterator, pbf_reader::const_uint32_iterator> pbf_reader::get_packed_uint32() {
    protozero_assert(tag() != 0 && "call next() before accessing field value");
    auto len = get_len_and_skip();
    return std::make_pair(pbf_reader::const_uint32_iterator(m_data-len, m_data),
                          pbf_reader::const_uint32_iterator(m_data, m_data));
}

std::pair<pbf_reader::const_sint32_iterator, pbf_reader::const_sint32_iterator> pbf_reader::get_packed_sint32() {
    protozero_assert(tag() != 0 && "call next() before accessing field value");
    auto len = get_len_and_skip();
    return std::make_pair(pbf_reader::const_sint32_iterator(m_data-len, m_data),
                          pbf_reader::const_sint32_iterator(m_data, m_data));
}

std::pair<pbf_reader::const_int64_iterator, pbf_reader::const_int64_iterator> pbf_reader::get_packed_int64() {
    protozero_assert(tag() != 0 && "call next() before accessing field value");
    auto len = get_len_and_skip();
    return std::make_pair(pbf_reader::const_int64_iterator(m_data-len, m_data),
                          pbf_reader::const_int64_iterator(m_data, m_data));
}

std::pair<pbf_reader::const_uint64_iterator, pbf_reader::const_uint64_iterator> pbf_reader::get_packed_uint64() {
    protozero_assert(tag() != 0 && "call next() before accessing field value");
    auto len = get_len_and_skip();
    return std::make_pair(pbf_reader::const_uint64_iterator(m_data-len, m_data),
                          pbf_reader::const_uint64_iterator(m_data, m_data));
}

std::pair<pbf_reader::const_sint64_iterator, pbf_reader::const_sint64_iterator> pbf_reader::get_packed_sint64() {
    protozero_assert(tag() != 0 && "call next() before accessing field value");
    auto len = get_len_and_skip();
    return std::make_pair(pbf_reader::const_sint64_iterator(m_data-len, m_data),
                          pbf_reader::const_sint64_iterator(m_data, m_data));
}

} // end namespace protozero

#endif // PROTOZERO_PBF_READER_HPP

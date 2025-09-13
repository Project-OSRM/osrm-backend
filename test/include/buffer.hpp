#ifndef BUFFER_HPP
#define BUFFER_HPP

#include "test.hpp"

#include <protozero/buffer_fixed.hpp>
#include <protozero/buffer_string.hpp>
#include <protozero/buffer_vector.hpp>

// This "simulates" an externally defined buffer type to make sure our
// buffer adaptor functions do the right thing.
namespace test_external {
    class ext_buffer : public std::string {
    };
} // namespace test_external

namespace protozero {

    template <>
    struct buffer_customization<test_external::ext_buffer> {

        static std::size_t size(const test_external::ext_buffer* buffer) noexcept {
            return buffer->size();
        }

        static void append(test_external::ext_buffer* buffer, const char* data, std::size_t count) {
            buffer->append(data, count);
        }

        static void append_zeros(test_external::ext_buffer* buffer, std::size_t count) {
            buffer->append(count, '\0');
        }

        static void resize(test_external::ext_buffer* buffer, std::size_t size) {
            protozero_assert(size < buffer->size());
            buffer->resize(size);
        }

        static void reserve_additional(test_external::ext_buffer* buffer, std::size_t size) {
            buffer->reserve(buffer->size() + size);
        }

        static void erase_range(test_external::ext_buffer* buffer, std::size_t from, std::size_t to) {
            protozero_assert(from <= buffer->size());
            protozero_assert(to <= buffer->size());
            protozero_assert(from <= to);
            buffer->erase(std::next(buffer->begin(), static_cast<std::string::iterator::difference_type>(from)),
                          std::next(buffer->begin(), static_cast<std::string::iterator::difference_type>(to)));
        }

        static char* at_pos(test_external::ext_buffer* buffer, std::size_t pos) {
            protozero_assert(pos <= buffer->size());
            return (&*buffer->begin()) + pos;
        }

        static void push_back(test_external::ext_buffer* buffer, char ch) {
            buffer->push_back(ch);
        }

    };

} // namespace protozero

// The following structs are used in many tests using TEMPLATE_TEST_CASE() to
// test the different buffer types:
//
// 1. Dynamically sized buffer based on std::string.
// 2. Dynamically sized buffer based on std::vector<char>.
// 3. Statically sized buffer based on std::array<char, N>.
// 4. Externally defined buffer.

class buffer_test_string {

    std::string m_buffer;

public:

    using type = std::string;
    using writer_type = protozero::pbf_writer; // == protozero::basic_pbf_writer<type>;

    type& buffer() noexcept {
        return m_buffer;
    }

    const char *data() const noexcept {
        return m_buffer.data();
    }

    std::size_t size() const noexcept {
        return m_buffer.size();
    }
}; // class buffer_test_string

class buffer_test_vector {

    std::vector<char> m_buffer;

public:

    using type = std::vector<char>;
    using writer_type = protozero::basic_pbf_writer<type>;

    type& buffer() noexcept {
        return m_buffer;
    }

    const char *data() const noexcept {
        return m_buffer.data();
    }

    std::size_t size() const noexcept {
        return m_buffer.size();
    }
}; // class buffer_test_vector

class buffer_test_array {

public:
    using type = protozero::fixed_size_buffer_adaptor;
    using writer_type = protozero::basic_pbf_writer<type>;

    type& buffer() noexcept {
        return adaptor;
    }

    const char *data() const noexcept {
        return adaptor.data();
    }

    std::size_t size() const noexcept {
        return adaptor.size();
    }

private:

    std::array<char, 1024> m_buffer = {{0}};
    type adaptor{m_buffer};

}; // class buffer_test_array

class buffer_test_external {

    test_external::ext_buffer m_buffer;

public:

    using type = test_external::ext_buffer;
    using writer_type = protozero::basic_pbf_writer<type>;

    type& buffer() noexcept {
        return m_buffer;
    }

    const char *data() const noexcept {
        return m_buffer.data();
    }

    std::size_t size() const noexcept {
        return m_buffer.size();
    }
}; // class buffer_test_external

#endif // BUFFER_HPP

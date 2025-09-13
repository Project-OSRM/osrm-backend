#ifndef TEST_HPP
#define TEST_HPP

#include <catch.hpp> // IWYU pragma: export

#include <array> // IWYU pragma: export
#include <limits> // IWYU pragma: export
#include <string> // IWYU pragma: export
#include <vector> // IWYU pragma: export

#include <stdexcept>
// Define protozero_assert() to throw this error. This allows the tests to
// check that the assert fails.
struct assert_error : public std::runtime_error {
    explicit assert_error(const char* what_arg) : std::runtime_error(what_arg) {
    }
};
#define protozero_assert(x) if (!(x)) { throw assert_error{#x}; } // NOLINT(readability-simplify-boolean-expr)

#include <protozero/pbf_builder.hpp> // IWYU pragma: export
#include <protozero/pbf_message.hpp> // IWYU pragma: export
#include <protozero/pbf_reader.hpp> // IWYU pragma: export
#include <protozero/pbf_writer.hpp> // IWYU pragma: export

extern std::string load_data(const std::string& filename);

#define PROTOZERO_TEST_CONCAT2(x, y) x##y
#define PROTOZERO_TEST_CONCAT(x, y) PROTOZERO_TEST_CONCAT2(x, y)

#define PROTOZERO_TEST_STRING2(s) #s
#define PROTOZERO_TEST_STRING(s) PROTOZERO_TEST_STRING2(s)

#endif // TEST_HPP

#ifndef TEST_HPP
#define TEST_HPP

#include <catch.hpp>

#include <array>
#include <vector>

#include <stdexcept>
// Define protozero_assert() to throw this error. This allows the tests to
// check that the assert fails.
struct assert_error : public std::runtime_error {
    explicit assert_error(const char* what_arg) : std::runtime_error(what_arg) {
    }
};
#define protozero_assert(x) if (!(x)) { throw assert_error{#x}; }

#include <protozero/pbf_builder.hpp>
#include <protozero/pbf_message.hpp>
#include <protozero/pbf_reader.hpp>
#include <protozero/pbf_writer.hpp>

extern std::string load_data(const std::string& filename);

#define PROTOZERO_TEST_CONCAT2(x, y) x##y
#define PROTOZERO_TEST_CONCAT(x, y) PROTOZERO_TEST_CONCAT2(x, y)

#define PROTOZERO_TEST_STRING2(s) #s
#define PROTOZERO_TEST_STRING(s) PROTOZERO_TEST_STRING2(s)

#endif // TEST_HPP

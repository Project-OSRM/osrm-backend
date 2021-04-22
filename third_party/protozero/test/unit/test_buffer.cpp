
#include <buffer.hpp>

#include <algorithm>
#include <array>
#include <stdexcept>
#include <type_traits>

TEMPLATE_TEST_CASE("Use various buffer types", "", buffer_test_string, buffer_test_vector, buffer_test_array, buffer_test_external) {
    TestType tt;
    auto* buffer = &tt.buffer();

    using bc = protozero::buffer_customization<typename TestType::type>;

    REQUIRE(bc::size(buffer) == 0);

    bc::append(buffer, "abc def ghi", 11);
    REQUIRE(bc::size(buffer) == 11);

    bc::append_zeros(buffer, 3);
    REQUIRE(bc::size(buffer) == 14);

    bc::resize(buffer, 11);
    REQUIRE(bc::size(buffer) == 11);

    bc::append(buffer, " jkl", 4);
    REQUIRE(bc::size(buffer) == 15);

    bc::erase_range(buffer, 4, 8);
    REQUIRE(bc::size(buffer) == 11);
    REQUIRE(std::equal(bc::at_pos(buffer, 0),
                       bc::at_pos(buffer, bc::size(buffer)),
                       "abc ghi jkl"));

    buffer->push_back(' ');
    buffer->push_back('x');
    buffer->push_back('y');
    REQUIRE(bc::size(buffer) == 14);
    REQUIRE(std::equal(bc::at_pos(buffer, 0),
                       bc::at_pos(buffer, bc::size(buffer)),
                       "abc ghi jkl xy"));

    REQUIRE(std::equal(buffer->cbegin(), buffer->cend(), "abc ghi jkl xy"));
}

TEST_CASE("fixed_size_buffer_adaptor has limited size") {
    std::array<char, 5> data = {{0}};
    protozero::fixed_size_buffer_adaptor fsba{&*data.begin(), data.size()};
    REQUIRE_THROWS_AS(protozero::buffer_customization<protozero::fixed_size_buffer_adaptor>::append(&fsba, "0123456789", 10), std::length_error);
}

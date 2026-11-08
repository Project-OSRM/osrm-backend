#include "util/json_container.hpp"
#include "util/json_renderer.hpp"

#include <boost/test/unit_test.hpp>

#include <array>
#include <bit>
#include <cmath>
#include <format>
#include <limits>
#include <locale>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace osrm::util::json
{

BOOST_AUTO_TEST_SUITE(json_renderer)

BOOST_AUTO_TEST_CASE(number_truncating)
{
    std::string str;
    Renderer<std::string> renderer(str);

    // this number would have more than 10 decimals if not truncated
    renderer(Number{42.9995999594999399299});
    BOOST_CHECK_EQUAL(str, "42.99959996");
}

BOOST_AUTO_TEST_CASE(integer)
{
    std::string str;
    Renderer<std::string> renderer(str);
    renderer(Number{42.0});
    BOOST_CHECK_EQUAL(str, "42");
}

BOOST_AUTO_TEST_CASE(test_json_issue_6531)
{
    std::string output;
    osrm::util::json::Renderer<std::string> renderer(output);
    renderer(0.0000000000017114087924596788);
    BOOST_CHECK_EQUAL(output, "1.711408792e-12");

    output.clear();
    renderer(42.0);
    BOOST_CHECK_EQUAL(output, "42");

    output.clear();
    renderer(42.1);
    BOOST_CHECK_EQUAL(output, "42.1");

    output.clear();
    renderer(42.12);
    BOOST_CHECK_EQUAL(output, "42.12");

    output.clear();
    renderer(42.123);
    BOOST_CHECK_EQUAL(output, "42.123");

    output.clear();
    renderer(42.1234);
    BOOST_CHECK_EQUAL(output, "42.1234");

    output.clear();
    renderer(42.12345);
    BOOST_CHECK_EQUAL(output, "42.12345");

    output.clear();
    renderer(42.123456);
    BOOST_CHECK_EQUAL(output, "42.123456");

    output.clear();
    renderer(42.1234567);
    BOOST_CHECK_EQUAL(output, "42.1234567");

    output.clear();
    renderer(42.12345678);
    BOOST_CHECK_EQUAL(output, "42.12345678");

    output.clear();
    renderer(42.123456789);
    BOOST_CHECK_EQUAL(output, "42.12345679");

    output.clear();
    renderer(0.12345678912345);
    BOOST_CHECK_EQUAL(output, "0.1234567891");

    output.clear();
    renderer(0.123456789);
    BOOST_CHECK_EQUAL(output, "0.123456789");

    output.clear();
    renderer(0.12345678916);
    BOOST_CHECK_EQUAL(output, "0.1234567892");

    output.clear();
    renderer(123456789123456789.);
    BOOST_CHECK_EQUAL(output, "1.234567891e+17");
}

// Tests for issue #7016: large OSM IDs rendered as scientific notation
BOOST_AUTO_TEST_CASE(test_large_osm_ids)
{
    std::string output;
    Renderer<std::string> renderer(output);

    // Test from original issue #7016 - was showing as 1.111742119e+10
    output.clear();
    renderer(Number{11117421192.0});
    BOOST_CHECK_EQUAL(output, "11117421192");

    // Test precision loss case - was becoming 10003966160
    output.clear();
    renderer(Number{10003966158.0});
    BOOST_CHECK_EQUAL(output, "10003966158");

    // Test 11-digit OSM IDs (current max range)
    output.clear();
    renderer(Number{12345678901.0});
    BOOST_CHECK_EQUAL(output, "12345678901");

    // Test 12-digit values
    output.clear();
    renderer(Number{100396615812.0});
    BOOST_CHECK_EQUAL(output, "100396615812");

    // Test values up to 2^53 (max exact integer in double)
    output.clear();
    renderer(Number{9007199254740992.0}); // 2^53
    BOOST_CHECK_EQUAL(output, "9007199254740992");

    // Test zero
    output.clear();
    renderer(Number{0.0});
    BOOST_CHECK_EQUAL(output, "0");

    // Test small integers
    output.clear();
    renderer(Number{42.0});
    BOOST_CHECK_EQUAL(output, "42");

    // Test 1
    output.clear();
    renderer(Number{1.0});
    BOOST_CHECK_EQUAL(output, "1");
}

BOOST_AUTO_TEST_CASE(test_floats_unchanged)
{
    std::string output;
    Renderer<std::string> renderer(output);

    // Floats should still use {:.10g} format
    output.clear();
    renderer(Number{3.14159});
    BOOST_CHECK_EQUAL(output, "3.14159");

    output.clear();
    renderer(Number{42.5});
    BOOST_CHECK_EQUAL(output, "42.5");

    // Very small numbers still use scientific notation
    output.clear();
    renderer(Number{0.0000000000017114087924596788});
    BOOST_CHECK_EQUAL(output, "1.711408792e-12");

    // Negative numbers use float format
    output.clear();
    renderer(Number{-42.0});
    BOOST_CHECK_EQUAL(output, "-42");

    output.clear();
    renderer(Number{42.9995999594999399299});
    BOOST_CHECK_EQUAL(output, "42.99959996");
}

BOOST_AUTO_TEST_CASE(test_edge_cases)
{
    std::string output;
    Renderer<std::string> renderer(output);

    // Negative zero should be treated as a whole number: it passes the non-negativity
    // check, std::trunc(-0.0) == -0.0, so it goes through the integer path and is "0"
    output.clear();
    renderer(Number{-0.0});
    BOOST_CHECK_EQUAL(output, "0");

    // Very large float (larger than max uint64) uses scientific notation
    output.clear();
    renderer(Number{1.8446744073709552e+19}); // > uint64 max
    BOOST_CHECK_EQUAL(output, "1.844674407e+19");
}

BOOST_AUTO_TEST_CASE(test_nan_infinity_produce_null)
{
    std::string output;
    Renderer<std::string> renderer(output);

    // NaN should produce null (valid JSON)
    output.clear();
    renderer(Number{std::numeric_limits<double>::quiet_NaN()});
    BOOST_CHECK_EQUAL(output, "null");

    // Positive infinity should produce null
    output.clear();
    renderer(Number{std::numeric_limits<double>::infinity()});
    BOOST_CHECK_EQUAL(output, "null");

    // Negative infinity should produce null
    output.clear();
    renderer(Number{-std::numeric_limits<double>::infinity()});
    BOOST_CHECK_EQUAL(output, "null");
}

BOOST_AUTO_TEST_CASE(test_nan_in_array)
{
    std::string output;
    Renderer<std::string> renderer(output);

    Array arr;
    arr.values.push_back(Number{42.0});
    arr.values.push_back(Number{std::numeric_limits<double>::quiet_NaN()});
    arr.values.push_back(Number{3.14});
    renderer(arr);
    BOOST_CHECK_EQUAL(output, "[42,null,3.14]");
}

BOOST_AUTO_TEST_CASE(test_integer_in_array)
{
    std::string output;
    Renderer<std::string> renderer(output);

    Array arr;
    arr.values.push_back(Number{11117421192.0});
    arr.values.push_back(Number{10003966158.0});
    arr.values.push_back(Number{3.14});
    renderer(arr);
    BOOST_CHECK_EQUAL(output, "[11117421192,10003966158,3.14]");
}

BOOST_AUTO_TEST_CASE(number_formatting_extremes_and_notation_boundaries)
{
    const std::array cases = {
        std::pair{std::numeric_limits<double>::max(), "1.797693135e+308"},
        std::pair{std::numeric_limits<double>::lowest(), "-1.797693135e+308"},
        std::pair{std::numeric_limits<double>::min(), "2.225073859e-308"},
        std::pair{-std::numeric_limits<double>::min(), "-2.225073859e-308"},
        std::pair{std::numeric_limits<double>::denorm_min(), "4.940656458e-324"},
        std::pair{-std::numeric_limits<double>::denorm_min(), "-4.940656458e-324"},
        std::pair{0.0001, "0.0001"},
        std::pair{0.00001, "1e-05"},
        std::pair{-0.00001, "-1e-05"},
        std::pair{9999999999.5, "1e+10"},
        std::pair{10000000000.0, "10000000000"},
        std::pair{10000000000.5, "1e+10"},
        std::pair{-9999999999.0, "-9999999999"},
        std::pair{-10000000000.0, "-1e+10"},
        std::pair{9007199254740992.0, "9007199254740992"},
        std::pair{std::nextafter(9007199254740992.0, 0.0), "9007199254740991"},
        std::pair{std::nextafter(9007199254740992.0, std::numeric_limits<double>::infinity()),
                  "9.007199255e+15"},
    };

    for (const auto &[number, expected] : cases)
    {
        std::string output;
        Renderer<std::string>{output}(Number{number});
        BOOST_CHECK_EQUAL(output, expected);
    }
}

BOOST_AUTO_TEST_CASE(number_formatting_preserves_the_previous_format_contract)
{
    const auto check_number = [](const double number)
    {
        std::string expected;
        constexpr auto max_exact_int =
            static_cast<double>(1ULL << std::numeric_limits<double>::digits);
        if (!std::isfinite(number))
            expected = "null";
        else if (number >= 0. && number <= max_exact_int && std::trunc(number) == number)
            expected = std::format("{}", static_cast<std::uint64_t>(number));
        else
            expected = std::format("{:.10g}", number);

        std::string output;
        Renderer<std::string>{output}(Number{number});
        BOOST_CHECK_EQUAL(output, expected);
    };

    std::mt19937_64 generator{7761};
    for (int sample = 0; sample < 10'000; ++sample)
        check_number(std::bit_cast<double>(generator()));

    for (int exponent = -323; exponent <= 308; ++exponent)
    {
        const auto number = std::pow(10., exponent);
        for (const auto neighbor :
             {std::nextafter(number, 0.),
              number,
              std::nextafter(number, std::numeric_limits<double>::infinity())})
        {
            check_number(neighbor);
            check_number(-neighbor);
        }
    }
}

BOOST_AUTO_TEST_CASE(number_formatting_is_consistent_across_output_sinks)
{
    const Array numbers{{Number{0.},
                         Number{-0.},
                         Number{42.5},
                         Number{11117421192.},
                         Number{0.00001},
                         Number{std::numeric_limits<double>::lowest()},
                         Number{std::numeric_limits<double>::quiet_NaN()},
                         Number{std::numeric_limits<double>::infinity()}}};
    constexpr std::string_view expected =
        "[0,0,42.5,11117421192,1e-05,-1.797693135e+308,null,null]";

    std::string string_output;
    Renderer<std::string>{string_output}(numbers);
    BOOST_CHECK_EQUAL(string_output, expected);

    std::vector<char> vector_output;
    Renderer<std::vector<char>>{vector_output}(numbers);
    BOOST_CHECK_EQUAL(std::string(vector_output.begin(), vector_output.end()), expected);

    std::ostringstream stream_output;
    Renderer<std::ostream>{stream_output}(numbers);
    BOOST_CHECK_EQUAL(stream_output.str(), expected);
}

BOOST_AUTO_TEST_CASE(number_formatting_ignores_the_stream_locale)
{
    struct CommaDecimalPoint final : std::numpunct<char>
    {
        char do_decimal_point() const override { return ','; }
    };

    std::ostringstream output;
    output.imbue(std::locale(std::locale::classic(), new CommaDecimalPoint));
    Renderer<std::ostream>{output}(Number{1234.5});
    BOOST_CHECK_EQUAL(output.str(), "1234.5");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace osrm::util::json

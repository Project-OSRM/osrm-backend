
#include <test.hpp>

namespace TestBoolean {

enum class Test : protozero::pbf_tag_type {
    required_bool_b = 1
};

} // end namespace TestBoolean

TEST_CASE("read bool field using pbf_reader: false") {
    const std::string buffer = load_data("bool/data-false");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE_FALSE(item.get_bool());
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read bool field using pbf_reader: true") {
    const std::string buffer = load_data("bool/data-true");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE(item.get_bool());
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read bool field using pbf_reader: also true") {
    const std::string buffer = load_data("bool/data-also-true");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next(1));
    REQUIRE(item.get_bool());
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read bool field using pbf_reader: still true") {
    const std::string buffer = load_data("bool/data-still-true");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next(1));
    REQUIRE(item.get_bool());
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read bool field using pbf_message: false") {
    const std::string buffer = load_data("bool/data-false");

    protozero::pbf_message<TestBoolean::Test> item{buffer};

    REQUIRE(item.next());
    REQUIRE_FALSE(item.get_bool());
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read bool field using pbf_message: true") {
    const std::string buffer = load_data("bool/data-true");

    protozero::pbf_message<TestBoolean::Test> item{buffer};

    REQUIRE(item.next());
    REQUIRE(item.get_bool());
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read bool field using pbf_message: also true") {
    const std::string buffer = load_data("bool/data-also-true");

    protozero::pbf_message<TestBoolean::Test> item{buffer};

    REQUIRE(item.next(TestBoolean::Test::required_bool_b));
    REQUIRE(item.get_bool());
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read bool field using pbf_message: still true") {
    const std::string buffer = load_data("bool/data-still-true");

    protozero::pbf_message<TestBoolean::Test> item{buffer};

    REQUIRE(item.next(TestBoolean::Test::required_bool_b));
    REQUIRE(item.get_bool());
    REQUIRE_FALSE(item.next());
}

TEST_CASE("write bool field using pbf_writer") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    SECTION("false") {
        pw.add_bool(1, false);
        REQUIRE(buffer == load_data("bool/data-false"));
    }

    SECTION("true") {
        pw.add_bool(1, true);
        REQUIRE(buffer == load_data("bool/data-true"));
    }
}

TEST_CASE("write bool field using pbf_builder") {
    std::string buffer;
    protozero::pbf_builder<TestBoolean::Test> pw{buffer};

    SECTION("false") {
        pw.add_bool(TestBoolean::Test::required_bool_b, false);
        REQUIRE(buffer == load_data("bool/data-false"));
    }

    SECTION("true") {
        pw.add_bool(TestBoolean::Test::required_bool_b, true);
        REQUIRE(buffer == load_data("bool/data-true"));
    }
}

TEST_CASE("write bool field using moved pbf_builder") {
    std::string buffer;
    protozero::pbf_builder<TestBoolean::Test> pw2{buffer};
    REQUIRE(pw2.valid());

    protozero::pbf_builder<TestBoolean::Test> pw{std::move(pw2)};
    REQUIRE(pw.valid());
    REQUIRE_FALSE(pw2.valid()); // NOLINT(hicpp-invalid-access-moved, bugprone-use-after-move, clang-analyzer-cplusplus.Move)

    SECTION("false") {
        pw.add_bool(TestBoolean::Test::required_bool_b, false);
        REQUIRE(buffer == load_data("bool/data-false"));
    }

    SECTION("true") {
        pw.add_bool(TestBoolean::Test::required_bool_b, true);
        REQUIRE(buffer == load_data("bool/data-true"));
    }
}

TEST_CASE("read bool from using pbf_reader: truncated message") {
    std::vector<char> buffer = { 0x08 };

    protozero::pbf_reader item{buffer.data(), buffer.size()};

    REQUIRE(item.next());
    REQUIRE_THROWS_AS(item.get_bool(), protozero::end_of_buffer_exception);
}

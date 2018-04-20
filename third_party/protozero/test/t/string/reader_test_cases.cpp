
#include <test.hpp>

TEST_CASE("read string field using get_string: empty") {
    const std::string buffer = load_data("string/data-empty");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE(item.get_string().empty());
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read string field using get_string: one") {
    const std::string buffer = load_data("string/data-one");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE(item.get_string() == "x");
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read string field using get_string: string") {
    const std::string buffer = load_data("string/data-string");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE(item.get_string() == "foobar");
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read string field using get_string: end of buffer") {
    const std::string buffer = load_data("string/data-string");

    for (std::string::size_type i = 1; i < buffer.size(); ++i) {
        protozero::pbf_reader item{buffer.data(), i};
        REQUIRE(item.next());
        REQUIRE_THROWS_AS(item.get_string(), const protozero::end_of_buffer_exception&);
    }
}

TEST_CASE("read string field using get_view: empty") {
    const std::string buffer = load_data("string/data-empty");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    const auto v = item.get_view();
    REQUIRE(v.empty());
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read string field using get_view: one") {
    const std::string buffer = load_data("string/data-one");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    const auto v = item.get_view();
    REQUIRE(*v.data() == 'x');
    REQUIRE(v.size() == 1);
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read string field using get_view: string") {
    const std::string buffer = load_data("string/data-string");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE(std::string(item.get_view()) == "foobar");
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read string field using get_view: end of buffer") {
    const std::string buffer = load_data("string/data-string");

    for (std::string::size_type i = 1; i < buffer.size(); ++i) {
        protozero::pbf_reader item{buffer.data(), i};
        REQUIRE(item.next());
        REQUIRE_THROWS_AS(item.get_view(), const protozero::end_of_buffer_exception&);
    }
}

TEST_CASE("write string field") {
    std::string buffer_test;
    protozero::pbf_writer pbf_test{buffer_test};

    SECTION("empty") {
        pbf_test.add_string(1, "");
        REQUIRE(buffer_test == load_data("string/data-empty"));
    }

    SECTION("one") {
        pbf_test.add_string(1, "x");
        REQUIRE(buffer_test == load_data("string/data-one"));
    }

    SECTION("string") {
        pbf_test.add_string(1, "foobar");
        REQUIRE(buffer_test == load_data("string/data-string"));
    }
}


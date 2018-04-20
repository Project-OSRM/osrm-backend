
#include <test.hpp>

TEST_CASE("read repeated fields: empty") {
    const std::string buffer = load_data("repeated/data-empty");

    protozero::pbf_reader item{buffer};

    REQUIRE_FALSE(item.next());
}

TEST_CASE("read repeated fields: one") {
    const std::string buffer = load_data("repeated/data-one");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE(item.get_int32() == 0L);
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read repeated fields: many") {
    const std::string buffer = load_data("repeated/data-many");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE(item.get_int32() == 0L);

    REQUIRE(item.next());
    REQUIRE(item.get_int32() == 1L);

    REQUIRE(item.next());
    REQUIRE(item.get_int32() == -1L);

    REQUIRE(item.next());
    REQUIRE(item.get_int32() == std::numeric_limits<int32_t>::max());

    REQUIRE(item.next());
    REQUIRE(item.get_int32() == std::numeric_limits<int32_t>::min());

    REQUIRE_FALSE(item.next());
}

TEST_CASE("read repeated fields: end of buffer") {
    const std::string buffer = load_data("repeated/data-one");

    for (std::string::size_type i = 1; i < buffer.size(); ++i) {
        protozero::pbf_reader item{buffer.data(), i};
        REQUIRE(item.next());
        REQUIRE_THROWS_AS(item.get_int32(), const protozero::end_of_buffer_exception&);
    }
}

TEST_CASE("write repeated fields") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    SECTION("one") {
        pw.add_int32(1, 0L);
        REQUIRE(buffer == load_data("repeated/data-one"));
    }

    SECTION("many") {
        pw.add_int32(1, 0L);
        pw.add_int32(1, 1L);
        pw.add_int32(1, -1L);
        pw.add_int32(1, std::numeric_limits<int32_t>::max());
        pw.add_int32(1, std::numeric_limits<int32_t>::min());

        REQUIRE(buffer == load_data("repeated/data-many"));
    }
}


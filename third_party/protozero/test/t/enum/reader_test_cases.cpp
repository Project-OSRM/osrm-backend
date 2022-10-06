
#include <test.hpp>

TEST_CASE("read enum field: zero") {
    const std::string buffer = load_data("enum/data-black");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE(item.get_enum() == 0L);
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read enum field: positive") {
    const std::string buffer = load_data("enum/data-blue");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE(item.get_enum() == 3L);
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read enum field: negative") {
    const std::string buffer = load_data("enum/data-neg");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE(item.get_enum() == -1L);
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read enum field: max") {
    const std::string buffer = load_data("enum/data-max");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE(item.get_enum() == std::numeric_limits<int32_t>::max());
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read enum field: min") {
    const std::string buffer = load_data("enum/data-min");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE(item.get_enum() == (std::numeric_limits<int32_t>::min() + 1));
    REQUIRE_FALSE(item.next());
}

TEST_CASE("write enum field") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    SECTION("zero") {
        pw.add_enum(1, 0L);
        REQUIRE(buffer == load_data("enum/data-black"));
    }

    SECTION("positive") {
        pw.add_enum(1, 3L);
        REQUIRE(buffer == load_data("enum/data-blue"));
    }

    SECTION("negative") {
        pw.add_enum(1, -1L);
        REQUIRE(buffer == load_data("enum/data-neg"));
    }

    SECTION("max") {
        pw.add_enum(1, std::numeric_limits<int32_t>::max());
        REQUIRE(buffer == load_data("enum/data-max"));
    }

    SECTION("min") {
        pw.add_enum(1, std::numeric_limits<int32_t>::min() + 1);
        REQUIRE(buffer == load_data("enum/data-min"));
    }
}



#include <test.hpp>

inline void check_tag(const std::string& buffer, protozero::pbf_tag_type tag) {
    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE(item.tag() == tag);
    REQUIRE(item.get_int32() == 333L);
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read tag: 1") {
    check_tag(load_data("tags/data-tag-1"), 1ul);
}

TEST_CASE("read tag: 200") {
    check_tag(load_data("tags/data-tag-200"), 200ul);
}

TEST_CASE("read tag: 200000") {
    check_tag(load_data("tags/data-tag-200000"), 200000ul);
}

TEST_CASE("read tag: max") {
    check_tag(load_data("tags/data-tag-max"), (1ul << 29u) - 1u);
}

TEST_CASE("write tags") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    SECTION("tag 1") {
        pw.add_int32(1L, 333L);
        REQUIRE(buffer == load_data("tags/data-tag-1"));
    }

    SECTION("tag 200") {
        pw.add_int32(200L, 333L);
        REQUIRE(buffer == load_data("tags/data-tag-200"));
    }

    SECTION("tag 200000") {
        pw.add_int32(200000L, 333L);
        REQUIRE(buffer == load_data("tags/data-tag-200000"));
    }

    SECTION("tag max") {
        pw.add_int32(static_cast<int32_t>((1ul << 29u) - 1u), 333L);
        REQUIRE(buffer == load_data("tags/data-tag-max"));
    }
}


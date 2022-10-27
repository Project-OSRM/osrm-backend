
#include <test.hpp>

#define PBF_TYPE sfixed32
#define PBF_TYPE_IS_SIGNED 1
using cpp_type = int32_t;

#include <packed_access.hpp>

TEST_CASE("length value must be dividable by sizeof(T)") {
    std::string data{load_data("repeated_packed_sfixed32/data-many")};

    SECTION("1") {
        data[1] = 1;
    }

    SECTION("2") {
        data[1] = 2;
    }

    SECTION("3") {
        data[1] = 3;
    }

    protozero::pbf_reader item{data};

    REQUIRE(item.next());
    REQUIRE_THROWS_AS(item.get_packed_sfixed32(), protozero::invalid_length_exception);
}


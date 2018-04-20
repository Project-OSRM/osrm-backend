
#include <test.hpp>

// Run these tests twice, the second time we basically move the data
// one byte down in the buffer. It doesn't matter how the data or buffer
// is aligned before that, in at least one of these cases the int32 will
// not be aligned properly. So we test that even in that case the int32
// will be extracted properly.

TEST_CASE("check alignment issues for fixed32 field") {
    for (std::string::size_type n = 0; n < 2; ++n) {

        std::string abuffer;
        abuffer.reserve(1000);
        abuffer.append(n, '\0');

        SECTION("zero") {
            abuffer.append(load_data("fixed32/data-zero"));
            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE(item.next());
            REQUIRE(item.get_fixed32() == 0UL);
            REQUIRE_FALSE(item.next());
        }

        SECTION("positive") {
            abuffer.append(load_data("fixed32/data-pos"));
            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE(item.next());
            REQUIRE(item.get_fixed32() == 1UL);
            REQUIRE_FALSE(item.next());
        }

        SECTION("max") {
            abuffer.append(load_data("fixed32/data-max"));
            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE(item.next());
            REQUIRE(item.get_fixed32() == std::numeric_limits<uint32_t>::max());
            REQUIRE_FALSE(item.next());
        }

        SECTION("end_of_buffer") {
            abuffer.append(load_data("fixed32/data-pos"));

            for (std::string::size_type i = 1; i < abuffer.size() - n; ++i) {
                protozero::pbf_reader item{abuffer.data() + n, i};
                REQUIRE(item.next());
                REQUIRE_THROWS_AS(item.get_fixed32(), const protozero::end_of_buffer_exception&);
            }
        }

        SECTION("assert detecting tag==0") {
            abuffer.append(load_data("fixed32/data-zero"));
            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE_THROWS_AS(item.get_fixed32(), const assert_error&);
            REQUIRE(item.next());
            REQUIRE(item.get_fixed32() == 0UL);
            REQUIRE_THROWS(item.get_fixed32());
            REQUIRE_FALSE(item.next());
        }

        SECTION("skip") {
            abuffer.append(load_data("fixed32/data-zero"));
            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE_THROWS_AS(item.skip(), const assert_error&);
            REQUIRE(item.next());
            item.skip();
            REQUIRE_THROWS(item.skip());
            REQUIRE_FALSE(item.next());
        }
    }
}

TEST_CASE("check alignment issues for fixed64 field") {
    for (std::string::size_type n = 0; n < 2; ++n) {
        std::string abuffer;
        abuffer.reserve(1000);
        abuffer.append(n, '\0');

        SECTION("zero") {
            abuffer.append(load_data("fixed64/data-zero"));
            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE(item.next());
            REQUIRE(item.get_fixed64() == 0ULL);
            REQUIRE_FALSE(item.next());
        }

        SECTION("positive") {
            abuffer.append(load_data("fixed64/data-pos"));
            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE(item.next());
            REQUIRE(item.get_fixed64() == 1ULL);
            REQUIRE_FALSE(item.next());
        }

        SECTION("max") {
            abuffer.append(load_data("fixed64/data-max"));
            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE(item.next());
            REQUIRE(item.get_fixed64() == std::numeric_limits<uint64_t>::max());
            REQUIRE_FALSE(item.next());
        }

        SECTION("end_of_buffer") {
            abuffer.append(load_data("fixed64/data-pos"));

            for (std::string::size_type i = 1; i < abuffer.size() - n; ++i) {
                protozero::pbf_reader item{abuffer.data() + n, i};
                REQUIRE(item.next());
                REQUIRE_THROWS_AS(item.get_fixed64(), const protozero::end_of_buffer_exception&);
            }
        }
    }
}


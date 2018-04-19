
#include <test.hpp>

TEST_CASE("read float field") {
    // Run these tests twice, the second time we basically move the data
    // one byte down in the buffer. It doesn't matter how the data or buffer
    // is aligned before that, in at least one of these cases the float will
    // not be aligned properly. So we test that even in that case the float
    // will be extracted properly.

    for (std::string::size_type n = 0; n < 2; ++n) {
        std::string abuffer;
        abuffer.reserve(1000);
        abuffer.append(n, '\0');

        SECTION("zero") {
            abuffer.append(load_data("float/data-zero"));
            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE(item.next());
            REQUIRE(double(item.get_float()) == Approx(0.0));
            REQUIRE_FALSE(item.next());
        }

        SECTION("positive") {
            abuffer.append(load_data("float/data-pos"));
            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE(item.next());
            REQUIRE(double(item.get_float()) == Approx(5.34));
            REQUIRE_FALSE(item.next());
        }

        SECTION("negative") {
            abuffer.append(load_data("float/data-neg"));
            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE(item.next());
            REQUIRE(double(item.get_float()) == Approx(-1.71));
            REQUIRE_FALSE(item.next());
        }

        SECTION("end_of_buffer") {
            abuffer.append(load_data("float/data-neg"));

            for (std::string::size_type i = 1; i < abuffer.size() - n; ++i) {
                protozero::pbf_reader item{abuffer.data() + n, i};
                REQUIRE(item.next());
                REQUIRE_THROWS_AS(item.get_float(), const protozero::end_of_buffer_exception&);
            }
        }
    }
}

TEST_CASE("write float field") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    SECTION("zero") {
        pw.add_float(1, 0.0f);
        REQUIRE(buffer == load_data("float/data-zero"));
    }

    SECTION("positive") {
        pw.add_float(1, 5.34f);
        REQUIRE(buffer == load_data("float/data-pos"));
    }

    SECTION("negative") {
        pw.add_float(1, -1.71f);
        REQUIRE(buffer == load_data("float/data-neg"));
    }
}


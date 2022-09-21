
#include <test.hpp>

#include <array>

TEST_CASE("read repeated packed double field") {
    // Run these tests twice, the second time we basically move the data
    // one byte down in the buffer. It doesn't matter how the data or buffer
    // is aligned before that, in at least one of these cases the doubles will
    // not be aligned properly. So we test that even in that case the doubles
    // will be extracted properly.

    for (std::string::size_type n = 0; n < 2; ++n) {
        std::string abuffer;
        abuffer.reserve(1000);
        abuffer.append(n, '\0');

        SECTION("empty") {
            abuffer.append(load_data("repeated_packed_double/data-empty"));
            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE_FALSE(item.next());
        }

        SECTION("one") {
            abuffer.append(load_data("repeated_packed_double/data-one"));
            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE(item.next());
            const auto it_range = item.get_packed_double();
            REQUIRE_FALSE(item.next());

            REQUIRE(*it_range.begin() == Approx(17.34));
            REQUIRE(std::next(it_range.begin()) == it_range.end());
        }

        SECTION("many") {
            abuffer.append(load_data("repeated_packed_double/data-many"));
            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE(item.next());
            const auto it_range = item.get_packed_double();
            REQUIRE_FALSE(item.next());

            auto it = it_range.begin();
            REQUIRE(*it++ == Approx(17.34));
            REQUIRE(*it++ == Approx( 0.0));
            REQUIRE(*it++ == Approx( 1.0));
            REQUIRE(*it++ == std::numeric_limits<double>::min());
            REQUIRE(*it++ == std::numeric_limits<double>::max());
            REQUIRE(it == it_range.end());

            it = it_range.begin();
            auto it2 = it + 1;
            REQUIRE(it2 > it);
            REQUIRE(it < it2);
            REQUIRE(it <= it_range.begin());
            REQUIRE(it >= it_range.begin());
            REQUIRE(*it2 == Approx(0.0));
            auto it3 = 1 + it;
            REQUIRE(*it3 == Approx(0.0));
            auto it4 = it2 - 1;
            REQUIRE(*it4 == Approx(17.34));
            it4 += 2;
            REQUIRE(*it4 == Approx(1.0));
            it4 -= 2;
            REQUIRE(*it4 == Approx(17.34));
            it4 += 2;
            REQUIRE(*it4 == Approx(1.0));
            REQUIRE(*it4-- == Approx(1.0));
            REQUIRE(*it4 == Approx(0.0));
            REQUIRE(*--it4 == Approx(17.34));
            REQUIRE(it4[0] == Approx(17.34));
            REQUIRE(it4[1] == Approx(0.0));
            REQUIRE(std::distance(it_range.begin(), it_range.end()) == 5);
            REQUIRE(it_range.end() - it_range.begin() == 5);
            REQUIRE(it_range.begin() - it_range.end() == -5);

        }

        SECTION("end_of_buffer") {
            abuffer.append(load_data("repeated_packed_double/data-many"));

            for (std::string::size_type i = 1; i < abuffer.size() - n; ++i) {
                protozero::pbf_reader item{abuffer.data() + n, i};
                REQUIRE(item.next());
                REQUIRE_THROWS_AS(item.get_packed_double(), protozero::end_of_buffer_exception);
            }
        }
    }
}

TEST_CASE("write repeated packed double field") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    SECTION("empty") {
        const std::array<double, 1> data = {{ 17.34 }};
        pw.add_packed_double(1, std::begin(data), std::begin(data) /* !!!! */);

        REQUIRE(buffer == load_data("repeated_packed_double/data-empty"));
    }

    SECTION("one") {
        const std::array<double, 1> data = {{ 17.34 }};
        pw.add_packed_double(1, std::begin(data), std::end(data));

        REQUIRE(buffer == load_data("repeated_packed_double/data-one"));
    }

    SECTION("many") {
        const std::array<double, 5> data = {{ 17.34, 0.0, 1.0,
                                              std::numeric_limits<double>::min(),
                                              std::numeric_limits<double>::max() }};
        pw.add_packed_double(1, std::begin(data), std::end(data));

        REQUIRE(buffer == load_data("repeated_packed_double/data-many"));
    }
}


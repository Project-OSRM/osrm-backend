// NOLINT(llvm-header-guard)

#include <array>
#include <sstream>

#define PBF_TYPE_NAME PROTOZERO_TEST_STRING(PBF_TYPE)
#define GET_TYPE PROTOZERO_TEST_CONCAT(get_packed_, PBF_TYPE)
#define ADD_TYPE PROTOZERO_TEST_CONCAT(add_packed_, PBF_TYPE)

using packed_field_type = PROTOZERO_TEST_CONCAT(protozero::packed_field_, PBF_TYPE);

TEST_CASE("read repeated packed field: " PBF_TYPE_NAME) {
    // Run these tests twice, the second time we basically move the data
    // one byte down in the buffer. It doesn't matter how the data or buffer
    // is aligned before that, in at least one of these cases the ints will
    // not be aligned properly. So we test that even in that case the ints
    // will be extracted properly.

    for (std::string::size_type n = 0; n < 2; ++n) {
        std::string abuffer;
        abuffer.reserve(1000);
        abuffer.append(n, '\0');

        SECTION("empty") {
            abuffer.append(load_data("repeated_packed_" PBF_TYPE_NAME "/data-empty"));

            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE_FALSE(item.next());
        }

        SECTION("one") {
            abuffer.append(load_data("repeated_packed_" PBF_TYPE_NAME "/data-one"));

            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE(item.next());
            const auto it_range = item.GET_TYPE();
            REQUIRE_FALSE(item.next());

            REQUIRE(it_range.begin() != it_range.end());
            REQUIRE(*it_range.begin() == 17);
            REQUIRE(std::next(it_range.begin()) == it_range.end());
        }

        SECTION("many") {
            abuffer.append(load_data("repeated_packed_" PBF_TYPE_NAME "/data-many"));

            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE(item.next());
            const auto it_range = item.GET_TYPE();
            REQUIRE_FALSE(item.next());

            auto it = it_range.begin();
            REQUIRE(it != it_range.end());
            REQUIRE(*it++ ==   17);
            REQUIRE(*it++ ==  200);
            REQUIRE(*it++ ==    0);
            REQUIRE(*it++ ==    1);
            REQUIRE(*it++ == std::numeric_limits<cpp_type>::max());
#if PBF_TYPE_IS_SIGNED
            REQUIRE(*it++ == -200);
            REQUIRE(*it++ ==   -1);
            REQUIRE(*it++ == std::numeric_limits<cpp_type>::min());
#endif
            REQUIRE(it == it_range.end());
        }

        SECTION("swap iterator range") {
            abuffer.append(load_data("repeated_packed_" PBF_TYPE_NAME "/data-many"));

            protozero::pbf_reader item{abuffer.data() + n, abuffer.size() - n};

            REQUIRE(item.next());
            auto it_range1 = item.GET_TYPE();
            REQUIRE_FALSE(item.next());

            decltype(it_range1) it_range;
            using std::swap;
            swap(it_range, it_range1);

            auto it = it_range.begin();
            REQUIRE(it != it_range.end());
            REQUIRE(*it++ ==   17);
            REQUIRE(*it++ ==  200);
            REQUIRE(*it++ ==    0);
            REQUIRE(*it++ ==    1);
            REQUIRE(*it++ == std::numeric_limits<cpp_type>::max());
        }

        SECTION("end_of_buffer") {
            abuffer.append(load_data("repeated_packed_" PBF_TYPE_NAME "/data-many"));

            for (std::string::size_type i = 1; i < abuffer.size() - n; ++i) {
                protozero::pbf_reader item{abuffer.data() + n, i};
                REQUIRE(item.next());
                REQUIRE_THROWS_AS(item.GET_TYPE(), protozero::end_of_buffer_exception);
            }
        }

    }

}

TEST_CASE("write repeated packed field: " PBF_TYPE_NAME) {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    SECTION("empty") {
        std::array<cpp_type, 1> data = {{ 17 }};
        pw.ADD_TYPE(1, std::begin(data), std::begin(data) /* !!!! */);

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-empty"));
    }

    SECTION("one") {
        std::array<cpp_type, 1> data = {{ 17 }};
        pw.ADD_TYPE(1, std::begin(data), std::end(data));

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-one"));
    }

    SECTION("many") {
        std::array<cpp_type,
#if PBF_TYPE_IS_SIGNED
                   8
#else
                   5
#endif
        > data = {{
               17
            , 200
            ,   0
            ,   1
            ,std::numeric_limits<cpp_type>::max()
#if PBF_TYPE_IS_SIGNED
            ,-200
            ,  -1
            ,std::numeric_limits<cpp_type>::min()
#endif
        }};
        pw.ADD_TYPE(1, std::begin(data), std::end(data));

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-many"));
    }

}

TEST_CASE("write repeated packed field using packed field: " PBF_TYPE_NAME) {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    SECTION("empty - should do rollback") {
        {
            packed_field_type field{pw, 1};
        }

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-empty"));
    }

    SECTION("one") {
        {
            packed_field_type field{pw, 1};
            field.add_element(cpp_type(17));
        }

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-one"));
    }

    SECTION("many") {
        {
            packed_field_type field{pw, 1};
            field.add_element(cpp_type(  17));
            field.add_element(cpp_type( 200));
            field.add_element(cpp_type(   0));
            field.add_element(cpp_type(   1));
            field.add_element(std::numeric_limits<cpp_type>::max());
#if PBF_TYPE_IS_SIGNED
            field.add_element(cpp_type(-200));
            field.add_element(cpp_type(  -1));
            field.add_element(std::numeric_limits<cpp_type>::min());
#endif
            REQUIRE(field.valid());
            SECTION("with commit") {
                field.commit();
                REQUIRE_FALSE(field.valid());
            }
        }

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-many"));
    }
}

TEST_CASE("move repeated packed field: " PBF_TYPE_NAME) {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    SECTION("move rvalue") {
        packed_field_type field;
        REQUIRE_FALSE(field.valid());
        field = packed_field_type{pw, 1};
        REQUIRE(field.valid());
        field.add_element(cpp_type(17));
    }

    SECTION("explicit move") {
        packed_field_type field2{pw, 1};
        packed_field_type field;

        REQUIRE(field2.valid());
        REQUIRE_FALSE(field.valid());

        field = std::move(field2);

        REQUIRE_FALSE(field2.valid()); // NOLINT(hicpp-invalid-access-moved, bugprone-use-after-move)
        REQUIRE(field.valid());

        field.add_element(cpp_type(17));
    }

    SECTION("move constructor") {
        packed_field_type field2{pw, 1};
        REQUIRE(field2.valid());

        packed_field_type field{std::move(field2)};
        REQUIRE(field.valid());
        REQUIRE_FALSE(field2.valid()); // NOLINT(hicpp-invalid-access-moved, bugprone-use-after-move)

        field.add_element(cpp_type(17));
    }

    SECTION("swap") {
        packed_field_type field;
        packed_field_type field2{pw, 1};

        REQUIRE_FALSE(field.valid());
        REQUIRE(field2.valid());

        using std::swap;
        swap(field, field2);

        REQUIRE(field.valid());
        REQUIRE_FALSE(field2.valid());

        field.add_element(cpp_type(17));
    }

    REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-one"));
}

TEST_CASE("write from different types of iterators: " PBF_TYPE_NAME) {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    SECTION("from uint16_t") {
#if PBF_TYPE_IS_SIGNED
        const std::array< int16_t, 5> data = {{ 1, 4, 9, 16, 25 }};
#else
        const std::array<uint16_t, 5> data = {{ 1, 4, 9, 16, 25 }};
#endif

        pw.ADD_TYPE(1, std::begin(data), std::end(data));
    }

    SECTION("from string") {
        std::string data{"1 4 9 16 25"};
        std::stringstream sdata{data};

#if PBF_TYPE_IS_SIGNED
        using test_type =  int32_t;
#else
        using test_type = uint32_t;
#endif

        std::istream_iterator<test_type> eod;
        std::istream_iterator<test_type> it(sdata);

        pw.ADD_TYPE(1, it, eod);
    }

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    auto it_range = item.GET_TYPE();
    REQUIRE_FALSE(item.next());
    REQUIRE_FALSE(it_range.empty());
    REQUIRE(std::distance(it_range.begin(), it_range.end()) == 5);
    REQUIRE(it_range.size() == 5);

    REQUIRE(it_range.front() ==  1); it_range.drop_front();
    REQUIRE(it_range.front() ==  4); it_range.drop_front();
    REQUIRE(std::distance(it_range.begin(), it_range.end()) == 3);
    REQUIRE(it_range.size() == 3);
    REQUIRE(it_range.front() ==  9); it_range.drop_front();
    REQUIRE(it_range.front() == 16); it_range.drop_front();
    REQUIRE(it_range.front() == 25); it_range.drop_front();
    REQUIRE(it_range.empty());
    REQUIRE(std::distance(it_range.begin(), it_range.end()) == 0);
    REQUIRE(it_range.size() == 0); // NOLINT(readability-container-size-empty)

    REQUIRE_THROWS_AS(it_range.front(), assert_error);
    REQUIRE_THROWS_AS(it_range.drop_front(), assert_error);
}


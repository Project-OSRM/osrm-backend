
#include <test.hpp>

TEST_CASE("rollback when using packed_field functions") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    pw.add_fixed64(2, 111);
    pw.add_string(3, "foo");

    SECTION("empty - should do rollback") {
        {
            const protozero::packed_field_sint64 field{pw, 1};
        }

        pw.add_int32(4, 123);

        protozero::pbf_reader msg{buffer};

        msg.next();
        REQUIRE(msg.tag() == 2);
        REQUIRE(msg.get_fixed64() == 111);

        msg.next();
        REQUIRE(msg.tag() == 3);
        REQUIRE(msg.get_string() == "foo");

        msg.next();
        REQUIRE(msg.tag() == 4);
        REQUIRE(msg.get_int32() == 123);
    }

    SECTION("one") {
        {
            protozero::packed_field_sint64 field{pw, 1};
            field.add_element(17L);
        }

        pw.add_int32(4, 123);

        protozero::pbf_reader msg{buffer};

        msg.next();
        REQUIRE(msg.tag() == 2);
        REQUIRE(msg.get_fixed64() == 111);

        msg.next();
        REQUIRE(msg.tag() == 3);
        REQUIRE(msg.get_string() == "foo");

        msg.next();
        REQUIRE(msg.tag() == 1);
        const auto it_range = msg.get_packed_sint64();
        auto it = it_range.begin();
        REQUIRE(*it++ == 17L);
        REQUIRE(it == it_range.end());

        msg.next();
        REQUIRE(msg.tag() == 4);
        REQUIRE(msg.get_int32() == 123);
    }

    SECTION("many") {
        {
            protozero::packed_field_sint64 field{pw, 1};
            field.add_element(17L);
            field.add_element( 0L);
            field.add_element( 1L);
            field.add_element(-1L);
            field.add_element(std::numeric_limits<int64_t>::max());
            field.add_element(std::numeric_limits<int64_t>::min());
        }

        pw.add_int32(4, 123);

        protozero::pbf_reader msg{buffer};

        msg.next();
        REQUIRE(msg.tag() == 2);
        REQUIRE(msg.get_fixed64() == 111);

        msg.next();
        REQUIRE(msg.tag() == 3);
        REQUIRE(msg.get_string() == "foo");

        msg.next();
        REQUIRE(msg.tag() == 1);
        const auto it_range = msg.get_packed_sint64();
        auto it = it_range.begin();
        REQUIRE(*it++ == 17L);
        REQUIRE(*it++ ==  0L);
        REQUIRE(*it++ ==  1L);
        REQUIRE(*it++ == -1L);
        REQUIRE(*it++ == std::numeric_limits<int64_t>::max());
        REQUIRE(*it++ == std::numeric_limits<int64_t>::min());
        REQUIRE(it == it_range.end());

        msg.next();
        REQUIRE(msg.tag() == 4);
        REQUIRE(msg.get_int32() == 123);
    }

    SECTION("manual rollback") {
        {
            protozero::packed_field_sint64 field{pw, 1};
            field.add_element(17L);
            field.add_element( 0L);
            field.add_element( 1L);
            field.rollback();
        }

        pw.add_int32(4, 123);

        protozero::pbf_reader msg{buffer};

        msg.next();
        REQUIRE(msg.tag() == 2);
        REQUIRE(msg.get_fixed64() == 111);

        msg.next();
        REQUIRE(msg.tag() == 3);
        REQUIRE(msg.get_string() == "foo");

        msg.next();
        REQUIRE(msg.tag() == 4);
        REQUIRE(msg.get_int32() == 123);
    }

    SECTION("manual rollback") {
        {
            protozero::packed_field_sint64 field{pw, 1};
            field.add_element(1L);
            field.rollback();
            REQUIRE_THROWS_AS(field.add_element(1L), assert_error);
        }
    }
}

TEST_CASE("rollback when using submessages") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    pw.add_fixed64(2, 111);
    pw.add_string(3, "foo");

    {
        protozero::pbf_writer pws{pw, 1};
        pws.add_string(1, "foobar");
        pws.rollback();
    }

    pw.add_int32(4, 123);

    protozero::pbf_reader msg{buffer};

    msg.next();
    REQUIRE(msg.tag() == 2);
    REQUIRE(msg.get_fixed64() == 111);

    msg.next();
    REQUIRE(msg.tag() == 3);
    REQUIRE(msg.get_string() == "foo");

    msg.next();
    REQUIRE(msg.tag() == 4);
    REQUIRE(msg.get_int32() == 123);

}

TEST_CASE("rollback on parent message is never allowed") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};
    REQUIRE_THROWS_AS(pw.rollback(), assert_error);
}

TEST_CASE("rollback on parent message is not allowed even if there is a submessage") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    pw.add_fixed64(2, 111);
    pw.add_string(3, "foo");

    {
        protozero::pbf_writer pws{pw, 1};
        pws.add_string(1, "foobar");
        REQUIRE_THROWS_AS(pw.rollback(), assert_error);
    }
}

TEST_CASE("rollback on message is not allowed if there is a nested submessage") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    pw.add_fixed64(2, 111);
    pw.add_string(3, "foo");

    {
        protozero::pbf_writer pws{pw, 1};
        pws.add_string(1, "foobar");
        const protozero::pbf_writer pws2{pws, 1};
        REQUIRE_THROWS_AS(pws.rollback(), assert_error);
    }
}


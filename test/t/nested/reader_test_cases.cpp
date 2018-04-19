
#include <test.hpp>

inline void check_subsub(protozero::pbf_reader message) {
    while (message.next()) {
        switch (message.tag()) {
            case 1: {
                REQUIRE("foobar" == message.get_string());
                break;
            }
            case 2: {
                REQUIRE(99 == message.get_int32());
                break;
            }
            default: {
                REQUIRE(false); // should never be here
                break;
            }
        }
    }
}

inline void check_sub(protozero::pbf_reader message) {
    while (message.next()) {
        switch (message.tag()) {
            case 1: {
                check_subsub(message.get_message());
                break;
            }
            case 2: {
                REQUIRE(88 == message.get_int32());
                break;
            }
            default: {
                REQUIRE(false); // should never be here
                break;
            }
        }
    }
}

inline void check(protozero::pbf_reader message) {
    while (message.next()) {
        switch (message.tag()) {
            case 1: {
                check_sub(message.get_message());
                break;
            }
            case 2: {
                REQUIRE(77 == message.get_int32());
                break;
            }
            default: {
                REQUIRE(false); // should never be here
                break;
            }
        }
    }
}

inline void check_empty(protozero::pbf_reader message) {
    while (message.next()) {
        switch (message.tag()) {
            case 1: {
                REQUIRE_FALSE(message.get_message().next());
                break;
            }
            case 2: {
                REQUIRE(77 == message.get_int32());
                break;
            }
            default: {
                REQUIRE(false); // should never be here
                break;
            }
        }
    }
}

TEST_CASE("read nested message fields: string") {
    const std::string buffer = load_data("nested/data-message");

    protozero::pbf_reader message{buffer};
    check(message);
}

TEST_CASE("read nested message fields: no submessage") {
    const std::string buffer = load_data("nested/data-no-message");

    protozero::pbf_reader message{buffer};
    check_empty(message);
}

TEST_CASE("write nested message fields") {
    std::string buffer_test;
    protozero::pbf_writer pbf_test{buffer_test};

    SECTION("string") {
        std::string buffer_subsub;
        protozero::pbf_writer pbf_subsub{buffer_subsub};
        pbf_subsub.add_string(1, "foobar");
        pbf_subsub.add_int32(2, 99);

        std::string buffer_sub;
        protozero::pbf_writer pbf_sub{buffer_sub};
        pbf_sub.add_string(1, buffer_subsub);
        pbf_sub.add_int32(2, 88);

        pbf_test.add_message(1, buffer_sub);
    }

    SECTION("string with subwriter") {
        protozero::pbf_writer pbf_sub{pbf_test, 1};
        {
            protozero::pbf_writer pbf_subsub{pbf_sub, 1};
            pbf_subsub.add_string(1, "foobar");
            pbf_subsub.add_int32(2, 99);
        }
        pbf_sub.add_int32(2, 88);
    }

    pbf_test.add_int32(2, 77);

    protozero::pbf_reader message{buffer_test};
    check(message);
}

TEST_CASE("write nested message fields - no message") {
    std::string buffer_test;
    protozero::pbf_writer pbf_test{buffer_test};

    SECTION("nothing") {
    }

    SECTION("empty string") {
        std::string buffer_sub;

        pbf_test.add_message(1, buffer_sub);
    }

    SECTION("string with pbf_writer") {
        std::string buffer_sub;
        protozero::pbf_writer pbf_sub{buffer_sub};

        pbf_test.add_message(1, buffer_sub);
    }

    SECTION("string with subwriter") {
        protozero::pbf_writer pbf_sub{pbf_test, 1};
    }

    pbf_test.add_int32(2, 77);

    protozero::pbf_reader message{buffer_test};
    check_empty(message);
}


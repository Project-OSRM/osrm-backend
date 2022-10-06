
#include <test.hpp>

TEST_CASE("read bytes field: empty") {
    const std::string buffer = load_data("bytes/data-empty");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE(item.get_bytes().empty());
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read bytes field: one") {
    const std::string buffer = load_data("bytes/data-one");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE(item.get_bytes() == "x");
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read bytes field: string") {
    const std::string buffer = load_data("bytes/data-string");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    REQUIRE(item.get_bytes() == "foobar");
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read bytes field: binary") {
    const std::string buffer = load_data("bytes/data-binary");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    const std::string data = item.get_bytes();
    REQUIRE(data.size() == 3);
    REQUIRE(data[0] == char(1));
    REQUIRE(data[1] == char(2));
    REQUIRE(data[2] == char(3));
    REQUIRE_FALSE(item.next());
}

TEST_CASE("read bytes field: end of buffer") {
    const std::string buffer = load_data("bytes/data-binary");

    for (std::string::size_type i = 1; i < buffer.size(); ++i) {
        protozero::pbf_reader item{buffer.data(), i};
        REQUIRE(item.next());
        REQUIRE_THROWS_AS(item.get_bytes(), protozero::end_of_buffer_exception);
    }
}

TEST_CASE("write bytes field") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    SECTION("empty") {
        pw.add_string(1, "");
        REQUIRE(buffer == load_data("bytes/data-empty"));
    }

    SECTION("one") {
        pw.add_string(1, "x");
        REQUIRE(buffer == load_data("bytes/data-one"));
    }

    SECTION("string") {
        pw.add_string(1, "foobar");
        REQUIRE(buffer == load_data("bytes/data-string"));
    }

    SECTION("binary") {
        std::string data;
        data.append(1, char(1));
        data.append(1, char(2));
        data.append(1, char(3));

        pw.add_string(1, data);

        REQUIRE(buffer == load_data("bytes/data-binary"));
    }
}

TEST_CASE("write bytes field using vectored approach") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    SECTION("using two strings") {
        std::string d1{"foo"};
        std::string d2{"bar"};

        pw.add_bytes_vectored(1, d1, d2);
    }

    SECTION("using a string and a dataview") {
        std::string d1{"foo"};
        std::string d2{"bar"};
        protozero::data_view dv{d2};

        pw.add_bytes_vectored(1, d1, dv);
    }

    SECTION("using three strings") {
        std::string d1{"foo"};
        std::string d2{"ba"};
        std::string d3{"r"};

        pw.add_bytes_vectored(1, d1, d2, d3);
    }

    SECTION("with empty string") {
        std::string d1{"foo"};
        std::string d2{};
        std::string d3{"bar"};

        pw.add_bytes_vectored(1, d1, d2, d3);
    }

    REQUIRE(buffer == load_data("bytes/data-string"));
}

TEST_CASE("write bytes field using vectored approach with builder") {
    enum class foo : protozero::pbf_tag_type { bar = 1 };
    std::string buffer;
    protozero::pbf_builder<foo> pw{buffer};

    const std::string d1{"foo"};
    const std::string d2{"bar"};

    pw.add_bytes_vectored(foo::bar, d1, d2);

    REQUIRE(buffer == load_data("bytes/data-string"));
}


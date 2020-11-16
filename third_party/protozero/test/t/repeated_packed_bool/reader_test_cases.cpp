
#include <test.hpp>

#include <array>

TEST_CASE("read repeated packed bool field: empty") {
    const std::string buffer = load_data("repeated_packed_bool/data-empty");

    protozero::pbf_reader item{buffer};

    REQUIRE_FALSE(item.next());
}

TEST_CASE("read repeated packed bool field: one") {
    const std::string buffer = load_data("repeated_packed_bool/data-one");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    const auto it_range = item.get_packed_bool();
    REQUIRE(std::distance(it_range.begin(), it_range.end()) == 1);
    REQUIRE(it_range.size() == 1);
    REQUIRE_FALSE(item.next());

    REQUIRE(it_range.begin() != it_range.end());
    REQUIRE(*it_range.begin());
    REQUIRE(std::next(it_range.begin()) == it_range.end());
}

TEST_CASE("read repeated packed bool field: many") {
    const std::string buffer = load_data("repeated_packed_bool/data-many");

    protozero::pbf_reader item{buffer};

    REQUIRE(item.next());
    const auto it_range = item.get_packed_bool();
    REQUIRE(std::distance(it_range.begin(), it_range.end()) == 4);
    REQUIRE(it_range.size() == 4);
    REQUIRE_FALSE(item.next());

    auto it = it_range.begin();
    REQUIRE(it != it_range.end());
    REQUIRE(*it++);
    REQUIRE(*it++);
    REQUIRE_FALSE(*it++);
    REQUIRE(*it++);
    REQUIRE(it == it_range.end());
}

TEST_CASE("read repeated packed bool field: end of buffer") {
    const std::string buffer = load_data("repeated_packed_bool/data-many");

    for (std::string::size_type i = 1; i < buffer.size(); ++i) {
        protozero::pbf_reader item{buffer.data(), i};
        REQUIRE(item.next());
        REQUIRE_THROWS_AS(item.get_packed_bool(), protozero::end_of_buffer_exception);
    }
}

TEST_CASE("write repeated packed bool field") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    SECTION("empty") {
        const std::array<bool, 1> data = {{ true }};
        pw.add_packed_bool(1, std::begin(data), std::begin(data) /* !!!! */);

        REQUIRE(buffer == load_data("repeated_packed_bool/data-empty"));
    }

    SECTION("one") {
        const std::array<bool, 1> data = {{ true }};
        pw.add_packed_bool(1, std::begin(data), std::end(data));

        REQUIRE(buffer == load_data("repeated_packed_bool/data-one"));
    }

    SECTION("many") {
        const std::array<bool, 4> data = {{ true, true, false, true }};
        pw.add_packed_bool(1, std::begin(data), std::end(data));

        REQUIRE(buffer == load_data("repeated_packed_bool/data-many"));
    }

}

TEST_CASE("write repeated packed bool field using packed_field_bool") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    SECTION("empty - should do rollback") {
        {
            protozero::packed_field_bool field{pw, 1};
        }

        REQUIRE(buffer == load_data("repeated_packed_bool/data-empty"));
    }

    SECTION("one") {
        {
            protozero::packed_field_bool field{pw, 1};
            field.add_element(true);
        }

        REQUIRE(buffer == load_data("repeated_packed_bool/data-one"));
    }

    SECTION("many") {
        {
            protozero::packed_field_bool field{pw, 1};
            field.add_element(true);
            field.add_element(true);
            field.add_element(false);
            field.add_element(true);
        }

        REQUIRE(buffer == load_data("repeated_packed_bool/data-many"));
    }
}

TEST_CASE("write repeated packed bool field using packed_field_bool with pbf_builder") {
    enum class msg : protozero::pbf_tag_type {
        f = 1
    };

    std::string buffer;
    protozero::pbf_builder<msg> pw{buffer};

    SECTION("empty - should do rollback") {
        {
            protozero::packed_field_bool field{pw, msg::f};
        }

        REQUIRE(buffer == load_data("repeated_packed_bool/data-empty"));
    }

    SECTION("one") {
        {
            protozero::packed_field_bool field{pw, msg::f};
            field.add_element(true);
        }

        REQUIRE(buffer == load_data("repeated_packed_bool/data-one"));
    }

    SECTION("many") {
        {
            protozero::packed_field_bool field{pw, msg::f};
            field.add_element(true);
            field.add_element(true);
            field.add_element(false);
            field.add_element(true);
        }

        REQUIRE(buffer == load_data("repeated_packed_bool/data-many"));
    }
}



#include <cstdint>
#include <string>
#include <vector>

#include <test.hpp>

enum class ExampleMsg : protozero::pbf_tag_type {
    repeated_uint32_x = 1
};

inline std::vector<uint32_t> read_data(const std::string& data) {
    std::vector<uint32_t> values;

    protozero::pbf_message<ExampleMsg> message{data};
    while (message.next()) {
        switch (message.tag_and_type()) {
            case tag_and_type(ExampleMsg::repeated_uint32_x, protozero::pbf_wire_type::length_delimited): {
                    const auto xit = message.get_packed_uint32();
                    for (const auto value : xit) {
                        values.push_back(value);
                    }
                }
                break;
            case tag_and_type(ExampleMsg::repeated_uint32_x, protozero::pbf_wire_type::varint): {
                    const auto value = message.get_uint32();
                    values.push_back(value);
                }
                break;
            default:
                message.skip();
        }
    }

    return values;
}

inline std::vector<uint32_t> read_data_packed(const std::string& data) {
    std::vector<uint32_t> values;

    protozero::pbf_message<ExampleMsg> message{data};
    while (message.next(ExampleMsg::repeated_uint32_x, protozero::pbf_wire_type::length_delimited)) {
        const auto xit = message.get_packed_uint32();
        for (const auto value : xit) {
            values.push_back(value);
        }
    }

    return values;
}

TEST_CASE("read not packed repeated field with tag_and_type") {
    const auto values = read_data(load_data("tag_and_type/data-not-packed"));

    REQUIRE(values.size() == 3);
    const std::vector<uint32_t> result{10, 11, 12};
    REQUIRE(values == result);
}

TEST_CASE("read not packed repeated field with tag_and_type using next(...)") {
    const auto values = read_data_packed(load_data("tag_and_type/data-not-packed"));

    REQUIRE(values.empty());
}

TEST_CASE("read packed repeated field with tag_and_type") {
    const auto values = read_data(load_data("tag_and_type/data-packed"));

    REQUIRE(values.size() == 3);
    const std::vector<uint32_t> result{20, 21, 22};
    REQUIRE(values == result);
}

TEST_CASE("read packed repeated field with tag_and_type using next(...)") {
    const auto values = read_data_packed(load_data("tag_and_type/data-packed"));

    REQUIRE(values.size() == 3);
    const std::vector<uint32_t> result{20, 21, 22};
    REQUIRE(values == result);
}

TEST_CASE("read combined packed and not-packed repeated field with tag_and_type") {
    const auto values = read_data(load_data("tag_and_type/data-combined"));

    REQUIRE(values.size() == 9);
    const std::vector<uint32_t> result{10, 11, 12, 20, 21, 22, 30, 31, 32};
    REQUIRE(values == result);
}


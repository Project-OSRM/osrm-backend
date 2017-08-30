
#include <cmath>
#include <stdexcept>

#include "common.hpp"

class TestHandler101 : public osmium::handler::Handler {

public:

    TestHandler101() :
        osmium::handler::Handler() {
    }

    void node(const osmium::Node& node) {
        constexpr const double epsilon = 0.00000001;
        if (node.id() == 101000) {
            REQUIRE(node.version() == 1);
            REQUIRE(std::abs(node.location().lon() - 1.12) < epsilon);
            REQUIRE(std::abs(node.location().lat() - 1.02) < epsilon);
        } else if (node.id() == 101001) {
            REQUIRE(node.version() == 1);
            REQUIRE(std::abs(node.location().lon() - 1.12) < epsilon);
            REQUIRE(std::abs(node.location().lat() - 1.03) < epsilon);
        } else if (node.id() == 101002) {
        } else if (node.id() == 101003) {
        } else {
            throw std::runtime_error{"Unknown ID"};
        }
    }

}; // class TestHandler101

TEST_CASE("101") {

    SECTION("test 101") {
        osmium::io::Reader reader{dirname + "/1/101/data.osm"};

        CheckBasicsHandler check_basics_handler{101, 4, 0, 0};
        CheckWKTHandler check_wkt_handler{dirname, 101};
        TestHandler101 test_handler;

        osmium::apply(reader, check_basics_handler, check_wkt_handler, test_handler);
    }

}



#include <stdexcept>

#include "common.hpp"

class TestHandler102 : public osmium::handler::Handler {

    osmium::Location location;

public:

    TestHandler102() :
        osmium::handler::Handler() {
    }

    void node(const osmium::Node& node) {
        constexpr const double epsilon = 0.00000001;
        if (node.id() == 102000) {
            REQUIRE(node.version() == 1);
            REQUIRE(node.location().lon() == Approx(1.24).epsilon(epsilon));
            REQUIRE(node.location().lat() == Approx(1.02).epsilon(epsilon));
            location = node.location();
        } else if (node.id() == 102001) {
            REQUIRE(node.version() == 1);
            REQUIRE(node.location().lon() == Approx(1.24).epsilon(epsilon));
            REQUIRE(node.location().lat() == Approx(1.02).epsilon(epsilon));
            REQUIRE(node.location() == location);
        } else {
            throw std::runtime_error{"Unknown ID"};
        }
    }

}; // class TestHandler102

TEST_CASE("102") {
    osmium::io::Reader reader{dirname + "/1/102/data.osm"};

    CheckBasicsHandler check_basics_handler{102, 2, 0, 0};
    CheckWKTHandler check_wkt_handler{dirname, 102};
    TestHandler102 test_handler;

    osmium::apply(reader, check_basics_handler, check_wkt_handler, test_handler);
}


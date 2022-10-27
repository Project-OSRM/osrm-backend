
#include <cstring>
#include <stdexcept>

#include "common.hpp"

class TestHandler113 : public osmium::handler::Handler {

public:

    TestHandler113() :
        osmium::handler::Handler() {
    }

    void node(const osmium::Node& node) const {
        constexpr const double epsilon = 0.00000001;
        if (node.id() == 113000) {
            REQUIRE(node.location().lon() == Approx(1.32).epsilon(epsilon));
            REQUIRE(node.location().lat() == Approx(1.12).epsilon(epsilon));
        } else if (node.id() == 113001) {
            REQUIRE(node.location().lon() == Approx(1.34).epsilon(epsilon));
            REQUIRE(node.location().lat() == Approx(1.13).epsilon(epsilon));
        } else if (node.id() == 113002) {
            REQUIRE(node.location().lon() == Approx(1.37).epsilon(epsilon));
            REQUIRE(node.location().lat() == Approx(1.12).epsilon(epsilon));
        } else if (node.id() == 113003) {
            REQUIRE(node.location().lon() == Approx(1.38).epsilon(epsilon));
            REQUIRE(node.location().lat() == Approx(1.18).epsilon(epsilon));
        } else {
            throw std::runtime_error{"Unknown ID"};
        }
    }

    void way(const osmium::Way& way) const {
        if (way.id() == 113800) {
            REQUIRE(way.version() == 1);
            REQUIRE(way.nodes().size() == 2);
            REQUIRE_FALSE(way.is_closed());
            REQUIRE(way.nodes()[0].ref() == 113000);
            REQUIRE(way.nodes()[1].ref() == 113001);
        } else if (way.id() == 113801) {
            REQUIRE(way.version() == 1);
            REQUIRE(way.nodes().size() == 2);
            REQUIRE_FALSE(way.is_closed());
            REQUIRE(way.nodes()[0].ref() == 113002);
            REQUIRE(way.nodes()[1].ref() == 113003);
        } else {
            throw std::runtime_error{"Unknown ID"};
        }
    }

}; // class TestHandler113

TEST_CASE("113") {
    osmium::io::Reader reader{dirname + "/1/113/data.osm"};

    index_pos_type index_pos;
    index_neg_type index_neg;
    location_handler_type location_handler{index_pos, index_neg};
    location_handler.ignore_errors();

    CheckBasicsHandler check_basics_handler{113, 4, 2, 0};
    CheckWKTHandler check_wkt_handler{dirname, 113};
    TestHandler113 test_handler;

    osmium::apply(reader, location_handler, check_basics_handler, check_wkt_handler, test_handler);
}


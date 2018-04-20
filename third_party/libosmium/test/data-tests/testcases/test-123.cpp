
#include <cstring>
#include <stdexcept>

#include "common.hpp"

class TestHandler123 : public osmium::handler::Handler {

public:

    TestHandler123() :
        osmium::handler::Handler() {
    }

    void way(const osmium::Way& way) const {
        if (way.id() == 123800) {
            REQUIRE(way.version() == 1);
            REQUIRE(way.nodes().size() == 2);
            REQUIRE(way.nodes()[0] != way.nodes()[1]);
            REQUIRE(way.nodes()[0].location() == way.nodes()[1].location());
        } else {
            throw std::runtime_error{"Unknown ID"};
        }
    }

}; // class TestHandler123

TEST_CASE("123") {
    osmium::io::Reader reader{dirname + "/1/123/data.osm"};

    index_pos_type index_pos;
    index_neg_type index_neg;
    location_handler_type location_handler{index_pos, index_neg};
    location_handler.ignore_errors();

    CheckBasicsHandler check_basics_handler{123, 2, 1, 0};
    CheckWKTHandler check_wkt_handler{dirname, 123};
    TestHandler123 test_handler;

    osmium::apply(reader, location_handler, check_basics_handler, check_wkt_handler, test_handler);
}


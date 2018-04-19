
#include <cstring>
#include <stdexcept>

#include "common.hpp"

class TestHandler124 : public osmium::handler::Handler {

public:

    TestHandler124() :
        osmium::handler::Handler() {
    }

    void way(const osmium::Way& way) const {
        if (way.id() == 124800) {
            REQUIRE(way.version() == 1);
            REQUIRE(way.nodes().size() == 3);
            REQUIRE(way.nodes()[0] != way.nodes()[1]);
            REQUIRE(way.nodes()[0].location() == way.nodes()[1].location());
            REQUIRE(way.nodes()[0] != way.nodes()[2]);
            REQUIRE(way.nodes()[0].location() != way.nodes()[2].location());
        } else {
            throw std::runtime_error{"Unknown ID"};
        }
    }

}; // class TestHandler124

TEST_CASE("124") {
    osmium::io::Reader reader{dirname + "/1/124/data.osm"};

    index_pos_type index_pos;
    index_neg_type index_neg;
    location_handler_type location_handler{index_pos, index_neg};
    location_handler.ignore_errors();

    CheckBasicsHandler check_basics_handler{124, 3, 1, 0};
    CheckWKTHandler check_wkt_handler{dirname, 124};
    TestHandler124 test_handler;

    osmium::apply(reader, location_handler, check_basics_handler, check_wkt_handler, test_handler);
}



#include <cstring>
#include <stdexcept>

#include "common.hpp"

class TestHandler116 : public osmium::handler::Handler {

public:

    TestHandler116() :
        osmium::handler::Handler() {
    }

    void way(const osmium::Way& way) const {
        if (way.id() == 116800) {
            REQUIRE(way.version() == 1);
            REQUIRE(way.nodes().size() == 2);
            REQUIRE_FALSE(way.is_closed());
            REQUIRE(way.nodes()[0].ref() == 116000);
            REQUIRE(way.nodes()[1].ref() == 116001);
        } else if (way.id() == 116801) {
            REQUIRE(way.nodes().size() == 2);
        } else if (way.id() == 116802) {
            REQUIRE(way.nodes().size() == 4);
        } else {
            throw std::runtime_error{"Unknown ID"};
        }
    }

}; // class TestHandler116

TEST_CASE("116") {
    osmium::io::Reader reader{dirname + "/1/116/data.osm"};

    index_pos_type index_pos;
    index_neg_type index_neg;
    location_handler_type location_handler{index_pos, index_neg};
    location_handler.ignore_errors();

    CheckBasicsHandler check_basics_handler{116, 5, 3, 0};
    CheckWKTHandler check_wkt_handler{dirname, 116};
    TestHandler116 test_handler;

    osmium::apply(reader, location_handler, check_basics_handler, check_wkt_handler, test_handler);
}


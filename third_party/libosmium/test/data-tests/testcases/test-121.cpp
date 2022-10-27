
#include <cstring>
#include <stdexcept>

#include "common.hpp"

class TestHandler121 : public osmium::handler::Handler {

public:

    TestHandler121() :
        osmium::handler::Handler() {
    }

    void way(const osmium::Way& way) const {
        if (way.id() == 121800) {
            REQUIRE(way.version() == 1);
            REQUIRE(way.nodes().size() == 1);
        } else {
            throw std::runtime_error{"Unknown ID"};
        }
    }

}; // class TestHandler121

TEST_CASE("121") {
    osmium::io::Reader reader{dirname + "/1/121/data.osm"};

    index_pos_type index_pos;
    index_neg_type index_neg;
    location_handler_type location_handler{index_pos, index_neg};
    location_handler.ignore_errors();

    CheckBasicsHandler check_basics_handler{121, 1, 1, 0};
    CheckWKTHandler check_wkt_handler{dirname, 121};
    TestHandler121 test_handler;

    osmium::apply(reader, location_handler, check_basics_handler, check_wkt_handler, test_handler);
}



#include <cstring>
#include <stdexcept>

#include "common.hpp"

class TestHandler122 : public osmium::handler::Handler {

public:

    TestHandler122() :
        osmium::handler::Handler() {
    }

    void way(const osmium::Way& way) const {
        if (way.id() == 122800) {
            REQUIRE(way.version() == 1);
            REQUIRE(way.nodes().size() == 2);
            REQUIRE(way.nodes()[0] == way.nodes()[1]);
        } else {
            throw std::runtime_error{"Unknown ID"};
        }
    }

}; // class TestHandler122

TEST_CASE("122") {
    osmium::io::Reader reader{dirname + "/1/122/data.osm"};

    index_pos_type index_pos;
    index_neg_type index_neg;
    location_handler_type location_handler{index_pos, index_neg};
    location_handler.ignore_errors();

    CheckBasicsHandler check_basics_handler{122, 1, 1, 0};
    CheckWKTHandler check_wkt_handler{dirname, 122};
    TestHandler122 test_handler;

    osmium::apply(reader, location_handler, check_basics_handler, check_wkt_handler, test_handler);
}



#include <cstring>
#include <stdexcept>

#include "common.hpp"

class TestHandler115 : public osmium::handler::Handler {

public:

    TestHandler115() :
        osmium::handler::Handler() {
    }

    void way(const osmium::Way& way) const {
        if (way.id() == 115800) {
            REQUIRE(way.version() == 1);
            REQUIRE(way.nodes().size() == 2);
            REQUIRE_FALSE(way.is_closed());
            REQUIRE(way.nodes()[0].ref() == 115000);
            REQUIRE(way.nodes()[1].ref() == 115001);
        } else if (way.id() == 115801) {
            REQUIRE(way.version() == 1);
            REQUIRE(way.nodes().size() == 2);
            REQUIRE_FALSE(way.is_closed());
            REQUIRE(way.nodes()[0].ref() == 115002);
            REQUIRE(way.nodes()[1].ref() == 115001);
        } else {
            throw std::runtime_error{"Unknown ID"};
        }
    }

}; // class TestHandler115

TEST_CASE("115") {
    osmium::io::Reader reader{dirname + "/1/115/data.osm"};

    index_pos_type index_pos;
    index_neg_type index_neg;
    location_handler_type location_handler{index_pos, index_neg};
    location_handler.ignore_errors();

    CheckBasicsHandler check_basics_handler{115, 3, 2, 0};
    CheckWKTHandler check_wkt_handler{dirname, 115};
    TestHandler115 test_handler;

    osmium::apply(reader, location_handler, check_basics_handler, check_wkt_handler, test_handler);
}



#include <cstring>
#include <stdexcept>

#include "common.hpp"

class TestHandler114 : public osmium::handler::Handler {

public:

    TestHandler114() :
        osmium::handler::Handler() {
    }

    void way(const osmium::Way& way) const {
        if (way.id() == 114800) {
            REQUIRE(way.version() == 1);
            REQUIRE(way.nodes().size() == 2);
            REQUIRE_FALSE(way.is_closed());
            REQUIRE(way.nodes()[0].ref() == 114000);
            REQUIRE(way.nodes()[1].ref() == 114001);
        } else if (way.id() == 114801) {
            REQUIRE(way.version() == 1);
            REQUIRE(way.nodes().size() == 2);
            REQUIRE_FALSE(way.is_closed());
            REQUIRE(way.nodes()[0].ref() == 114001);
            REQUIRE(way.nodes()[1].ref() == 114002);
        } else {
            throw std::runtime_error{"Unknown ID"};
        }
    }

}; // class TestHandler114

TEST_CASE("114") {
    osmium::io::Reader reader{dirname + "/1/114/data.osm"};

    index_pos_type index_pos;
    index_neg_type index_neg;
    location_handler_type location_handler{index_pos, index_neg};
    location_handler.ignore_errors();

    CheckBasicsHandler check_basics_handler{114, 3, 2, 0};
    CheckWKTHandler check_wkt_handler{dirname, 114};
    TestHandler114 test_handler;

    osmium::apply(reader, location_handler, check_basics_handler, check_wkt_handler, test_handler);
}


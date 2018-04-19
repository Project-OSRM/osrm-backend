
#include <cstring>
#include <stdexcept>

#include "common.hpp"

class TestHandler120 : public osmium::handler::Handler {

public:

    TestHandler120() :
        osmium::handler::Handler() {
    }

    void way(const osmium::Way& way) const {
        if (way.id() == 120800) {
            REQUIRE(way.version() == 1);
            REQUIRE(way.nodes().empty());
        } else {
            throw std::runtime_error{"Unknown ID"};
        }
    }

}; // class TestHandler120

TEST_CASE("120") {
    osmium::io::Reader reader{dirname + "/1/120/data.osm"};

    index_pos_type index_pos;
    index_neg_type index_neg;
    location_handler_type location_handler{index_pos, index_neg};
    location_handler.ignore_errors();

    CheckBasicsHandler check_basics_handler{120, 0, 1, 0};
    TestHandler120 test_handler;

    osmium::apply(reader, location_handler, check_basics_handler, test_handler);
}


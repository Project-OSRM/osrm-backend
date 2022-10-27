
#include <cstring>
#include <stdexcept>

#include "common.hpp"

class TestHandler112 : public osmium::handler::Handler {

public:

    TestHandler112() :
        osmium::handler::Handler() {
    }

    void way(const osmium::Way& way) const {
        if (way.id() == 112800) {
            REQUIRE(way.version() == 1);
            REQUIRE(way.nodes().size() == 5);
            REQUIRE(way.is_closed());

            const char *test_id = way.tags().get_value_by_key("test:id");
            REQUIRE(test_id);
            REQUIRE(!std::strcmp(test_id, "112"));
        } else {
            throw std::runtime_error{"Unknown ID"};
        }
    }

}; // class TestHandler112

TEST_CASE("112") {
    osmium::io::Reader reader{dirname + "/1/112/data.osm"};

    index_pos_type index_pos;
    index_neg_type index_neg;
    location_handler_type location_handler{index_pos, index_neg};
    location_handler.ignore_errors();

    CheckBasicsHandler check_basics_handler{112, 4, 1, 0};
    CheckWKTHandler check_wkt_handler{dirname, 112};
    TestHandler112 test_handler;

    osmium::apply(reader, location_handler, check_basics_handler, check_wkt_handler, test_handler);
}



#include <cstring>
#include <stdexcept>

#include "common.hpp"

class TestHandler111 : public osmium::handler::Handler {

public:

    TestHandler111() :
        osmium::handler::Handler() {
    }

    void way(const osmium::Way& way) const {
        if (way.id() == 111800) {
            REQUIRE(way.version() == 1);
            REQUIRE(way.nodes().size() == 4);
            REQUIRE_FALSE(way.is_closed());

            const char *test_id = way.tags().get_value_by_key("test:id");
            REQUIRE(test_id);
            REQUIRE(!std::strcmp(test_id, "111"));
        } else {
            throw std::runtime_error{"Unknown ID"};
        }
    }

}; // class TestHandler111

TEST_CASE("111") {
    osmium::io::Reader reader{dirname + "/1/111/data.osm"};

    index_pos_type index_pos;
    index_neg_type index_neg;
    location_handler_type location_handler{index_pos, index_neg};
    location_handler.ignore_errors();

    CheckBasicsHandler check_basics_handler{111, 4, 1, 0};
    CheckWKTHandler check_wkt_handler{dirname, 111};
    TestHandler111 test_handler;

    osmium::apply(reader, location_handler, check_basics_handler, check_wkt_handler, test_handler);
}


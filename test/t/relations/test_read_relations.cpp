#include "catch.hpp"

#include "utils.hpp"

#include <osmium/handler.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/relations/manager_util.hpp>
#include <osmium/util/progress_bar.hpp>

class TestHandler : public osmium::handler::Handler {

public:

    int count = 0;
    bool prep = false;

    void relation(const osmium::Relation& /*relation*/) noexcept {
        ++count;
    }

    void prepare_for_lookup() noexcept {
        prep = true;
    }

}; // class TestHandler

TEST_CASE("Read relations with one handler") {
    osmium::io::File file{with_data_dir("t/relations/data.osm")};

    TestHandler handler;

    osmium::relations::read_relations(file, handler);

    REQUIRE(handler.count == 3);
    REQUIRE(handler.prep);
}

TEST_CASE("Read relations with two handlers") {
    osmium::io::File file{with_data_dir("t/relations/data.osm")};

    TestHandler handler1;
    TestHandler handler2;

    osmium::relations::read_relations(file, handler1, handler2);

    REQUIRE(handler1.count == 3);
    REQUIRE(handler2.count == 3);
    REQUIRE(handler1.prep);
    REQUIRE(handler2.prep);
}

TEST_CASE("Read relations with progress bar and one handler") {
    osmium::io::File file{with_data_dir("t/relations/data.osm")};
    osmium::ProgressBar progress_bar{file.size(), false};

    TestHandler handler;

    osmium::relations::read_relations(progress_bar, file, handler);

    REQUIRE(handler.count == 3);
    REQUIRE(handler.prep);
}

TEST_CASE("Read relations with progress bar and two handlers") {
    osmium::io::File file{with_data_dir("t/relations/data.osm")};
    osmium::ProgressBar progress_bar{file.size(), false};

    TestHandler handler1;
    TestHandler handler2;

    osmium::relations::read_relations(progress_bar, file, handler1, handler2);

    REQUIRE(handler1.count == 3);
    REQUIRE(handler2.count == 3);
    REQUIRE(handler1.prep);
    REQUIRE(handler2.prep);
}


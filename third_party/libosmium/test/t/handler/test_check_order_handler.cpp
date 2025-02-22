#include "catch.hpp"

#include <osmium/handler/check_order.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/opl.hpp>
#include <osmium/visitor.hpp>

TEST_CASE("CheckOrder handler if everything is in order") {
    osmium::memory::Buffer buffer{1024};

    REQUIRE(osmium::opl_parse("n-126", buffer));
    REQUIRE(osmium::opl_parse("n123", buffer));
    REQUIRE(osmium::opl_parse("n124", buffer));
    REQUIRE(osmium::opl_parse("n128", buffer));
    REQUIRE(osmium::opl_parse("w-100", buffer));
    REQUIRE(osmium::opl_parse("w100", buffer));
    REQUIRE(osmium::opl_parse("w102", buffer));
    REQUIRE(osmium::opl_parse("r-200", buffer));
    REQUIRE(osmium::opl_parse("r100", buffer));

    osmium::handler::CheckOrder handler;
    osmium::apply(buffer, handler);
    REQUIRE(handler.max_node_id()     == 128);
    REQUIRE(handler.max_way_id()      == 102);
    REQUIRE(handler.max_relation_id() == 100);
}

TEST_CASE("CheckOrder handler: Nodes must be in order") {
    osmium::memory::Buffer buffer{1024};

    REQUIRE(osmium::opl_parse("n3", buffer));

    SECTION("Positive ID") {
        REQUIRE(osmium::opl_parse("n2", buffer));
    }
    SECTION("Negative ID") {
        REQUIRE(osmium::opl_parse("n-2", buffer));
    }

    osmium::handler::CheckOrder handler;
    REQUIRE_THROWS_AS(osmium::apply(buffer, handler), osmium::out_of_order_error);
}

TEST_CASE("CheckOrder handler: Ways must be in order") {
    osmium::memory::Buffer buffer{1024};

    REQUIRE(osmium::opl_parse("w3", buffer));
    SECTION("Positive ID") {
        REQUIRE(osmium::opl_parse("w2", buffer));
    }
    SECTION("Negative ID") {
        REQUIRE(osmium::opl_parse("w-2", buffer));
    }

    osmium::handler::CheckOrder handler;
    REQUIRE_THROWS_AS(osmium::apply(buffer, handler), osmium::out_of_order_error);
}

TEST_CASE("CheckOrder handler: Relations must be in order") {
    osmium::memory::Buffer buffer{1024};

    REQUIRE(osmium::opl_parse("r3", buffer));
    SECTION("Positive ID") {
        REQUIRE(osmium::opl_parse("r2", buffer));
    }
    SECTION("Negative ID") {
        REQUIRE(osmium::opl_parse("r-2", buffer));
    }

    osmium::handler::CheckOrder handler;
    REQUIRE_THROWS_AS(osmium::apply(buffer, handler), osmium::out_of_order_error);
}

TEST_CASE("CheckOrder handler: Same id twice is not allowed") {
    osmium::memory::Buffer buffer{1024};

    REQUIRE(osmium::opl_parse("n3", buffer));
    REQUIRE(osmium::opl_parse("n3", buffer));

    osmium::handler::CheckOrder handler;
    REQUIRE_THROWS_AS(osmium::apply(buffer, handler), osmium::out_of_order_error);
}

TEST_CASE("CheckOrder handler: Nodes after ways") {
    osmium::memory::Buffer buffer{1024};

    REQUIRE(osmium::opl_parse("w50", buffer));
    SECTION("Positive ID") {
        REQUIRE(osmium::opl_parse("n30", buffer));
    }
    SECTION("Negative ID") {
        REQUIRE(osmium::opl_parse("n-30", buffer));
    }

    osmium::handler::CheckOrder handler;
    REQUIRE_THROWS_AS(osmium::apply(buffer, handler), osmium::out_of_order_error);
}

TEST_CASE("CheckOrder handler: Nodes after relations") {
    osmium::memory::Buffer buffer{1024};

    REQUIRE(osmium::opl_parse("r50", buffer));
    SECTION("Positive ID") {
        REQUIRE(osmium::opl_parse("n30", buffer));
    }
    SECTION("Negative ID") {
        REQUIRE(osmium::opl_parse("n-30", buffer));
    }

    osmium::handler::CheckOrder handler;
    REQUIRE_THROWS_AS(osmium::apply(buffer, handler), osmium::out_of_order_error);
}

TEST_CASE("CheckOrder handler: Ways after relations") {
    osmium::memory::Buffer buffer{1024};

    REQUIRE(osmium::opl_parse("r50", buffer));
    SECTION("Positive ID") {
        REQUIRE(osmium::opl_parse("w30", buffer));
    }
    SECTION("Negative ID") {
        REQUIRE(osmium::opl_parse("w-30", buffer));
    }

    osmium::handler::CheckOrder handler;
    REQUIRE_THROWS_AS(osmium::apply(buffer, handler), osmium::out_of_order_error);
}


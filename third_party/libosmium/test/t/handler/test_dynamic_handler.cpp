#include "catch.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/dynamic_handler.hpp>
#include <osmium/visitor.hpp>

struct Handler1 : public osmium::handler::Handler {

    int& count;

    explicit Handler1(int& c) :
        count(c) {
    }

    void node(const osmium::Node& /*node*/) noexcept {
        ++count;
    }

    void way(const osmium::Way& /*way*/) noexcept {
        ++count;
    }

    void relation(const osmium::Relation& /*relation*/) noexcept {
        ++count;
    }

    void area(const osmium::Area& /*area*/) noexcept {
        ++count;
    }

    void changeset(const osmium::Changeset& /*changeset*/) noexcept {
        ++count;
    }

    void flush() noexcept {
        ++count;
    }

};

struct Handler2 : public osmium::handler::Handler {

    int& count;

    explicit Handler2(int& c) :
        count(c) {
    }

    void node(const osmium::Node& /*node*/) noexcept {
        count += 2;
    }

    void way(const osmium::Way& /*way*/) noexcept {
        count += 2;
    }

    void relation(const osmium::Relation& /*relation*/) noexcept {
        count += 2;
    }

    void area(const osmium::Area& /*area*/) noexcept {
        count += 2;
    }

    void changeset(const osmium::Changeset& /*changeset*/) noexcept {
        count += 2;
    }

};

osmium::memory::Buffer fill_buffer() {
    using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)
    osmium::memory::Buffer buffer{1024UL * 1024UL, osmium::memory::Buffer::auto_grow::yes};

    osmium::builder::add_node(buffer, _id(1));
    osmium::builder::add_way(buffer, _id(2));
    osmium::builder::add_relation(buffer, _id(3));
    osmium::builder::add_area(buffer, _id(4));
    osmium::builder::add_changeset(buffer, _cid(5));

    return buffer;
}

TEST_CASE("Base test: static handler") {
    const auto buffer = fill_buffer();

    int count = 0;
    Handler1 h1{count};
    osmium::apply(buffer, h1);
    REQUIRE(count == 6);

    count = 0;
    Handler2 h2{count};
    osmium::apply(buffer, h2);
    REQUIRE(count == 10);
}

TEST_CASE("Dynamic handler") {
    const auto buffer = fill_buffer();

    osmium::handler::DynamicHandler handler;
    osmium::apply(buffer, handler);

    int count = 0;
    handler.set<Handler1>(count);
    osmium::apply(buffer, handler);
    REQUIRE(count == 6);

    count = 0;
    handler.set<Handler2>(count);
    osmium::apply(buffer, handler);
    REQUIRE(count == 10);
}


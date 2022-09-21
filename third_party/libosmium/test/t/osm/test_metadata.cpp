#include "catch.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/metadata_options.hpp>
#include <osmium/osm/object.hpp>

#include <stdexcept>

TEST_CASE("Metadata options: default") {
    const osmium::metadata_options m{};
    REQUIRE_FALSE(m.none());
    REQUIRE(m.any());
    REQUIRE(m.all());
    REQUIRE(m.version());
    REQUIRE(m.timestamp());
    REQUIRE(m.changeset());
    REQUIRE(m.uid());
    REQUIRE(m.user());
}

TEST_CASE("Metadata options: false") {
    const osmium::metadata_options m{"false"};
    REQUIRE(m.none());
    REQUIRE_FALSE(m.any());
    REQUIRE_FALSE(m.all());
    REQUIRE_FALSE(m.version());
    REQUIRE_FALSE(m.timestamp());
    REQUIRE_FALSE(m.changeset());
    REQUIRE_FALSE(m.uid());
    REQUIRE_FALSE(m.user());
}

TEST_CASE("Metadata options: none") {
    const osmium::metadata_options m{"none"};
    REQUIRE(m.none());
    REQUIRE_FALSE(m.any());
    REQUIRE_FALSE(m.all());
    REQUIRE_FALSE(m.version());
    REQUIRE_FALSE(m.timestamp());
    REQUIRE_FALSE(m.changeset());
    REQUIRE_FALSE(m.uid());
    REQUIRE_FALSE(m.user());
}

TEST_CASE("Metadata options: true") {
    const osmium::metadata_options m{"true"};
    REQUIRE_FALSE(m.none());
    REQUIRE(m.any());
    REQUIRE(m.all());
    REQUIRE(m.version());
    REQUIRE(m.timestamp());
    REQUIRE(m.changeset());
    REQUIRE(m.uid());
    REQUIRE(m.user());
}

TEST_CASE("Metadata options: all") {
    const osmium::metadata_options m{"all"};
    REQUIRE_FALSE(m.none());
    REQUIRE(m.any());
    REQUIRE(m.all());
    REQUIRE(m.version());
    REQUIRE(m.timestamp());
    REQUIRE(m.changeset());
    REQUIRE(m.uid());
    REQUIRE(m.user());
}

TEST_CASE("Metadata options: version,changeset") {
    const osmium::metadata_options m{"version+changeset"};
    REQUIRE_FALSE(m.none());
    REQUIRE(m.any());
    REQUIRE_FALSE(m.all());
    REQUIRE(m.version());
    REQUIRE_FALSE(m.timestamp());
    REQUIRE(m.changeset());
    REQUIRE_FALSE(m.uid());
    REQUIRE_FALSE(m.user());
}

TEST_CASE("Metadata options: timestamp,uid,user") {
    const osmium::metadata_options m{"timestamp+uid+user"};
    REQUIRE_FALSE(m.none());
    REQUIRE(m.any());
    REQUIRE_FALSE(m.all());
    REQUIRE_FALSE(m.version());
    REQUIRE(m.timestamp());
    REQUIRE_FALSE(m.changeset());
    REQUIRE(m.uid());
    REQUIRE(m.user());
}

TEST_CASE("Metadata options: fail") {
    REQUIRE_THROWS_AS(osmium::metadata_options{"timestamp+foo"}, const std::invalid_argument&);
}

TEST_CASE("Metdata options: constructor using OSMObject") {
    osmium::memory::Buffer buffer{10 * 1000};
    using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)

    SECTION("only version") {
        const osmium::OSMObject& obj = buffer.get<osmium::OSMObject>(osmium::builder::add_node(buffer,
                _id(1),
                _version(2)));
        osmium::metadata_options options = osmium::detect_available_metadata(obj);
        REQUIRE_FALSE(options.user());
        REQUIRE_FALSE(options.uid());
        REQUIRE_FALSE(options.changeset());
        REQUIRE_FALSE(options.timestamp());
        REQUIRE(options.version());
    }

    SECTION("full") {
        const osmium::OSMObject& obj = buffer.get<osmium::OSMObject>(osmium::builder::add_node(buffer,
                _id(1),
                _version(2),
                _timestamp("2018-01-01T23:00:00Z"),
                _cid(30),
                _uid(8),
                _user("foo")));
        osmium::metadata_options options = osmium::detect_available_metadata(obj);
        REQUIRE(options.all());
    }

    SECTION("changeset+timestamp+version") {
        const osmium::OSMObject& obj = buffer.get<osmium::OSMObject>(osmium::builder::add_node(buffer,
                _id(1),
                _version(2),
                _timestamp("2018-01-01T23:00:00Z"),
                _cid(30)));
        osmium::metadata_options options = osmium::detect_available_metadata(obj);
        REQUIRE(options.version());
        REQUIRE(options.timestamp());
        REQUIRE(options.changeset());
        REQUIRE_FALSE(options.user());
        REQUIRE_FALSE(options.uid());
    }
}

TEST_CASE("Metdata options: string representation should be valid 'version+changeset'") {
    const osmium::metadata_options options{"version+changeset"};
    REQUIRE(options.to_string() == "version+changeset");
}

TEST_CASE("Metdata options: string representation should be valid 'version+uid+user'") {
    const osmium::metadata_options options{"version+uid+user"};
    REQUIRE(options.to_string() == "version+uid+user");
}

TEST_CASE("Metdata options: string representation should be valid 'version+timestamp'") {
    const osmium::metadata_options options{"version+timestamp"};
    REQUIRE(options.to_string() == "version+timestamp");
}

TEST_CASE("Metdata options: string representation should be valid 'timestamp+version (different order'") {
    const osmium::metadata_options options{"timestamp+version"};
    REQUIRE(options.to_string() == "version+timestamp");
}

TEST_CASE("Metdata options: string representation should be valid 'none'") {
    const osmium::metadata_options options{"none"};
    REQUIRE(options.to_string() == "none");
}

TEST_CASE("Metdata options: string representation should be valid 'all (short)'") {
    const osmium::metadata_options options{"all"};
    REQUIRE(options.to_string() == "all");
}

TEST_CASE("Metdata options: string representation should be valid 'all (long)'") {
    const osmium::metadata_options options{"user+uid+version+timestamp+changeset"};
    REQUIRE(options.to_string() == "all");
}


#include "catch.hpp"

#include <osmium/io/file.hpp>

#include <iterator>
#include <stdexcept>

TEST_CASE("Default file format") {
    const osmium::io::File f;
    REQUIRE(osmium::io::file_format::unknown == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    REQUIRE_THROWS_AS(f.check(), const std::runtime_error&);
}

TEST_CASE("File format when empty (stdin/stdout)") {
    const osmium::io::File f{""};
    REQUIRE(osmium::io::file_format::unknown == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    REQUIRE_THROWS_AS(f.check(), const std::runtime_error&);
}

TEST_CASE("File format from dash (stdin/stdout)") {
    const osmium::io::File f{"-"};
    REQUIRE(osmium::io::file_format::unknown == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    REQUIRE_THROWS_AS(f.check(), const std::runtime_error&);
}

TEST_CASE("File format from dash with osm.bz2") {
    const osmium::io::File f{"-", "osm.bz2"};
    REQUIRE(f.filename().empty());
    REQUIRE(osmium::io::file_format::xml == f.format());
    REQUIRE(osmium::io::file_compression::bzip2 == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Detect file format by suffix 'osm'") {
    const osmium::io::File f{"test.osm"};
    REQUIRE(osmium::io::file_format::xml == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Detect file format by suffix 'pbf'") {
    const osmium::io::File f{"test.pbf"};
    REQUIRE(osmium::io::file_format::pbf == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Detect file format by suffix 'osm.pbf'") {
    const osmium::io::File f{"test.osm.pbf"};
    REQUIRE(osmium::io::file_format::pbf == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Detect file format by suffix 'opl'") {
    const osmium::io::File f{"test.opl"};
    REQUIRE(osmium::io::file_format::opl == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Detect file format by suffix 'osm.opl'") {
    const osmium::io::File f{"test.osm.opl"};
    REQUIRE(osmium::io::file_format::opl == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Detect file format by suffix 'osm.gz'") {
    const osmium::io::File f{"test.osm.gz"};
    REQUIRE(osmium::io::file_format::xml == f.format());
    REQUIRE(osmium::io::file_compression::gzip == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Detect file format by suffix 'opl.bz2'") {
    const osmium::io::File f{"test.osm.opl.bz2"};
    REQUIRE(osmium::io::file_format::opl == f.format());
    REQUIRE(osmium::io::file_compression::bzip2 == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Detect file format by suffix 'osc.gz'") {
    const osmium::io::File f{"test.osc.gz"};
    REQUIRE(osmium::io::file_format::xml == f.format());
    REQUIRE(osmium::io::file_compression::gzip == f.compression());
    REQUIRE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Detect file format by suffix 'opl.gz'") {
    const osmium::io::File f{"test.osh.opl.gz"};
    REQUIRE(osmium::io::file_format::opl == f.format());
    REQUIRE(osmium::io::file_compression::gzip == f.compression());
    REQUIRE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Detect file format by suffix 'osh.pbf'") {
    const osmium::io::File f{"test.osh.pbf"};
    REQUIRE(osmium::io::file_format::pbf == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Override file format by suffix 'osm'") {
    const osmium::io::File f{"test", "osm"};
    REQUIRE(osmium::io::file_format::xml == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Override file format by suffix 'pbf'") {
    const osmium::io::File f{"test", "pbf"};
    REQUIRE(osmium::io::file_format::pbf == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Override file format by suffix 'osm.pbf'") {
    const osmium::io::File f{"test", "osm.pbf"};
    REQUIRE(osmium::io::file_format::pbf == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Override file format by suffix 'opl'") {
    const osmium::io::File f{"test", "opl"};
    REQUIRE(osmium::io::file_format::opl == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Override file format by suffix 'osm.opl'") {
    const osmium::io::File f{"test", "osm.opl"};
    REQUIRE(osmium::io::file_format::opl == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Override file format by suffix 'osm.gz'") {
    const osmium::io::File f{"test", "osm.gz"};
    REQUIRE(osmium::io::file_format::xml == f.format());
    REQUIRE(osmium::io::file_compression::gzip == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Override file format by suffix 'osm.opl.bz2'") {
    const osmium::io::File f{"test", "osm.opl.bz2"};
    REQUIRE(osmium::io::file_format::opl == f.format());
    REQUIRE(osmium::io::file_compression::bzip2 == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Override file format by suffix 'osc.gz'") {
    const osmium::io::File f{"test", "osc.gz"};
    REQUIRE(osmium::io::file_format::xml == f.format());
    REQUIRE(osmium::io::file_compression::gzip == f.compression());
    REQUIRE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Override file format by suffix 'osh.opl.gz'") {
    const osmium::io::File f{"test", "osh.opl.gz"};
    REQUIRE(osmium::io::file_format::opl == f.format());
    REQUIRE(osmium::io::file_compression::gzip == f.compression());
    REQUIRE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("File format by suffix 'blackhole'") {
    const osmium::io::File f{"test.blackhole"};
    REQUIRE(osmium::io::file_format::blackhole == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Override file format by suffix 'blackhole'") {
    const osmium::io::File f{"test", "blackhole"};
    REQUIRE(osmium::io::file_format::blackhole == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Override file format by suffix 'osm.blackhole'") {
    const osmium::io::File f{"test", "osm.blackhole"};
    REQUIRE(osmium::io::file_format::blackhole == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Override file format by suffix 'osm.blackhole.bz2'") {
    const osmium::io::File f{"test", "osm.blackhole.bz2"};
    REQUIRE(osmium::io::file_format::blackhole == f.format());
    REQUIRE(osmium::io::file_compression::bzip2 == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Override file format by suffix 'osh.blackhole.gz'") {
    const osmium::io::File f{"test", "osh.blackhole.gz"};
    REQUIRE(osmium::io::file_format::blackhole == f.format());
    REQUIRE(osmium::io::file_compression::gzip == f.compression());
    REQUIRE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Override file format by suffix 'osh.pbf'") {
    const osmium::io::File f{"test", "osh.pbf"};
    REQUIRE(osmium::io::file_format::pbf == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Format option pbf history") {
    const osmium::io::File f{"test", "pbf,history=true"};
    REQUIRE(osmium::io::file_format::pbf == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("Format option pbf foo") {
    const osmium::io::File f{"test.osm", "pbf,foo=bar"};
    REQUIRE(osmium::io::file_format::pbf == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE("bar" == f.get("foo"));
    f.check();
}

TEST_CASE("Format option xml abc something") {
    const osmium::io::File f{"test.bla", "xml,abc,some=thing"};
    REQUIRE(osmium::io::file_format::xml == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE("true" == f.get("abc"));
    REQUIRE("thing" == f.get("some"));
    REQUIRE(2 == std::distance(f.begin(), f.end()));
    f.check();
}

TEST_CASE("Unknown format 'foo.bar'") {
    const osmium::io::File f{"test.foo.bar"};
    REQUIRE(osmium::io::file_format::unknown == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_THROWS_AS(f.check(), const std::runtime_error&);
}

TEST_CASE("Unknown format 'foo'") {
    const osmium::io::File f{"test", "foo"};
    REQUIRE_THROWS_AS(f.check(), const std::runtime_error&);
}

TEST_CASE("Unknown format 'osm.foo'") {
    const osmium::io::File f{"test", "osm.foo"};
    REQUIRE_THROWS_AS(f.check(), const std::runtime_error&);
}

TEST_CASE("Unknown format 'bla=foo'") {
    const osmium::io::File f{"test", "bla=foo"};
    REQUIRE_THROWS_AS(f.check(), const std::runtime_error&);
}

TEST_CASE("URL without format") {
    const osmium::io::File f{"http://www.example.com/api"};
    REQUIRE(osmium::io::file_format::xml == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("URL without format and filename") {
    const osmium::io::File f{"https://planet.osm.org/pbf/planet-latest.osm.pbf"};
    REQUIRE(osmium::io::file_format::pbf == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE_FALSE(f.has_multiple_object_versions());
    f.check();
}

TEST_CASE("URL with format") {
    const osmium::io::File f{"http://www.example.com/api", "osh"};
    REQUIRE(osmium::io::file_format::xml == f.format());
    REQUIRE(osmium::io::file_compression::none == f.compression());
    REQUIRE(f.has_multiple_object_versions());
    f.check();
}


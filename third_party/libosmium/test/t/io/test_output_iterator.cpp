#include "catch.hpp"

#include <osmium/io/output_iterator.hpp>
#include <osmium/io/writer.hpp>
#include <osmium/io/xml_output.hpp>

TEST_CASE("Output iterator should be copy constructable") {
    const osmium::io::Header header{};
    osmium::io::Writer writer{"test.osm", header, osmium::io::overwrite::allow};

    osmium::io::OutputIterator<osmium::io::Writer> out1{writer};
    osmium::io::OutputIterator<osmium::io::Writer> out2{out1};
}

TEST_CASE("Output iterator should be copy assignable") {
    const osmium::io::Header header{};
    osmium::io::Writer writer1{"test1.osm", header, osmium::io::overwrite::allow};
    osmium::io::Writer writer2{"test2.osm", header, osmium::io::overwrite::allow};

    osmium::io::OutputIterator<osmium::io::Writer> out1{writer1};
    osmium::io::OutputIterator<osmium::io::Writer> out2{writer2};

    out2 = out1;
}

TEST_CASE("Output iterator should be incrementable") {
    const osmium::io::Header header{};
    osmium::io::Writer writer{"test.osm", header, osmium::io::overwrite::allow};
    osmium::io::OutputIterator<osmium::io::Writer> out{writer};

    ++out;
}


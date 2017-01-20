#include "catch.hpp"

#include <osmium/io/xml_output.hpp>
#include <osmium/io/output_iterator.hpp>
#include <osmium/io/writer.hpp>

TEST_CASE("output iterator") {

    osmium::io::Header header;

    SECTION("should be copy constructable") {
        osmium::io::Writer writer{"test.osm", header, osmium::io::overwrite::allow};
        osmium::io::OutputIterator<osmium::io::Writer> out1{writer};

        osmium::io::OutputIterator<osmium::io::Writer> out2{out1};
    }

    SECTION("should be copy assignable") {
        osmium::io::Writer writer1{"test1.osm", header, osmium::io::overwrite::allow};
        osmium::io::Writer writer2{"test2.osm", header, osmium::io::overwrite::allow};

        osmium::io::OutputIterator<osmium::io::Writer> out1{writer1};
        osmium::io::OutputIterator<osmium::io::Writer> out2{writer2};

        out2 = out1;
    }

    SECTION("should be incrementable") {
        osmium::io::Writer writer{"test.osm", header, osmium::io::overwrite::allow};
        osmium::io::OutputIterator<osmium::io::Writer> out{writer};

        ++out;
    }

}


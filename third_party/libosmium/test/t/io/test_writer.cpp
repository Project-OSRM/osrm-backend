#include "catch.hpp"
#include "utils.hpp"

#include <algorithm>

#include <osmium/io/any_compression.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/io/xml_output.hpp>
#include <osmium/io/output_iterator.hpp>
#include <osmium/memory/buffer.hpp>

TEST_CASE("Writer") {
    osmium::io::Header header;
    header.set("generator", "test_writer.cpp");

    osmium::io::Reader reader{with_data_dir("t/io/data.osm")};
    osmium::memory::Buffer buffer = reader.read();
    REQUIRE(buffer);
    REQUIRE(buffer.committed() > 0);
    const auto num = std::distance(buffer.select<osmium::OSMObject>().cbegin(), buffer.select<osmium::OSMObject>().cend());
    REQUIRE(num > 0);
    REQUIRE(buffer.select<osmium::OSMObject>().cbegin()->id() == 1);

    std::string filename;

    SECTION("Empty writes") {

        SECTION("Empty buffer") {
            filename = "test-writer-out-empty-buffer.osm";
            osmium::io::Writer writer{filename, header, osmium::io::overwrite::allow};
            osmium::memory::Buffer empty_buffer(1024);
            writer(std::move(empty_buffer));
            writer.close();
        }

        SECTION("Invalid buffer") {
            filename = "test-writer-out-invalid-buffer.osm";
            osmium::io::Writer writer{filename, header, osmium::io::overwrite::allow};
            osmium::memory::Buffer invalid_buffer;
            writer(std::move(invalid_buffer));
            writer.close();
        }

        osmium::io::Reader reader_check{filename};
        osmium::memory::Buffer buffer_check = reader_check.read();
        REQUIRE(!buffer_check);
    }

    SECTION("Successfull writes") {

        SECTION("Writer buffer") {
            filename = "test-writer-out-buffer.osm";
            osmium::io::Writer writer{filename, header, osmium::io::overwrite::allow};
            writer(std::move(buffer));
            writer.close();

            REQUIRE_THROWS_AS({
                writer(osmium::memory::Buffer{});
            }, osmium::io_error);
        }

        SECTION("Writer item") {
            filename = "test-writer-out-item.osm";
            osmium::io::Writer writer{filename, header, osmium::io::overwrite::allow};
            for (const auto& item : buffer) {
                writer(item);
            }
            writer.close();
        }

        SECTION("Writer output iterator") {
            filename = "test-writer-out-iterator.osm";
            osmium::io::Writer writer{filename, header, osmium::io::overwrite::allow};
            auto it = osmium::io::make_output_iterator(writer);
            std::copy(buffer.cbegin(), buffer.cend(), it);
            writer.close();
        }

        osmium::io::Reader reader_check{filename};
        osmium::memory::Buffer buffer_check = reader_check.read();
        REQUIRE(buffer_check);
        REQUIRE(buffer_check.committed() > 0);
        REQUIRE(buffer_check.select<osmium::OSMObject>().size() == num);
        REQUIRE(buffer_check.select<osmium::OSMObject>().cbegin()->id() == 1);
    }

    SECTION("Interrupted writer after open") {
        int error = 0;
        try {
            filename = "test-writer-out-fail1.osm";
            osmium::io::Writer writer{filename, header, osmium::io::overwrite::allow};
            throw 1;
        } catch (int e) {
            error = e;
        }

        REQUIRE(error > 0);
    }

    SECTION("Interrupted writer after write") {
        int error = 0;
        try {
            filename = "test-writer-out-fail2.osm";
            osmium::io::Writer writer{filename, header, osmium::io::overwrite::allow};
            writer(std::move(buffer));
            throw 2;
        } catch (int e) {
            error = e;
        }

        REQUIRE(error > 0);
    }

}


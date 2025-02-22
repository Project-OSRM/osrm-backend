/*

  EXAMPLE osmium_create_pois

  Showing how to create nodes for points of interest out of thin air.

  DEMONSTRATES USE OF:
  * file output
  * Osmium buffers
  * using builders to write data

  SIMPLER EXAMPLES you might want to understand first:
  * osmium_read
  * osmium_count
  * osmium_pub_names

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <cstring>   // for std::strcmp
#include <ctime>     // for std::time
#include <exception> // for std::exception
#include <iostream>  // for std::cout, std::cerr
#include <string>    // for std::string
#include <utility>   // for std::move

// Allow any format of output files (XML, PBF, ...)
#include <osmium/io/any_output.hpp>

// We want to use the builder interface
#include <osmium/builder/attr.hpp>
#include <osmium/builder/osm_object_builder.hpp>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " OUTFILE\n";
        return 1;
    }

    // Get output file name from command line.
    const std::string output_file_name{argv[1]};

    // If output file name is "-", this means STDOUT. Set the OPL file type
    // in this case. Otherwise take the file type from the file name suffix.
    const osmium::io::File output_file{output_file_name, output_file_name == "-" ? ".opl" : ""};

    try {
        // Create a buffer where all objects will live. Use a sensible initial
        // buffer size and set the buffer to automatically grow if needed.
        const size_t initial_buffer_size = 10000;
        osmium::memory::Buffer buffer{initial_buffer_size, osmium::memory::Buffer::auto_grow::yes};

        // Declare this to use the functions starting with the underscore (_) below.
        using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)

        // Add nodes to the buffer. This is, of course, only an example.
        // You can set any of the attributes and more tags, etc. Ways and
        // relations can be added in a similar way.
        osmium::builder::add_node(buffer,
            _id(-1),
            _version(1),
            _timestamp(std::time(nullptr)),
            _location(osmium::Location{1.23, 3.45}),
            _tag("amenity", "post_box")
        );

        osmium::builder::add_node(buffer,
            _id(-2),
            _version(1),
            _timestamp(std::time(nullptr)),
            _location(1.24, 3.46),
            _tags({{"amenity", "restaurant"},
                   {"name", "Chez OSM"}})
        );

        // Create header and set generator.
        osmium::io::Header header;
        header.set("generator", "osmium_create_pois");

        // Initialize Writer using the header from above and tell it that it
        // is allowed to overwrite a possibly existing file.
        osmium::io::Writer writer{output_file, header, osmium::io::overwrite::allow};

        // Write out the contents of the output buffer.
        writer(std::move(buffer));

        // Explicitly close the writer. Will throw an exception if there is
        // a problem. If you wait for the destructor to close the writer, you
        // will not notice the problem, because destructors must not throw.
        writer.close();
    } catch (const std::exception& e) {
        // All exceptions used by the Osmium library derive from std::exception.
        std::cerr << e.what() << '\n';
        return 1;
    }
}


/*

  EXAMPLE osmium_pub_names

  Show the names and addresses of all pubs found in an OSM file.

  DEMONSTRATES USE OF:
  * file input
  * your own handler
  * access to tags

  SIMPLER EXAMPLES you might want to understand first:
  * osmium_read
  * osmium_count

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <cstdlib>  // for std::exit
#include <cstring>  // for std::strncmp
#include <iostream> // for std::cout, std::cerr

// Allow any format of input files (XML, PBF, ...)
#include <osmium/io/any_input.hpp>

// We want to use the handler interface
#include <osmium/handler.hpp>

// For osmium::apply()
#include <osmium/visitor.hpp>

class NamesHandler : public osmium::handler::Handler {

    static void output_pubs(const osmium::OSMObject& object) {
        const osmium::TagList& tags = object.tags();
        if (tags.has_tag("amenity", "pub")) {

            // Print name of the pub if it is set.
            const char* name = tags["name"];
            if (name) {
                std::cout << name << "\n";
            } else {
                std::cout << "pub with unknown name\n";
            }

            // Iterate over all tags finding those which start with "addr:"
            // and print them.
            for (const osmium::Tag& tag : tags) {
                if (!std::strncmp(tag.key(), "addr:", 5)) {
                    std::cout << "  " << tag.key() << ": " << tag.value() << "\n";
                }
            }
        }
    }

public:

    // Nodes can be tagged amenity=pub.
    void node(const osmium::Node& node) {
        output_pubs(node);
    }

    // Ways can be tagged amenity=pub, too (typically buildings).
    void way(const osmium::Way& way) {
        output_pubs(way);
    }

}; // class NamesHandler

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE\n";
        std::exit(1);
    }

    try {
        // Construct the handler defined above
        NamesHandler names_handler;

        // Initialize the reader with the filename from the command line and
        // tell it to only read nodes and ways. We are ignoring multipolygon
        // relations in this simple example.
        osmium::io::Reader reader{argv[1], osmium::osm_entity_bits::node | osmium::osm_entity_bits::way};

        // Apply input data to our own handler
        osmium::apply(reader, names_handler);
    } catch (const std::exception& e) {
        // All exceptions used by the Osmium library derive from std::exception.
        std::cerr << e.what() << '\n';
        std::exit(1);
    }
}


/*

  EXAMPLE osmium_debug

  Dump the contents of the input file in a debug format.

  DEMONSTRATES USE OF:
  * file input reading only some types
  * the dump handler

  SIMPLER EXAMPLES you might want to understand first:
  * osmium_read
  * osmium_count

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <cstdlib>  // for std::exit
#include <iostream> // for std::cout, std::cerr
#include <string>   // for std::string

// The Dump handler
#include <osmium/handler/dump.hpp>

// Allow any format of input files (XML, PBF, ...)
#include <osmium/io/any_input.hpp>

int main(int argc, char* argv[]) {
    // Speed up output (not Osmium-specific)
    std::ios_base::sync_with_stdio(false);

    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE [TYPES]\n";
        std::cerr << "TYPES can be any combination of 'n', 'w', 'r', and 'c' to indicate what types of OSM entities you want (default: all).\n";
        std::exit(1);
    }

    try {
        // Default is all entity types: nodes, ways, relations, and changesets
        osmium::osm_entity_bits::type read_types = osmium::osm_entity_bits::all;

        // Get entity types from command line if there is a 2nd argument.
        if (argc == 3) {
            read_types = osmium::osm_entity_bits::nothing;
            std::string types = argv[2];
            if (types.find('n') != std::string::npos) {
                read_types |= osmium::osm_entity_bits::node;
            }
            if (types.find('w') != std::string::npos) {
                read_types |= osmium::osm_entity_bits::way;
            }
            if (types.find('r') != std::string::npos) {
                read_types |= osmium::osm_entity_bits::relation;
            }
            if (types.find('c') != std::string::npos) {
                read_types |= osmium::osm_entity_bits::changeset;
            }
        }

        // Initialize Reader with file name and the types of entities we want to
        // read.
        osmium::io::Reader reader{argv[1], read_types};

        // The file header can contain metadata such as the program that generated
        // the file and the bounding box of the data.
        osmium::io::Header header = reader.header();
        std::cout << "HEADER:\n  generator=" << header.get("generator") << "\n";

        for (const auto& bbox : header.boxes()) {
            std::cout << "  bbox=" << bbox << "\n";
        }

        // Initialize Dump handler.
        osmium::handler::Dump dump{std::cout};

        // Read from input and send everything to Dump handler.
        osmium::apply(reader, dump);

        // You do not have to close the Reader explicitly, but because the
        // destructor can't throw, you will not see any errors otherwise.
        reader.close();
    } catch (const std::exception& e) {
        // All exceptions used by the Osmium library derive from std::exception.
        std::cerr << e.what() << '\n';
        std::exit(1);
    }
}


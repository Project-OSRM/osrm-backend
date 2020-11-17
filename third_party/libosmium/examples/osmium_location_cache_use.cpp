/*

  EXAMPLE osmium_location_cache_use

  This reads ways from an OSM file and writes out the way node locations
  it got from a location cache generated with osmium_location_cache_create.

  Warning: The locations cache file will get huge (>32GB) if you are using
           the DenseFileArray index even if the input file is small, because
           it depends on the *largest* node ID, not the number of nodes.

  DEMONSTRATES USE OF:
  * file input
  * location indexes and the NodeLocationsForWays handler
  * location indexes on disk

  SIMPLER EXAMPLES you might want to understand first:
  * osmium_read
  * osmium_count
  * osmium_road_length

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <cerrno>      // for errno
#include <cstdlib>     // for std::exit
#include <cstring>     // for strerror
#include <fcntl.h>     // for open
#include <iostream>    // for std::cout, std::cerr
#include <string>      // for std::string
#include <sys/stat.h>  // for open
#include <sys/types.h> // for open

#ifdef _WIN32
# include <io.h>       // for _setmode
#endif

// Allow any format of input files (XML, PBF, ...)
#include <osmium/io/any_input.hpp>

// For the location index. There are different types of index implementation
// available. These implementations put the index on disk. See below.
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/index/map/sparse_file_array.hpp>

// For the NodeLocationForWays handler
#include <osmium/handler/node_locations_for_ways.hpp>

// For osmium::apply()
#include <osmium/visitor.hpp>

// Chose one of these two. "sparse" is best used for small and medium extracts,
// the "dense" index for large extracts or the whole planet.
using index_type = osmium::index::map::SparseFileArray<osmium::unsigned_object_id_type, osmium::Location>;
//using index_type = osmium::index::map::DenseFileArray<osmium::unsigned_object_id_type, osmium::Location>;

// The location handler always depends on the index type
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

// This handler only implements the way() function which prints out the way
// ID and all nodes IDs and locations in those ways.
struct MyHandler : public osmium::handler::Handler {

    void way(const osmium::Way& way) {
        std::cout << "way " << way.id() << "\n";
        for (const auto& nr : way.nodes()) {
            std::cout << "  node " << nr.ref() << " " << nr.location() << "\n";
        }
    }

}; // struct MyHandler

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " OSM_FILE CACHE_FILE\n";
        std::exit(1);
    }

    try {
        const std::string input_filename{argv[1]};
        const std::string cache_filename{argv[2]};

        // Construct Reader reading only ways
        osmium::io::Reader reader{input_filename, osmium::osm_entity_bits::way};

        // Initialize location index on disk using an existing file
        const int fd = ::open(cache_filename.c_str(), O_RDWR);
        if (fd == -1) {
            std::cerr << "Can not open location cache file '" << cache_filename << "': " << std::strerror(errno) << "\n";
            return 1;
        }
#ifdef _WIN32
        _setmode(fd, _O_BINARY);
#endif
        index_type index{fd};

        // The handler that adds node locations from the index to the ways.
        location_handler_type location_handler{index};

        // Feed all ways through the location handler and then our own handler.
        MyHandler handler;
        osmium::apply(reader, location_handler, handler);

        // Explicitly close input so we get notified of any errors.
        reader.close();
    } catch (const std::exception& e) {
        // All exceptions used by the Osmium library derive from std::exception.
        std::cerr << e.what() << '\n';
        std::exit(1);
    }
}


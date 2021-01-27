/*

  EXAMPLE osmium_amenity_list

  Create a list of all amenities in the OSM input data. The type of amenity
  (tag value) and, if available, the name is printed. For nodes, the location
  is printed, for areas the center location.

  DEMONSTRATES USE OF:
  * file input
  * location indexes and the NodeLocationsForWays handler
  * the MultipolygonManager and Assembler to assemble areas (multipolygons)
  * your own handler that works with areas (multipolygons)
  * accessing tags
  * osmium::geom::Coordinates

  SIMPLER EXAMPLES you might want to understand first:
  * osmium_read
  * osmium_count
  * osmium_debug

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <cstdio>   // for std::printf
#include <cstdlib>  // for std::exit
#include <iostream> // for std::cerr
#include <string>   // for std::string

// For the location index. There are different types of indexes available.
// This will work for all input files keeping the index in memory.
#include <osmium/index/map/flex_mem.hpp>

// For the NodeLocationForWays handler
#include <osmium/handler/node_locations_for_ways.hpp>

// The type of index used. This must match the include file above
using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;

// The location handler always depends on the index type
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

// For assembling multipolygons
#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_manager.hpp>

// Allow any format of input files (XML, PBF, ...)
#include <osmium/io/any_input.hpp>

// For osmium::apply()
#include <osmium/visitor.hpp>

// For osmium::geom::Coordinates
#include <osmium/geom/coordinates.hpp>

class AmenityHandler : public osmium::handler::Handler {

    // Print info about one amenity to stdout.
    static void print_amenity(const char* type, const char* name, const osmium::geom::Coordinates& c) {
        std::printf("%8.4f,%8.4f %-15s %s\n", c.x, c.y, type, name ? name : "");
    }

    // Calculate the center point of a NodeRefList.
    static osmium::geom::Coordinates calc_center(const osmium::NodeRefList& nr_list) {
        // Coordinates simply store an X and Y coordinate pair as doubles.
        // (Unlike osmium::Location which stores them more efficiently as
        // 32 bit integers.) Use Coordinates when you want to do calculations
        // or store projected coordinates.
        osmium::geom::Coordinates c{0.0, 0.0};

        for (const auto& nr : nr_list) {
            c.x += nr.lon();
            c.y += nr.lat();
        }

        c.x /= nr_list.size();
        c.y /= nr_list.size();

        return c;
    }

public:

    void node(const osmium::Node& node) {
        // Getting a tag value can be expensive, because a list of tags has
        // to be gone through and each tag has to be checked. So we store the
        // result and reuse it.
        const char* amenity = node.tags()["amenity"];
        if (amenity) {
            print_amenity(amenity, node.tags()["name"], node.location());
        }
    }

    void area(const osmium::Area& area) {
        const char* amenity = area.tags()["amenity"];
        if (amenity) {
            // Use the center of the first outer ring. Because we set
            // create_empty_areas = false in the assembler config, we can
            // be sure there will always be at least one outer ring.
            const auto center = calc_center(*area.cbegin<osmium::OuterRing>());

            print_amenity(amenity, area.tags()["name"], center);
        }
    }

}; // class AmenityHandler

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE\n";
        std::exit(1);
    }

    try {
        // The input file
        const osmium::io::File input_file{argv[1]};

        // Configuration for the multipolygon assembler. We disable the option to
        // create empty areas when invalid multipolygons are encountered. This
        // means areas created have a valid geometry and invalid multipolygons
        // are simply ignored.
        osmium::area::Assembler::config_type assembler_config;
        assembler_config.create_empty_areas = false;

        // Initialize the MultipolygonManager. Its job is to collect all
        // relations and member ways needed for each area. It then calls an
        // instance of the osmium::area::Assembler class (with the given config)
        // to actually assemble one area.
        osmium::area::MultipolygonManager<osmium::area::Assembler> mp_manager{assembler_config};

        // We read the input file twice. In the first pass, only relations are
        // read and fed into the multipolygon manager.
        std::cerr << "Pass 1...\n";
        osmium::relations::read_relations(input_file, mp_manager);
        std::cerr << "Pass 1 done\n";

        // The index storing all node locations.
        index_type index;

        // The handler that stores all node locations in the index and adds them
        // to the ways.
        location_handler_type location_handler{index};

        // If a location is not available in the index, we ignore it. It might
        // not be needed (if it is not part of a multipolygon relation), so why
        // create an error?
        location_handler.ignore_errors();

        // Create our handler.
        AmenityHandler data_handler;

        // On the second pass we read all objects and run them first through the
        // node location handler and then the multipolygon manager. The manager
        // will put the areas it has created into the "buffer" which are then
        // fed through our handler.
        //
        // The read_meta::no option disables reading of meta data (such as version
        // numbers, timestamps, etc.) which are not needed in this case. Disabling
        // this can speed up your program.
        std::cerr << "Pass 2...\n";
        osmium::io::Reader reader{input_file, osmium::io::read_meta::no};

        osmium::apply(reader, location_handler, data_handler, mp_manager.handler([&data_handler](const osmium::memory::Buffer& area_buffer) {
            osmium::apply(area_buffer, data_handler);
        }));

        reader.close();
        std::cerr << "Pass 2 done\n";
    } catch (const std::exception& e) {
        // All exceptions used by the Osmium library derive from std::exception.
        std::cerr << e.what() << '\n';
        std::exit(1);
    }
}


/*

  EXAMPLE osmium_area_test

  Create multipolygons from OSM data and dump them to stdout in one of two
  formats: WKT or using the built-in Dump format.

  DEMONSTRATES USE OF:
  * file input
  * location indexes and the NodeLocationsForWays handler
  * the MultipolygonManager and Assembler to assemble areas (multipolygons)
  * your own handler that works with areas (multipolygons)
  * the WKTFactory to write geometries in WKT format
  * the Dump handler
  * the DynamicHandler

  SIMPLER EXAMPLES you might want to understand first:
  * osmium_read
  * osmium_count
  * osmium_debug
  * osmium_amenity_list

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <cstring>  // for std::strcmp
#include <iostream> // for std::cout, std::cerr

// For assembling multipolygons
#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_manager.hpp>

// For the DynamicHandler class
#include <osmium/dynamic_handler.hpp>

// For the WKT factory
#include <osmium/geom/wkt.hpp>

// For the Dump handler
#include <osmium/handler/dump.hpp>

// For the NodeLocationForWays handler
#include <osmium/handler/node_locations_for_ways.hpp>

// Allow any format of input files (XML, PBF, ...)
#include <osmium/io/any_input.hpp>

// For osmium::apply()
#include <osmium/visitor.hpp>

// For the location index. There are different types of indexes available.
// This will work for all input files keeping the index in memory.
#include <osmium/index/map/flex_mem.hpp>

// The type of index used. This must match the include file above
using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;

// The location handler always depends on the index type
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

// This handler writes all area geometries out in WKT (Well Known Text) format.
class WKTDump : public osmium::handler::Handler {

    // This factory is used to create a geometry in WKT format from OSM
    // objects. The template parameter is empty here, because we output WGS84
    // coordinates, but could be used for a projection.
    osmium::geom::WKTFactory<> m_factory;

public:

    // This callback is called by osmium::apply for each area in the data.
    void area(const osmium::Area& area) {
        try {
            std::cout << m_factory.create_multipolygon(area) << "\n";
        } catch (const osmium::geometry_error& e) {
            std::cout << "GEOMETRY ERROR: " << e.what() << "\n";
        }
    }

}; // class WKTDump

void print_help() {
    std::cout << "osmium_area_test [OPTIONS] OSMFILE\n\n"
              << "Read OSMFILE and build multipolygons from it.\n"
              << "\nOptions:\n"
              << "  -h, --help           This help message\n"
              << "  -w, --dump-wkt       Dump area geometries as WKT\n"
              << "  -o, --dump-objects   Dump area objects\n";
}

void print_usage(const char* prgname) {
    std::cerr << "Usage: " << prgname << " [OPTIONS] OSMFILE\n";
}

int main(int argc, char* argv[]) {
    if (argc > 1 && (!std::strcmp(argv[1], "-h") ||
                     !std::strcmp(argv[1], "--help"))) {
        print_help();
        return 0;
    }

    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    try {
        // Initialize an empty DynamicHandler. Later it will be associated
        // with one of the handlers. You can think of the DynamicHandler as
        // a kind of "variant handler" or a "pointer handler" pointing to the
        // real handler.
        osmium::handler::DynamicHandler handler;

        if (!std::strcmp(argv[1], "-w") || !std::strcmp(argv[1], "--dump-wkt")) {
            handler.set<WKTDump>();
        } else if (!std::strcmp(argv[1], "-o") || !std::strcmp(argv[1], "--dump-objects")) {
            handler.set<osmium::handler::Dump>(std::cout);
        } else {
            print_usage(argv[0]);
            return 1;
        }

        const osmium::io::File input_file{argv[2]};

        // Configuration for the multipolygon assembler. Here the default settings
        // are used, but you could change multiple settings.
        const osmium::area::Assembler::config_type assembler_config;

        // Set up a filter matching only forests. This will be used to only build
        // areas with matching tags.
        osmium::TagsFilter filter{false};
        filter.add_rule(true, "landuse", "forest");
        filter.add_rule(true, "natural", "wood");

        // Initialize the MultipolygonManager. Its job is to collect all
        // relations and member ways needed for each area. It then calls an
        // instance of the osmium::area::Assembler class (with the given config)
        // to actually assemble one area. The filter parameter is optional, if
        // it is not set, all areas will be built.
        osmium::area::MultipolygonManager<osmium::area::Assembler> mp_manager{assembler_config, filter};

        // We read the input file twice. In the first pass, only relations are
        // read and fed into the multipolygon manager.
        std::cerr << "Pass 1...\n";
        osmium::relations::read_relations(input_file, mp_manager);
        std::cerr << "Pass 1 done\n";

        // Output the amount of main memory used so far. All multipolygon relations
        // are in memory now.
        std::cerr << "Memory:\n";
        osmium::relations::print_used_memory(std::cerr, mp_manager.used_memory());

        // The index storing all node locations.
        index_type index;

        // The handler that stores all node locations in the index and adds them
        // to the ways.
        location_handler_type location_handler{index};

        // If a location is not available in the index, we ignore it. It might
        // not be needed (if it is not part of a multipolygon relation), so why
        // create an error?
        location_handler.ignore_errors();

        // On the second pass we read all objects and run them first through the
        // node location handler and then the multipolygon collector. The collector
        // will put the areas it has created into the "buffer" which are then
        // fed through our "handler".
        std::cerr << "Pass 2...\n";
        osmium::io::Reader reader{input_file};
        osmium::apply(reader, location_handler, mp_manager.handler([&handler](osmium::memory::Buffer&& buffer) {
            osmium::apply(buffer, handler);
        }));
        reader.close();
        std::cerr << "Pass 2 done\n";

        // Output the amount of main memory used so far. All complete multipolygon
        // relations have been cleaned up.
        std::cerr << "Memory:\n";
        osmium::relations::print_used_memory(std::cerr, mp_manager.used_memory());

        // If there were multipolgyon relations in the input, but some of their
        // members are not in the input file (which often happens for extracts)
        // this will write the IDs of the incomplete relations to stderr.
        std::vector<osmium::object_id_type> incomplete_relations_ids;
        mp_manager.for_each_incomplete_relation([&](const osmium::relations::RelationHandle& handle){
            incomplete_relations_ids.push_back(handle->id());
        });
        if (!incomplete_relations_ids.empty()) {
            std::cerr << "Warning! Some member ways missing for these multipolygon relations:";
            for (const auto id : incomplete_relations_ids) {
                std::cerr << " " << id;
            }
            std::cerr << "\n";
        }
    } catch (const std::exception& e) {
        // All exceptions used by the Osmium library derive from std::exception.
        std::cerr << e.what() << '\n';
        return 1;
    }
}


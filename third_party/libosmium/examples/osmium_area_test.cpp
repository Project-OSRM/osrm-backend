/*

  EXAMPLE osmium_area_test

  Create multipolygons from OSM data and dump them to stdout in one of two
  formats: WKT or using the built-in Dump format.

  DEMONSTRATES USE OF:
  * file input
  * location indexes and the NodeLocationsForWays handler
  * the MultipolygonCollector and Assembler to assemble areas (multipolygons)
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

#include <cstdlib>  // for std::exit
#include <getopt.h> // for getopt_long
#include <iostream> // for std::cout, std::cerr

// For assembling multipolygons
#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_collector.hpp>

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
// This will work for small and medium sized input files.
#include <osmium/index/map/sparse_mem_array.hpp>

// The type of index used. This must match the include file above
using index_type = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;

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

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"help",         no_argument, 0, 'h'},
        {"dump-wkt",     no_argument, 0, 'w'},
        {"dump-objects", no_argument, 0, 'o'},
        {0, 0, 0, 0}
    };

    // Initialize an empty DynamicHandler. Later it will be associated
    // with one of the handlers. You can think of the DynamicHandler as
    // a kind of "variant handler" or a "pointer handler" pointing to the
    // real handler.
    osmium::handler::DynamicHandler handler;

    // Read options from command line.
    while (true) {
        const int c = getopt_long(argc, argv, "hwo", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help();
                std::exit(0);
            case 'w':
                handler.set<WKTDump>();
                break;
            case 'o':
                handler.set<osmium::handler::Dump>(std::cout);
                break;
            default:
                std::exit(1);
        }
    }

    const int remaining_args = argc - optind;
    if (remaining_args != 1) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] OSMFILE\n";
        std::exit(1);
    }

    osmium::io::File input_file{argv[optind]};

    // Configuration for the multipolygon assembler. Here the default settings
    // are used, but you could change multiple settings.
    osmium::area::Assembler::config_type assembler_config;

    // Initialize the MultipolygonCollector. Its job is to collect all
    // relations and member ways needed for each area. It then calls an
    // instance of the osmium::area::Assembler class (with the given config)
    // to actually assemble one area.
    osmium::area::MultipolygonCollector<osmium::area::Assembler> collector{assembler_config};

    // We read the input file twice. In the first pass, only relations are
    // read and fed into the multipolygon collector.
    std::cerr << "Pass 1...\n";
    osmium::io::Reader reader1{input_file, osmium::osm_entity_bits::relation};
    collector.read_relations(reader1);
    reader1.close();
    std::cerr << "Pass 1 done\n";

    // Output the amount of main memory used so far. All multipolygon relations
    // are in memory now.
    std::cerr << "Memory:\n";
    collector.used_memory();

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
    osmium::io::Reader reader2{input_file};
    osmium::apply(reader2, location_handler, collector.handler([&handler](osmium::memory::Buffer&& buffer) {
        osmium::apply(buffer, handler);
    }));
    reader2.close();
    std::cerr << "Pass 2 done\n";

    // Output the amount of main memory used so far. All complete multipolygon
    // relations have been cleaned up.
    std::cerr << "Memory:\n";
    collector.used_memory();

    // If there were multipolgyon relations in the input, but some of their
    // members are not in the input file (which often happens for extracts)
    // this will write the IDs of the incomplete relations to stderr.
    std::vector<const osmium::Relation*> incomplete_relations = collector.get_incomplete_relations();
    if (!incomplete_relations.empty()) {
        std::cerr << "Warning! Some member ways missing for these multipolygon relations:";
        for (const auto* relation : incomplete_relations) {
            std::cerr << " " << relation->id();
        }
        std::cerr << "\n";
    }
}


/*

  EXAMPLE osmium_tiles

  Convert WGS84 longitude and latitude to Mercator coordinates and tile
  coordinates.

  DEMONSTRATES USE OF:
  * the Location and Coordinates classes
  * the Mercator projection function
  * the Tile class

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <cstdlib>  // for std::exit, std::atoi, std::atof
#include <iostream> // for std::cout, std::cerr

// The Location contains a longitude and latitude and is usually used inside
// a node to store its location in the world.
#include <osmium/osm/location.hpp>

// Needed for the Mercator projection function. Osmium supports the Mercator
// projection out of the box, or pretty much any projection using the Proj.4
// library (with the osmium::geom::Projection class).
#include <osmium/geom/mercator_projection.hpp>

// The Tile class handles tile coordinates and zoom levels.
#include <osmium/geom/tile.hpp>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " ZOOM LON LAT\n";
        std::exit(1);
    }

    const int zoom = std::atoi(argv[1]); // NOLINT(cert-err34-c)

    if (zoom < 0 || zoom > 30) {
        std::cerr << "ERROR: Zoom must be between 0 and 30\n";
        std::exit(1);
    }

    osmium::Location location{};
    try {
        location.set_lon(argv[2]);
        location.set_lat(argv[3]);
    } catch (const osmium::invalid_location&) {
        std::cerr << "ERROR: Location is invalid\n";
        std::exit(1);
    }

    // A location can store some invalid locations, ie locations outside the
    // -180 to 180 and -90 to 90 degree range. This function checks for that.
    if (!location.valid()) {
        std::cerr << "ERROR: Location is invalid\n";
        std::exit(1);
    }

    std::cout << "WGS84:    lon=" << location.lon() << " lat=" << location.lat() << "\n";

    // Project the coordinates using a helper function. You can also use the
    // osmium::geom::MercatorProjection class.
    const osmium::geom::Coordinates c = osmium::geom::lonlat_to_mercator(location);
    std::cout << "Mercator: x=" << c.x << " y=" << c.y << "\n";

    // Create a tile at this location. This will also internally use the
    // Mercator projection and then calculate the tile coordinates.
    const osmium::geom::Tile tile{uint32_t(zoom), location};
    std::cout << "Tile:     zoom=" << tile.z << " x=" << tile.x << " y=" << tile.y << "\n";
}


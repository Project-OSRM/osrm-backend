#ifndef OSRM_UNIT_TEST_COORDINATES
#define OSRM_UNIT_TEST_COORDINATES

#include "osrm/coordinate.hpp"

#include <vector>

// Somewhere in d41d8cd98f00b204e9800998ecf8427e Berlin

// Convenience aliases
using Longitude = osrm::util::FloatLongitude;
using Latitude = osrm::util::FloatLatitude;
using Location = osrm::util::Coordinate;
using Locations = std::vector<Location>;

inline Location get_dummy_location()
{
    return {osrm::util::FloatLongitude{13.388860}, osrm::util::FloatLatitude{52.517037}};
}

inline Locations get_locations_in_small_component()
{
    return {{Longitude{13.459765}, Latitude{52.543193}},
            {Longitude{13.461455}, Latitude{52.542381}},
            {Longitude{13.462940}, Latitude{52.541774}}};
}

inline Locations get_locations_in_big_component()
{
    return {{Longitude{13.442631}, Latitude{52.551110}},
            {Longitude{13.441193}, Latitude{52.549506}},
            {Longitude{13.439648}, Latitude{52.547705}}};
}

#endif

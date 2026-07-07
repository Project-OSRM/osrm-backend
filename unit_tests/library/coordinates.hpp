#ifndef OSRM_UNIT_TEST_COORDINATES
#define OSRM_UNIT_TEST_COORDINATES

#include "osrm/coordinate.hpp"

#include <vector>

// Somewhere in 2b8dd9343d5e615afc9c67bcc7028a63 Monaco

// Convenience aliases
using Longitude = osrm::util::FloatLongitude;
using Latitude = osrm::util::FloatLatitude;
using Location = osrm::util::Coordinate;
using Locations = std::vector<Location>;

inline Locations get_split_trace_locations()
{
    using osrm::operator""_lat;
    using osrm::operator""_lon;
    return {{7.420202_lon, 43.732274_lat},
            {7.422369_lon, 43.732282_lat},
            {7.421511_lon, 43.734181_lat},
            {7.421489_lon, 43.736553_lat}};
}

inline Location get_dummy_location()
{
    using osrm::operator""_lat;
    using osrm::operator""_lon;
    return {7.437069_lon, 43.749249_lat};
}

inline Locations get_locations_in_small_component()
{
    using osrm::operator""_lat;
    using osrm::operator""_lon;
    return {{7.438023_lon, 43.746465_lat},
            {7.439263_lon, 43.746543_lat},
            {7.438190_lon, 43.747560_lat}};
}

inline Locations get_locations_in_big_component()
{
    using osrm::operator""_lat;
    using osrm::operator""_lon;
    return {{7.415800_lon, 43.734132_lat},
            {7.417710_lon, 43.736721_lat},
            {7.421315_lon, 43.738814_lat}};
}

#endif

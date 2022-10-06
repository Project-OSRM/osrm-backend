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
    return {{Longitude{7.420202}, Latitude{43.732274}},
            {Longitude{7.422369}, Latitude{43.732282}},
            {Longitude{7.421511}, Latitude{43.734181}},
            {Longitude{7.421489}, Latitude{43.736553}}};
}

inline Location get_dummy_location()
{
    return {osrm::util::FloatLongitude{7.437069}, osrm::util::FloatLatitude{43.749249}};
}

inline Locations get_locations_in_small_component()
{
    return {{Longitude{7.438023}, Latitude{43.746465}},
            {Longitude{7.439263}, Latitude{43.746543}},
            {Longitude{7.438190}, Latitude{43.747560}}};
}

inline Locations get_locations_in_big_component()
{
    return {{Longitude{7.415800}, Latitude{43.734132}},
            {Longitude{7.417710}, Latitude{43.736721}},
            {Longitude{7.421315}, Latitude{43.738814}}};
}

#endif

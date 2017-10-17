#ifndef OSRM_ENGINE_API_TABLE_RESULT
#define OSRM_ENGINE_API_TABLE_RESULT

#include "engine/api/waypoint.hpp"
#include "util/typedefs.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace api
{
struct TableResult
{
    std::vector<Waypoint> sources;
    std::vector<Waypoint> destinations;
    std::vector<EdgeWeight> durations;
};
}
}
}
#endif

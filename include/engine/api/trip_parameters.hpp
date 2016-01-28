#ifndef ENGINE_API_TRIP_PARAMETERS_HPP
#define ENGINE_API_TRIP_PARAMETERS_HPP

#include "engine/api/route_parameters.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace api
{

struct TripParameters : public RouteParameters
{
    bool IsValid() const;
};

}
}
}

#endif

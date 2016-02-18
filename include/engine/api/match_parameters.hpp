#ifndef ENGINE_API_MATCH_PARAMETERS_HPP
#define ENGINE_API_MATCH_PARAMETERS_HPP

#include "engine/api/route_parameters.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace api
{

struct MatchParameters : public RouteParameters
{
    std::vector<boost::optional<unsigned>> timestamps;
    bool IsValid() const
    {
        return timestamps.empty() || timestamps.size() == coordinates.size();
    }
};

}
}
}

#endif

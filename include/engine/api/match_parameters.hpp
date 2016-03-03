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
    MatchParameters()
        : RouteParameters(false,
                          false,
                          RouteParameters::GeometriesType::Polyline,
                          RouteParameters::OverviewType::Simplified,
                          {})
    {
    }

    template <typename... Args>
    MatchParameters(std::vector<unsigned> timestamps_, Args... args_)
        : RouteParameters{std::forward<Args>(args_)...}, timestamps{std::move(timestamps_)}
    {
    }

    std::vector<unsigned> timestamps;
    bool IsValid() const
    {
        return RouteParameters::IsValid() &&
               (timestamps.empty() || timestamps.size() == coordinates.size());
    }
};
}
}
}

#endif

#ifndef ENGINE_API_ROUTE_PARAMETERS_HPP
#define ENGINE_API_ROUTE_PARAMETERS_HPP

#include "engine/api/base_parameters.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace api
{

struct RouteParameters : public BaseParameters
{
    enum class GeometriesType
    {
        Polyline,
        GeoJSON
    };
    enum class OverviewType
    {
        Simplified,
        Full,
        False
    };

    RouteParameters() = default;

    template <typename... Args>
    RouteParameters(const bool steps_,
                    const bool alternatives_,
                    const GeometriesType geometries_,
                    const OverviewType overview_,
                    std::vector<boost::optional<bool>> uturns_,
                    Args... args_)
        : BaseParameters{std::forward<Args>(args_)...}, steps{steps_}, alternatives{alternatives_},
          geometries{geometries_}, overview{overview_}, uturns{std::move(uturns_)}
    {
    }

    bool steps = true;
    bool alternatives = true;
    GeometriesType geometries = GeometriesType::Polyline;
    OverviewType overview = OverviewType::Simplified;
    std::vector<boost::optional<bool>> uturns;

    bool IsValid() const
    {
        return coordinates.size() >= 2 && BaseParameters::IsValid() &&
               (uturns.empty() || uturns.size() == coordinates.size());
    }
};
}
}
}

#endif

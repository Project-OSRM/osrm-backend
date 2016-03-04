#ifndef ENGINE_API_BASE_PARAMETERS_HPP
#define ENGINE_API_BASE_PARAMETERS_HPP

#include "engine/hint.hpp"
#include "engine/bearing.hpp"
#include "util/coordinate.hpp"

#include <boost/optional.hpp>

#include <vector>
#include <algorithm>

namespace osrm
{
namespace engine
{
namespace api
{
struct BaseParameters
{
    std::vector<util::Coordinate> coordinates;
    std::vector<boost::optional<Hint>> hints;
    std::vector<boost::optional<double>> radiuses;
    std::vector<boost::optional<Bearing>> bearings;

    // FIXME add validation for invalid bearing values
    bool IsValid() const
    {
        return (hints.empty() || hints.size() == coordinates.size()) &&
               (bearings.empty() || bearings.size() == coordinates.size()) &&
               (radiuses.empty() || radiuses.size() == coordinates.size()) &&
               std::all_of(bearings.begin(), bearings.end(),
                           [](const boost::optional<Bearing> bearing_and_range)
                           {
                               if (bearing_and_range)
                               {
                                   return bearing_and_range->IsValid();
                               }
                               return true;
                           });
    }
};
}
}
}

#endif // ROUTE_PARAMETERS_HPP

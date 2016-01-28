#ifndef ENGINE_API_BASE_PARAMETERS_HPP
#define ENGINE_API_BASE_PARAMETERS_HPP

#include "engine/hint.hpp"
#include "util/coordinate.hpp"

#include <boost/optional.hpp>

#include <vector>

namespace osrm
{
namespace engine
{
namespace api
{
struct BaseParameters
{
    struct Bearing
    {
        short bearing;
        short range;
    };

    std::vector<util::FixedPointCoordinate> coordinates;
    std::vector<boost::optional<Hint>> hints;
    std::vector<boost::optional<double>> radiuses;
    std::vector<boost::optional<Bearing>> bearings;

    // FIXME add validation for invalid bearing values
    bool IsValid() const
    {
        return (hints.empty() || hints.size() == coordinates.size()) &&
               (bearings.empty() || bearings.size() == coordinates.size()) &&
               (radiuses.empty() || radiuses.size() == coordinates.size());
    }
};

inline bool operator==(const BaseParameters::Bearing lhs, const BaseParameters::Bearing rhs)
{
    return lhs.bearing == rhs.bearing && lhs.range == rhs.range;
}
inline bool operator!=(const BaseParameters::Bearing lhs, const BaseParameters::Bearing rhs)
{
    return !(lhs == rhs);
}
}
}
}

#endif // ROUTE_PARAMETERS_HPP

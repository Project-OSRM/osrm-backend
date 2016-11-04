#ifndef ROUTE_HPP
#define ROUTE_HPP


#include <vector>

namespace osrm
{

namespace util
{
struct Coordinate;
}

namespace engine
{
namespace guidance
{

struct RouteLeg;

struct Route
{
    double duration;
    double distance;
    std::vector<RouteLeg> legs;
    std::vector<util::Coordinate> geometry;
};
}
}
}

#endif

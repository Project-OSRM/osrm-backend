#ifndef ROUTE_HPP
#define ROUTE_HPP

namespace osrm
{
namespace engine
{
namespace guidance
{

struct Route
{
    double distance;
    double duration;
    double weight;
};
} // namespace guidance
} // namespace engine
} // namespace osrm

#endif

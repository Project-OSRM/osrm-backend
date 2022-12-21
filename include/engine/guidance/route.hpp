#ifndef ROUTE_HPP
#define ROUTE_HPP

namespace osrm::engine::guidance
{

struct Route
{
    double distance;
    double duration;
    double weight;
};
} // namespace osrm::engine::guidance

#endif

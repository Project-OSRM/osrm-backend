#include "partition/recursive_bisection.hpp"

namespace osrm
{
namespace partition
{

RecursiveBisection::RecursiveBisection(const BisectionGraph &bisection_graph_)
    : bisection_graph(bisection_graph_)
{
}

} // namespace partition
} // namespace osrm

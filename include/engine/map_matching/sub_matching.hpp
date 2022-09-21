#ifndef MAP_MATCHING_SUB_MATCHING_HPP
#define MAP_MATCHING_SUB_MATCHING_HPP

#include "engine/phantom_node.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace map_matching
{

struct SubMatching
{
    std::vector<PhantomNode> nodes;
    std::vector<unsigned> indices;
    std::vector<unsigned> alternatives_count;
    double confidence;
};
} // namespace map_matching
} // namespace engine
} // namespace osrm

#endif

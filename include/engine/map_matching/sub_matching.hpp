#ifndef MAP_MATCHING_SUB_MATCHING_HPP
#define MAP_MATCHING_SUB_MATCHING_HPP

#include "engine/phantom_node.hpp"

#include <vector>

namespace osrm::engine::map_matching
{

struct SubMatching
{
    std::vector<PhantomNode> nodes;
    std::vector<unsigned> indices;
    std::vector<unsigned> alternatives_count;
    double confidence;
};
} // namespace osrm::engine::map_matching

#endif

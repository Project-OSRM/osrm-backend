#ifndef NODE_ID_HPP
#define NODE_ID_HPP

#include "util/typedefs.hpp"

namespace osrm
{
namespace extractor
{

struct Cmp
{
    using value_type = OSMNodeID;
    bool operator()(const value_type left, const value_type right) const { return left < right; }
    value_type max_value() { return MAX_OSM_NODEID; }
    value_type min_value() { return MIN_OSM_NODEID; }
};

}
}

#endif // NODE_ID_HPP

#ifndef SUB_PATH_H_
#define SUB_PATH_H_

#include "../Util/MercatorUtil.h"

#include <boost/assert.hpp>
#include <boost/range/irange.hpp>

enum PathMonotiticy
{
    MONOTONE_INCREASING_X = 1,
    MONOTONE_INCREASING_Y = 2,
    MONOTONE_DECREASING_X = 4,
    MONOTONE_DECREASING_Y = 8,
    MONOTONE_CONSTANT_X   = 5,
    MONOTONE_CONSTANT_Y   = 10,
    MONOTONE_CONSTANT     = 15,
    MONOTONE_INVALID      = 0
};

struct SubPath
{
    enum EdgeType
    {
        EDGE_HORIZONTAL,
        EDGE_STRICTLY_HORIZONTAL,
        EDGE_VERTICAL,
        EDGE_STRICTLY_VERTICAL
    };
    typedef decltype(boost::irange(0u, 1u)) Stripe;

    SubPath(const std::vector<SymbolicCoordinate>& nodes,
            PathMonotiticy monoticity,
            unsigned input_start_idx,
            unsigned input_end_idx)
    : nodes(nodes)
    , monoticity(monoticity)
    , input_start_idx(input_start_idx)
    , input_end_idx(input_end_idx)
    {
    }

    std::vector<SymbolicCoordinate> nodes;
    PathMonotiticy monoticity;

    // save index in unschematized path
    unsigned input_start_idx;
    unsigned input_end_idx;

    // needed for schematization
    std::vector<unsigned>          y_order_to_node;
    std::vector<unsigned>          node_to_y_order;
    std::vector<SubPath::Stripe>   stripes;
    std::vector<SubPath::EdgeType> edge_types;
    std::vector<unsigned>          prefered_directions;
    std::vector<unsigned>          alternative_directions;

    // offset for coordinates
    double delta_x;
    double delta_y;
};

#endif

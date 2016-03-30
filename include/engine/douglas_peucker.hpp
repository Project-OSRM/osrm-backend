#ifndef DOUGLAS_PEUCKER_HPP_
#define DOUGLAS_PEUCKER_HPP_

#include "util/coordinate.hpp"

#include <vector>
#include <iterator>

namespace osrm
{
namespace engine
{
namespace detail
{
const constexpr int DOUGLAS_PEUCKER_THRESHOLDS[19] = {
    512440, // z0
    256720, // z1
    122560, // z2
    56780,  // z3
    28800,  // z4
    14400,  // z5
    7200,   // z6
    3200,   // z7
    2400,   // z8
    1000,   // z9
    600,    // z10
    120,    // z11
    60,     // z12
    45,     // z13
    36,     // z14
    20,     // z15
    8,      // z16
    6,      // z17
    4,      // z18
};

const constexpr auto DOUGLAS_PEUCKER_THRESHOLDS_SIZE =
    sizeof(DOUGLAS_PEUCKER_THRESHOLDS) / sizeof(*DOUGLAS_PEUCKER_THRESHOLDS);
} // ns detail

// These functions compute the bitvector of indicating generalized input
// points according to the (Ramer-)Douglas-Peucker algorithm.
//
// Input is vector of pairs. Each pair consists of the point information and a
// bit indicating if the points is present in the generalization.
// Note: points may also be pre-selected*/
std::vector<util::Coordinate> douglasPeucker(std::vector<util::Coordinate>::const_iterator begin,
                                             std::vector<util::Coordinate>::const_iterator end,
                                             const unsigned zoom_level);

// Convenience range-based function
inline std::vector<util::Coordinate> douglasPeucker(const std::vector<util::Coordinate> &geometry,
                                                    const unsigned zoom_level)
{
    return douglasPeucker(begin(geometry), end(geometry), zoom_level);
}
}
}

#endif /* DOUGLAS_PEUCKER_HPP_ */

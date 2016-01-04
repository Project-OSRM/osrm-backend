#ifndef DOUGLAS_PEUCKER_HPP_
#define DOUGLAS_PEUCKER_HPP_

#include "engine/segment_information.hpp"

#include <array>
#include <stack>
#include <utility>
#include <vector>

/* This class object computes the bitvector of indicating generalized input
 * points according to the (Ramer-)Douglas-Peucker algorithm.
 *
 * Input is vector of pairs. Each pair consists of the point information and a
 * bit indicating if the points is present in the generalization.
 * Note: points may also be pre-selected*/

static const std::array<int, 19> DOUGLAS_PEUCKER_THRESHOLDS{{
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
    4       // z18
}};

class DouglasPeucker
{
  public:
    using RandomAccessIt = std::vector<SegmentInformation>::iterator;

    using GeometryRange = std::pair<RandomAccessIt, RandomAccessIt>;
    // Stack to simulate the recursion
    std::stack<GeometryRange> recursion_stack;

  public:
    void Run(RandomAccessIt begin, RandomAccessIt end, const unsigned zoom_level);
    void Run(std::vector<SegmentInformation> &input_geometry, const unsigned zoom_level);
};

#endif /* DOUGLAS_PEUCKER_HPP_ */

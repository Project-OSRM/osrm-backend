#ifndef DOUGLAS_PEUCKER_HPP_
#define DOUGLAS_PEUCKER_HPP_

#include "util/coordinate.hpp"

#include <iterator>
#include <vector>

namespace osrm
{
namespace engine
{
namespace detail
{

// This is derived from the following formular:
// x = b * (1 + lon/180) => dx = b * dlon/180
// y = b * (1 - lat/180) => dy = b * dlat/180
// dx^2 + dy^2 < min_pixel^2
// => dlon^2 + dlat^2 < min_pixel^2 / b^2 * 180^2
inline std::vector<std::uint64_t> generateThreshold(double min_pixel, unsigned number_of_zoomlevels)
{
    std::vector<std::uint64_t> thresholds(number_of_zoomlevels);
    for (unsigned zoom = 0; zoom < number_of_zoomlevels; ++zoom)
    {
        const double shift = (1u << zoom) * 256;
        const double b = shift / 2.0;
        const double pixel_to_deg = 180. / b;
        const std::uint64_t min_deg = min_pixel * pixel_to_deg * COORDINATE_PRECISION;
        thresholds[zoom] = min_deg * min_deg;
    }

    return thresholds;
}

const constexpr std::uint64_t DOUGLAS_PEUCKER_THRESHOLDS[19] = {
    49438476562500, // z0
    12359619140625, // z1
    3089903027344,  // z2
    772475756836,   // z3
    193118939209,   // z4
    48279515076,    // z5
    12069878769,    // z6
    3017414761,     // z7
    754326225,      // z8
    188567824,      // z9
    47141956,       // z10
    11785489,       // z11
    2944656,        // z12
    736164,         // z13
    184041,         // z14
    45796,          // z15
    11449,          // z16
    2809,           // z17
    676,            // z18
};

const constexpr auto DOUGLAS_PEUCKER_THRESHOLDS_SIZE =
    sizeof(DOUGLAS_PEUCKER_THRESHOLDS) / sizeof(*DOUGLAS_PEUCKER_THRESHOLDS);
} // namespace detail

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
} // namespace engine
} // namespace osrm

#endif /* DOUGLAS_PEUCKER_HPP_ */

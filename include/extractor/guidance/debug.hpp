#ifndef OSRM_EXTRACTOR_GUIDANCE_DEBUG_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_DEBUG_HPP_

#include <iomanip>
#include <iostream>
#include <string>

namespace osrm
{
namespace extractor
{
namespace guidance
{

inline void print(const LaneDataVector &turn_lane_data)
{
    std::cout << " Tags:\n";
    for (auto entry : turn_lane_data)
        std::cout << "\t" << entry.tag << " from: " << static_cast<int>(entry.from)
                  << " to: " << static_cast<int>(entry.to) << "\n";
    std::cout << std::flush;
}

inline void printTurnAssignmentData(const NodeID at,
                                    const LaneDataVector &turn_lane_data,
                                    const Intersection &intersection,
                                    const std::vector<QueryNode> &node_info_list)
{
    std::cout << "[Turn Assignment Progress]\nLocation:";
    auto coordinate = node_info_list[at];
    std::cout << std::setprecision(12) << toFloating(coordinate.lat) << " "
              << toFloating(coordinate.lon) << "\n";

    std::cout << "  Intersection:\n";
    for (const auto &road)
        std::cout << "\t" << toString(road) << "\n";

    //flushes as well
    print(turn_lane_data);
}

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /* OSRM_EXTRACTOR_GUIDANCE_DEBUG_HPP_ */

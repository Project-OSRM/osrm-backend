#ifndef OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_DATA_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_DATA_HPP_

#include "util/typedefs.hpp"
#include <string>
#include <vector>

namespace osrm
{
namespace extractor
{
namespace guidance
{
namespace lanes
{

struct TurnLaneData
{
    std::string tag;
    LaneID from;
    LaneID to;

    bool operator<(const TurnLaneData &other) const;
};
typedef std::vector<TurnLaneData> LaneDataVector;

// convertes a string given in the OSM format into a TurnLaneData vector
LaneDataVector laneDataFromString(std::string turn_lane_string);

// Locate A Tag in a lane data vector
LaneDataVector::const_iterator findTag(const std::string &tag, const LaneDataVector &data);
LaneDataVector::iterator findTag(const std::string &tag, LaneDataVector &data);

bool hasTag(const std::string &tag, const LaneDataVector &data);

} // namespace lane_data_generation

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /* OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_DATA_HPP_ */

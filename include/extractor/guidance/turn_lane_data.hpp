#ifndef OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_DATA_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_DATA_HPP_

#include "util/typedefs.hpp"
#include "extractor/guidance/turn_lane_types.hpp"
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
    TurnLaneType::Mask tag;
    LaneID from;
    LaneID to;

    bool operator<(const TurnLaneData &other) const;
};
typedef std::vector<TurnLaneData> LaneDataVector;

// convertes a string given in the OSM format into a TurnLaneData vector
LaneDataVector laneDataFromDescription(const TurnLaneDescription &turn_lane_description);

// Locate A Tag in a lane data vector (if multiple tags are set, the first one found is returned)
LaneDataVector::const_iterator findTag(const TurnLaneType::Mask tag, const LaneDataVector &data);
LaneDataVector::iterator findTag(const TurnLaneType::Mask tag, LaneDataVector &data);

// Returns true if any of the queried tags is contained
bool hasTag(const TurnLaneType::Mask tag, const LaneDataVector &data);

} // namespace lane_data_generation

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /* OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_DATA_HPP_ */

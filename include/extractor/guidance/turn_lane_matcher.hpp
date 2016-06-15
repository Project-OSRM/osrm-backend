#ifndef OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_MATCHER_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_MATCHER_HPP_

#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/toolkit.hpp"
#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/guidance/turn_lane_data.hpp"

#include "util/guidance/turn_lanes.hpp"
#include "util/node_based_graph.hpp"

#include <unordered_map>

namespace osrm
{
namespace extractor
{
namespace guidance
{
namespace lanes
{

// Translate Turn Lane Tags into a matching modifier
DirectionModifier::Enum getMatchingModifier(const std::string &tag);

// check whether a match of a given tag and a turn instruction can be seen as valid
bool isValidMatch(const std::string &tag, const TurnInstruction instruction);

// Every tag is somewhat idealized in form of the expected angle. A through lane should go straight
// (or follow a 180 degree turn angle between in/out segments.) The following function tries to find
// the best possible match for every tag in a given intersection, considering a few corner cases
// introduced to OSRM handling u-turns
typename Intersection::const_iterator findBestMatch(const std::string &tag,
                                                    const Intersection &intersection);
// Reverse is a special case, because it requires access to the leftmost tag. It has its own
// matching function as a result of that. The leftmost tag is required, since u-turns are disabled
// by default in OSRM. Therefor we cannot check whether a turn is allowed, since it could be
// possible that it is forbidden. In addition, the best u-turn angle does not necessarily represent
// the u-turn, since it could be a sharp-left turn instead on a road with a middle island.
typename Intersection::const_iterator findBestMatchForReverse(const std::string &leftmost_tag,
                                                              const Intersection &intersection);

// a match is trivial if all turns can be associated with their best match in a valid way and the
// matches occur in order
bool canMatchTrivially(const Intersection &intersection, const LaneDataVector &lane_data);

// perform a trivial match on the turn lanes
Intersection triviallyMatchLanesToTurns(Intersection intersection,
                                        const LaneDataVector &lane_data,
                                        const util::NodeBasedDynamicGraph &node_based_graph,
                                        const LaneStringID lane_string_id,
                                        LaneDataIdMap &lane_data_to_id);

} // namespace lanes
} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /*OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_MATCHER_HPP_*/

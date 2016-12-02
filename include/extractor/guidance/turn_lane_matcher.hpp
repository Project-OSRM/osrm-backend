#ifndef OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_MATCHER_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_MATCHER_HPP_

#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/guidance/turn_lane_data.hpp"

#include "util/attributes.hpp"
#include "util/guidance/turn_lanes.hpp"
#include "util/node_based_graph.hpp"

namespace osrm
{
namespace extractor
{
namespace guidance
{
namespace lanes
{

// Translate Turn Lane Tags into a matching modifier
DirectionModifier::Enum getMatchingModifier(const TurnLaneType::Mask tag);

// check whether a match of a given tag and a turn instruction can be seen as valid
bool isValidMatch(const TurnLaneType::Mask tag, const TurnInstruction instruction);

// localisation of the best possible match for a tag
typename Intersection::const_iterator findBestMatch(const TurnLaneType::Mask tag,
                                                    const Intersection &intersection);

// the quality of a matching to decide between first/second possibility on segregated intersections
double getMatchingQuality(const TurnLaneType::Mask tag, const ConnectedRoad &road);

typename Intersection::const_iterator findBestMatchForReverse(const TurnLaneType::Mask leftmost_tag,
                                                              const Intersection &intersection);

// a match is trivial if all turns can be associated with their best match in a valid way and the
// matches occur in order
bool canMatchTrivially(const Intersection &intersection, const LaneDataVector &lane_data);

// perform a trivial match on the turn lanes
OSRM_ATTR_WARN_UNUSED
Intersection triviallyMatchLanesToTurns(Intersection intersection,
                                        const LaneDataVector &lane_data,
                                        const util::NodeBasedDynamicGraph &node_based_graph,
                                        const LaneDescriptionID lane_string_id,
                                        util::guidance::LaneDataIdMap &lane_data_to_id);

} // namespace lanes
} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /*OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_MATCHER_HPP_*/

#ifndef OSRM_GUIDANCE_INTERSECTION_HPP_
#define OSRM_GUIDANCE_INTERSECTION_HPP_

#include "extractor/intersection/intersection_view.hpp"
#include "guidance/turn_instruction.hpp"

namespace osrm::guidance
{

// A Connected Road is the internal representation of a potential turn. Internally, we require
// full list of all connected roads to determine the outcome.
// The reasoning behind is that even invalid turns can influence the perceived angles, or even
// instructions themselves. An possible example can be described like this:
//
// aaa(2)aa
//          a - bbbbb
// aaa(1)aa
//
// will not be perceived as a turn from (1) -> b, and as a U-turn from (1) -> (2).
// In addition, they can influence whether a turn is obvious or not. b->(2) would also be no
// turn-operation, but rather a name change.
//
// If this were a normal intersection with
//
// cccccccc
//            o  bbbbb
// aaaaaaaa
//
// We would perceive a->c as a sharp turn, a->b as a slight turn, and b->c as a slight turn.
struct ConnectedRoad final : extractor::intersection::IntersectionViewData
{
    ConnectedRoad(const extractor::intersection::IntersectionViewData &view,
                  const TurnInstruction instruction,
                  const LaneDataID lane_data_id)
        : IntersectionViewData(view), instruction(instruction), lane_data_id(lane_data_id)
    {
    }

    TurnInstruction instruction;
    LaneDataID lane_data_id;

    // used to sort the set of connected roads (we require sorting throughout turn handling)
    bool compareByAngle(const ConnectedRoad &other) const;

    // make a left turn into an equivalent right turn and vice versa
    void mirror()
    {
        const constexpr DirectionModifier::Enum mirrored_modifiers[] = {
            DirectionModifier::UTurn,
            DirectionModifier::SharpLeft,
            DirectionModifier::Left,
            DirectionModifier::SlightLeft,
            DirectionModifier::Straight,
            DirectionModifier::SlightRight,
            DirectionModifier::Right,
            DirectionModifier::SharpRight};

        static_assert(
            sizeof(mirrored_modifiers) / sizeof(DirectionModifier::Enum) ==
                DirectionModifier::MaxDirectionModifier,
            "The list of mirrored modifiers needs to match the available modifiers in size.");

        if (util::angularDeviation(angle, 0) > std::numeric_limits<double>::epsilon())
        {
            angle = 360 - angle;
            instruction.direction_modifier = mirrored_modifiers[instruction.direction_modifier];
        }
    }
};

// small helper function to print the content of a connected road
std::string toString(const ConnectedRoad &road);

// `Intersection` is a relative view of an intersection by an incoming edge.
// `Intersection` are streets at an intersection stored as an ordered list of connected roads
// ordered from sharp right counter-clockwise to
// sharp left where `intersection[0]` is _always_ a u-turn

// An intersection is an ordered list of connected roads ordered from sharp right
// counter-clockwise to sharp left where `intersection[0]` is always a u-turn
//
//                                           |
//                                           |
//                                     (intersec[3])
//                                           |
//                                           |
//                                           |
//  nid ---(via_eid/intersec[0])--- nbg.GetTarget(via)  ---(intersec[2])---
//                                           |
//                                           |
//                                           |
//                                     (intersec[1])
//                                           |
//                                           |
//
// intersec := intersection
// nbh := node_based_graph
//
struct Intersection final : std::vector<ConnectedRoad>,                                  //
                            extractor::intersection::EnableShapeOps<Intersection>,       //
                            extractor::intersection::EnableIntersectionOps<Intersection> //
{
    using Base = std::vector<ConnectedRoad>;
};

inline std::string toString(const ConnectedRoad &road)
{
    std::string result = "[connection] ";
    result += std::to_string(road.eid);
    result += " allows entry: ";
    result += std::to_string(road.entry_allowed);
    result += " angle: ";
    result += std::to_string(road.angle);
    result += " bearing: ";
    result += std::to_string(road.perceived_bearing);
    result += " instruction: ";
    result += std::to_string(static_cast<std::int32_t>(road.instruction.type)) + " " +
              std::to_string(static_cast<std::int32_t>(road.instruction.direction_modifier)) + " " +
              std::to_string(static_cast<std::int32_t>(road.lane_data_id));
    return result;
}

} // namespace osrm::guidance

#endif /* OSRM_GUIDANCE_INTERSECTION_HPP_*/

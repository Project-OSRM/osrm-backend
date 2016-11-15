#ifndef OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_HPP_

#include <string>
#include <vector>

#include "extractor/guidance/turn_instruction.hpp"
#include "util/guidance/toolkit.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp" // EdgeID

#include <boost/optional.hpp>

namespace osrm
{
namespace extractor
{
namespace guidance
{

// Every Turn Operation describes a way of switching onto a segment, indicated by an EdgeID. The
// associated turn is described by an angle and an instruction that is used to announce it.
// The Turn Operation indicates what is exposed to the outside of the turn analysis.
struct TurnOperation
{
    EdgeID eid;
    double angle;
    double bearing;
    TurnInstruction instruction;
    LaneDataID lane_data_id;
};

// A Connected Road is the internal representation of a potential turn. Internally, we require
// full list of all connected roads to determine the outcome.
// The reasoning behind is that even invalid turns can influence the perceived angles, or even
// instructions themselves. An pososible example can be described like this:
//
// aaa(2)aa
//          a - bbbbb
// aaa(1)aa
//
// will not be perceived as a turn from (1) -> b, and as a U-turn from (1) -> (2).
// In addition, they can influence whether a turn is obvious or not. b->(2) would also be no
// turn-operation,
// but rather a name change.
//
// If this were a normal intersection with
//
// cccccccc
//            o  bbbbb
// aaaaaaaa
//
// We would perceive a->c as a sharp turn, a->b as a slight turn, and b->c as a slight turn.
struct ConnectedRoad final : public TurnOperation
{
    using Base = TurnOperation;

    ConnectedRoad(const TurnOperation turn,
                  const bool entry_allowed = false,
                  const boost::optional<double> segment_length = {});

    // a turn may be relevant to good instructions, even if we cannot enter the road
    bool entry_allowed;
    boost::optional<double> segment_length;

    // used to sort the set of connected roads (we require sorting throughout turn handling)
    bool compareByAngle(const ConnectedRoad &other) const;

    // make a left turn into an equivalent right turn and vice versa
    void mirror();

    OSRM_ATTR_WARN_UNUSED
    ConnectedRoad getMirroredCopy() const;
};

// small helper function to print the content of a connected road
std::string toString(const ConnectedRoad &road);

struct Intersection final : public std::vector<ConnectedRoad>
{
    using Base = std::vector<ConnectedRoad>;

    /*
     * find the turn whose angle offers the least angularDeviation to the specified angle
     * E.g. for turn angles [0,90,260] and a query of 180 we return the 260 degree turn (difference
     * 80 over the difference of 90 to the 90 degree turn)
     */
    Base::iterator findClosestTurn(double angle);
    Base::const_iterator findClosestTurn(double angle) const;

    /*
     * Check validity of the intersection object. We assume a few basic properties every set of
     * connected roads should follow throughout guidance pre-processing. This utility function
     * allows checking intersections for validity
     */
    bool valid() const;

    // given all possible turns, which is the highest connected number of lanes per turn. This value
    // is used, for example, during generation of intersections.
    std::uint8_t getHighestConnectedLaneCount(const util::NodeBasedDynamicGraph &) const;
};

Intersection::const_iterator findClosestTurn(const Intersection &intersection, const double angle);
Intersection::iterator findClosestTurn(Intersection &intersection, const double angle);

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /*OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_HPP_*/

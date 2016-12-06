#ifndef OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_HPP_

#include <algorithm>
#include <functional>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

#include "util/bearing.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp" // EdgeID

#include "extractor/guidance/turn_instruction.hpp"

#include <boost/range/algorithm/min_element.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/count_if.hpp>

namespace osrm
{
namespace extractor
{
namespace guidance
{

// the shape of an intersection only knows about edge IDs and bearings
struct IntersectionShapeData
{
    EdgeID eid;
    double bearing;
    double segment_length;
};

inline auto makeCompareShapeDataByBearing(const double base_bearing)
{
    return [base_bearing](const auto &lhs, const auto &rhs) {
        return util::bearing::angleBetween(lhs.bearing, base_bearing) <
               util::bearing::angleBetween(rhs.bearing, base_bearing);
    };
}

inline auto makeCompareAngularDeviation(const double angle)
{
    return [angle](const auto &lhs, const auto &rhs) {
        return util::angularDeviation(lhs.angle, angle) < util::angularDeviation(rhs.angle, angle);
    };
}

inline auto makeExtractLanesForRoad(const util::NodeBasedDynamicGraph &node_based_graph)
{
    return [&node_based_graph](const auto &road) {
        return node_based_graph.GetEdgeData(road.eid).road_classification.GetNumberOfLanes();
    };
}

// When viewing an intersection from an incoming edge, we can transform a shape into a view which
// gives additional information on angles and whether a turn is allowed
struct IntersectionViewData : IntersectionShapeData
{
    IntersectionViewData(const IntersectionShapeData &shape,
                         const bool entry_allowed,
                         const double angle)
        : IntersectionShapeData(shape), entry_allowed(entry_allowed), angle(angle)
    {
    }

    bool entry_allowed;
    double angle;

    bool CompareByAngle(const IntersectionViewData &other) const;
};

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
struct ConnectedRoad final : IntersectionViewData
{
    ConnectedRoad(const IntersectionViewData &view,
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
    void mirror();

    OSRM_ATTR_WARN_UNUSED
    ConnectedRoad getMirroredCopy() const;
};

// small helper function to print the content of a connected road
std::string toString(const IntersectionShapeData &shape);
std::string toString(const IntersectionViewData &view);
std::string toString(const ConnectedRoad &road);

// Intersections are sorted roads: [0] being the UTurn road, then from sharp right to sharp left.
// common operations shared amongst all intersection types
template <typename Self> struct EnableShapeOps
{
    // same as closest turn, but for bearings
    auto FindClosestBearing(double bearing) const
    {
        auto comp = makeCompareShapeDataByBearing(bearing);
        return std::min_element(self()->begin(), self()->end(), comp);
    }

    // search a given eid in the intersection
    auto FindEid(const EdgeID eid) const
    {
        return boost::range::find_if(
            *self(), [eid](const auto &road) { return road.eid == eid; });
    }

    // find the maximum value based on a conversion operator
    template <typename UnaryProjection> auto FindMaximum(UnaryProjection converter) const
    {
        BOOST_ASSERT(!self()->empty());
        auto initial = converter(self()->front());

        const auto extract_maximal_value = [&initial, converter](const auto &road) {
            initial = std::max(initial, converter(road));
            return false;
        };

        boost::range::find_if(*self(), extract_maximal_value);
        return initial;
    }

    // find the maximum value based on a conversion operator and a predefined initial value
    template <typename UnaryPredicate> auto Count(UnaryPredicate detector) const
    {
        BOOST_ASSERT(!self()->empty());
        return boost::range::count_if(*self(), detector);
    }

  private:
    auto self() { return static_cast<Self *>(this); }
    auto self() const { return static_cast<const Self *>(this); }
};

struct IntersectionShape final : std::vector<IntersectionShapeData>, //
                                 EnableShapeOps<IntersectionShape>   //
{
    using Base = std::vector<IntersectionShapeData>;
};

// Common operations shared among IntersectionView and Intersections.
// Inherit to enable those operations on your compatible type. CRTP pattern.
template <typename Self> struct EnableIntersectionOps
{
    // Find the turn whose angle offers the least angular deviation to the specified angle
    // For turn angles [0, 90, 260] and a query of 180 we return the 260 degree turn.
    auto findClosestTurn(double angle) const
    {
        auto comp = makeCompareAngularDeviation(angle);
        return boost::range::min_element(*self(), comp);
    }

    /* Check validity of the intersection object. We assume a few basic properties every set of
     * connected roads should follow throughout guidance pre-processing. This utility function
     * allows checking intersections for validity
     */
    auto valid() const
    {
        if (self()->empty())
            return false;

        auto comp = [](const auto &lhs, const auto &rhs) { return lhs.CompareByAngle(rhs); };

        const auto ordered = std::is_sorted(self()->begin(), self()->end(), comp);

        if (!ordered)
            return false;

        const auto uturn = self()->operator[](0).angle < std::numeric_limits<double>::epsilon();

        if (!uturn)
            return false;

        return true;
    }

    // Returns the UTurn road we took to arrive at this intersection.
    const auto &getUTurnRoad() const { return self()->operator[](0); }

    // Returns the right-most road at this intersection.
    const auto &getRightmostRoad() const
    {
        return self()->size() > 1 ? self()->operator[](1) : self()->getUTurnRoad();
    }

    // Returns the left-most road at this intersection.
    const auto &getLeftmostRoad() const
    {
        return self()->size() > 1 ? self()->back() : self()->getUTurnRoad();
    }

    // Can this be skipped over?
    auto isTrafficSignalOrBarrier() const { return self()->size() == 2; }

    // Checks if there is at least one road available (except UTurn road) on which to continue.
    auto isDeadEnd() const
    {
        auto pred = [](const auto &road) { return road.entry_allowed; };
        return std::none_of(self()->begin() + 1, self()->end(), pred);
    }

    // Returns the number of roads we can enter at this intersection, respectively.
    auto countEnterable() const
    {
        auto pred = [](const auto &road) { return road.entry_allowed; };
        return boost::range::count_if(*self(), pred);
    }

    // Returns the number of roads we can not enter at this intersection, respectively.
    auto countNonEnterable() const { return self()->size() - self()->countEnterable(); }

    // same as find closests turn but with an additional predicate to allow filtering
    // the filter has to return `true` for elements that should be ignored
    template <typename UnaryPredicate>
    auto findClosestTurn(const double angle, const UnaryPredicate filter) const
    {
        BOOST_ASSERT(!self()->empty());
        const auto candidate = boost::range::min_element(
            *self(), [angle, &filter](const auto &lhs, const auto &rhs) {
                const auto filtered_lhs = filter(lhs), filtered_rhs = filter(rhs);
                const auto deviation_lhs = util::angularDeviation(lhs.angle, angle),
                           deviation_rhs = util::angularDeviation(rhs.angle, angle);
                return std::tie(filtered_lhs, deviation_lhs) <
                       std::tie(filtered_rhs, deviation_rhs);
            });

        // make sure only to return valid elements
        return filter(*candidate) ? self()->end() : candidate;
    }

  private:
    auto self() { return static_cast<Self *>(this); }
    auto self() const { return static_cast<const Self *>(this); }
};

struct IntersectionView final : std::vector<IntersectionViewData>,      //
                                EnableShapeOps<IntersectionView>,       //
                                EnableIntersectionOps<IntersectionView> //
{
    using Base = std::vector<IntersectionViewData>;
};

struct Intersection final : std::vector<ConnectedRoad>,         //
                            EnableShapeOps<Intersection>,       //
                            EnableIntersectionOps<Intersection> //
{
    using Base = std::vector<ConnectedRoad>;
};

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /*OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_HPP_*/

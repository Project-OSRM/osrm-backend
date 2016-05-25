#include "extractor/guidance/turn_classification.hpp"

#include "util/simple_logger.hpp"

#include <cstddef>
#include <cstdint>
#include <iomanip>

namespace osrm
{
namespace extractor
{
namespace guidance
{

struct TurnPossibility
{
    TurnPossibility(bool entry_allowed, double bearing)
        : entry_allowed(entry_allowed), bearing(std::move(bearing))
    {
    }

    TurnPossibility() : entry_allowed(false), bearing(0) {}

    bool entry_allowed;
    double bearing;
};

std::pair<util::guidance::EntryClass, util::guidance::BearingClass>
classifyIntersection(NodeID nid,
                     const Intersection &intersection,
                     const util::NodeBasedDynamicGraph &node_based_graph,
                     const extractor::CompressedEdgeContainer &compressed_geometries,
                     const std::vector<extractor::QueryNode> &query_nodes)
{
    if (intersection.empty())
        return {};

    std::vector<TurnPossibility> turns;

    const auto node_coordinate = util::Coordinate(query_nodes[nid].lon, query_nodes[nid].lat);

    // generate a list of all turn angles between a base edge, the node and a current edge
    for (const auto &road : intersection)
    {
        const auto eid = road.turn.eid;
        const auto edge_coordinate = getRepresentativeCoordinate(
            nid, node_based_graph.GetTarget(eid), eid, false, compressed_geometries, query_nodes);

        const double bearing =
            util::coordinate_calculation::bearing(node_coordinate, edge_coordinate);
        turns.push_back({road.entry_allowed, bearing});
    }

    std::sort(turns.begin(), turns.end(),
              [](const TurnPossibility left, const TurnPossibility right) {
                  return left.bearing < right.bearing;
              });

    util::guidance::EntryClass entry_class;
    util::guidance::BearingClass bearing_class;

    const bool canBeDiscretized = [&]() {
        if (turns.size() <= 1)
            return true;

        DiscreteBearing last_discrete_bearing =
            util::guidance::BearingClass::getDiscreteBearing(std::round(turns.back().bearing));
        for (const auto turn : turns)
        {
            const DiscreteBearing discrete_bearing =
                util::guidance::BearingClass::getDiscreteBearing(std::round(turn.bearing));
            if (discrete_bearing == last_discrete_bearing)
                return false;
            last_discrete_bearing = discrete_bearing;
        }
        return true;
    }();

    // finally transfer data to the entry/bearing classes
    std::size_t number = 0;
    if (canBeDiscretized)
    {
        if(util::guidance::BearingClass::getDiscreteBearing(turns.back().bearing) <
            util::guidance::BearingClass::getDiscreteBearing(turns.front().bearing))
        {
            turns.insert(turns.begin(), turns.back());
            turns.pop_back();
        }
        for (const auto turn : turns)
        {
            if (turn.entry_allowed)
                entry_class.activate(number);
            auto discrete_bearing_class =
                util::guidance::BearingClass::getDiscreteBearing(std::round(turn.bearing));
            bearing_class.add(std::round(discrete_bearing_class *
                                         util::guidance::BearingClass::discrete_step_size));
            ++number;
        }
    }
    else
    {
        for (const auto turn : turns)
        {
            if (turn.entry_allowed)
                entry_class.activate(number);
            bearing_class.add(std::round(turn.bearing));
            ++number;
        }
    }
    return std::make_pair(entry_class, bearing_class);
}

} // namespace guidance
} // namespace extractor
} // namespace osrm

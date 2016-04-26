#include "extractor/guidance/turn_classification.hpp"

#include "util/simple_logger.hpp"

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

    if (turns.empty())
        return {};

    std::sort(turns.begin(), turns.end(),
              [](const TurnPossibility left, const TurnPossibility right) {
                  return left.bearing < right.bearing;
              });

    util::guidance::EntryClass entry_class;
    util::guidance::BearingClass bearing_class;

    std::size_t turn_index = 0;
    for (std::size_t i = 0; i < turns.size(); ++i)
    {
        if (turns[i].entry_allowed)
            entry_class.activate(turn_index);
        if (bearing_class.addContinuous(turns[i].bearing))
            ++turn_index;
    }

    if (turn_index != turns.size())
    {
        util::SimpleLogger().Write(logDEBUG)
            << "Failed to provide full turn list for intersection: " << turn_index << " roads of "
            << turns.size() << " mapped.";
        for (auto turn : turns)
            std::cout << " " << (int)turn.bearing << " ("
                      << (int)util::guidance::BearingClass::discreteBearingID(turn.bearing) << ")";
        std::cout << std::endl;
        for (const auto &road : intersection)
        {
            const auto eid = road.turn.eid;
            const auto edge_coordinate =
                getRepresentativeCoordinate(nid, node_based_graph.GetTarget(eid), eid, false,
                                            compressed_geometries, query_nodes);

            const double bearing =
                util::coordinate_calculation::bearing(node_coordinate, edge_coordinate);
            std::cout << " " << bearing;
        }
        std::cout << std::endl;
    }

    return std::make_pair(entry_class, bearing_class);
}

} // namespace guidance
} // namespace extractor
} // namespace osrm

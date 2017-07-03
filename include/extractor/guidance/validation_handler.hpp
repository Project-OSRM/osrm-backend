#ifndef OSRM_EXTRACTOR_GUIDANCE_VALIDATION_HANDLER_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_VALIDATION_HANDLER_HPP_

#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/intersection_generator.hpp"
#include "extractor/guidance/intersection_handler.hpp"
#include "extractor/guidance/road_classification.hpp"
#include "extractor/packed_osm_ids.hpp"

#include "util/coordinate.hpp"
#include "util/guidance/name_announcements.hpp"
#include "util/name_table.hpp"
#include "util/node_based_graph.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace osrm
{
namespace extractor
{
namespace guidance
{

// Runs sanity checks on intersections and dumps out suspicious ones.
class ValidationHandler final : public IntersectionHandler
{
    const extractor::PackedOSMIDs &osm_node_ids;

    struct TurnDiagnostic
    {
        NodeID from, via, to;
        std::string handler;
        std::string what;

        auto as_tuple() const { return std::tie(from, via, to, handler, what); }

        friend bool operator<(const TurnDiagnostic &lhs, const TurnDiagnostic &rhs)
        {
            return lhs.as_tuple() < rhs.as_tuple();
        }

        friend bool operator==(const TurnDiagnostic &lhs, const TurnDiagnostic &rhs)
        {
            return lhs.as_tuple() == rhs.as_tuple();
        }
    };

    mutable std::vector<TurnDiagnostic> turn_diagnostics;
    mutable std::mutex turn_diagnostics_lock;

  public:
    ValidationHandler(const IntersectionGenerator &intersection_generator,
                      const util::NodeBasedDynamicGraph &node_based_graph,
                      const std::vector<util::Coordinate> &coordinates,
                      const extractor::PackedOSMIDs &osm_node_ids,
                      const util::NameTable &name_table,
                      const SuffixTable &street_name_suffix_table)
        : IntersectionHandler(node_based_graph,
                              coordinates,
                              name_table,
                              street_name_suffix_table,
                              intersection_generator),
          osm_node_ids(osm_node_ids)
    {
    }

    ~ValidationHandler() override final
    {
        std::lock_guard<std::mutex> defer{turn_diagnostics_lock};

        std::sort(begin(turn_diagnostics), end(turn_diagnostics));
        auto it = std::unique(begin(turn_diagnostics), end(turn_diagnostics));
        turn_diagnostics.erase(it, end(turn_diagnostics));

        for (auto &diag : turn_diagnostics)
        {
            printTurnInfo(diag.from, diag.via, diag.to, diag.handler, diag.what);
        }
    }

    bool canProcess(const NodeID, const EdgeID, const Intersection &) const override final
    {
        return true;
    }

    Intersection operator()(const NodeID nid,
                            const EdgeID via_eid,
                            Intersection intersection) const override final
    {
        checkForSharpTurnsOntoRamps(nid, via_eid, intersection);
        checkForSharpTurnsBetweenRamps(nid, via_eid, intersection);
        // checkForSharpTurnsOnFastRoads(nid, via_eid, intersection);
        checkForSharpDualCarriagewayUturns(nid, via_eid, intersection);

        return intersection;
    }

  private:
    // Assumed high speed on this edge
    bool isFastRoad(const EdgeID edge) const
    {
        const auto road_class = node_based_graph.GetEdgeData(edge).road_classification.GetClass();

        const RoadPriorityClass::Enum fast_classes[] = {
            RoadPriorityClass::MOTORWAY,
            RoadPriorityClass::TRUNK,
            RoadPriorityClass::PRIMARY,
            RoadPriorityClass::SECONDARY,
        };

        const auto it = std::find(std::begin(fast_classes), std::end(fast_classes), road_class);
        const auto is_fast_road = it != std::end(fast_classes);

        return is_fast_road;
    }

    void printTurnInfo(const NodeID from,
                       const NodeID via,
                       const NodeID to,
                       const std::string &handler,
                       const std::string &what) const
    {
        std::stringstream fmt;
        fmt << std::setprecision(12);

        fmt << osm_node_ids[from] << "," << osm_node_ids[via] << "," << osm_node_ids[to];
        fmt << "," << util::toFloating(coordinates[from].lon) << ","
            << util::toFloating(coordinates[from].lat);
        fmt << "," << util::toFloating(coordinates[via].lon) << ","
            << util::toFloating(coordinates[via].lat);
        fmt << "," << util::toFloating(coordinates[to].lon) << ","
            << util::toFloating(coordinates[to].lat);
        fmt << ",\"" << handler << "\"," << what;

        // Note: single operator<< cout call to not garble output running
        // multi-threaded; no endl to not flush the stream after every turn
        std::cout << fmt.str() << '\n';
    }

    void checkForSharpTurnsOntoRamps(const NodeID from_nid,
                                     const EdgeID via_eid,
                                     const Intersection &intersection) const
    {
        // Index 0 is UTurn road
        for (std::size_t i = 1; i < intersection.size(); ++i)
        {
            const auto &road = intersection[i];

            if (road.instruction.type != TurnType::OnRamp || !road.entry_allowed)
            {
                continue;
            }

            if (osrm::util::angularDeviation(0, road.angle) <= 2 * NARROW_TURN_ANGLE)
            {
                const NodeID via_nid = node_based_graph.GetTarget(via_eid);
                const NodeID to_nid = node_based_graph.GetTarget(road.eid);

                static const auto handler = "SharpTurnsOntoRamps";

                std::lock_guard<std::mutex> defer{turn_diagnostics_lock};

                TurnDiagnostic diag{from_nid, via_nid, to_nid, handler, std::to_string(road.angle)};
                turn_diagnostics.push_back(std::move(diag));
            }
        }
    }

    void checkForSharpTurnsBetweenRamps(const NodeID from_nid,
                                        const EdgeID via_eid,
                                        const Intersection &intersection) const
    {
        bool edge_is_link = node_based_graph.GetEdgeData(via_eid).road_classification.IsLinkClass();

        // Index 0 is UTurn road
        for (std::size_t i = 1; i < intersection.size(); ++i)
        {
            const auto &road = intersection[i];

            bool road_is_link =
                node_based_graph.GetEdgeData(road.eid).road_classification.IsLinkClass();

            if (!road.entry_allowed)
            {
                continue;
            }

            if (osrm::util::angularDeviation(0, road.angle) <= 2 * NARROW_TURN_ANGLE &&
                edge_is_link && road_is_link)
            {
                const NodeID via_nid = node_based_graph.GetTarget(via_eid);
                const NodeID to_nid = node_based_graph.GetTarget(road.eid);

                static const auto handler = "SharpTurnsBetweenRamps";

                std::lock_guard<std::mutex> defer{turn_diagnostics_lock};

                TurnDiagnostic diag{from_nid, via_nid, to_nid, handler, std::to_string(road.angle)};
                turn_diagnostics.push_back(std::move(diag));
            }
        }
    }

    void checkForSharpTurnsOnFastRoads(const NodeID from_nid,
                                       const EdgeID via_eid,
                                       const Intersection &intersection) const
    {
        // Turning from a slow road onto a slow or fast road might require a sharp turn
        if (!isFastRoad(via_eid))
        {
            return;
        }

        // Index 0 is UTurn road
        for (std::size_t i = 1; i < intersection.size(); ++i)
        {
            const auto &road = intersection[i];

            // We care for possible turns from fast roads on fast roads
            if (!road.entry_allowed)
            {
                continue;
            }

            if (osrm::util::angularDeviation(0, road.angle) <= 2 * NARROW_TURN_ANGLE)
            {
                const NodeID via_nid = node_based_graph.GetTarget(via_eid);
                const NodeID to_nid = node_based_graph.GetTarget(road.eid);

                static const auto handler = "SharpTurnsOnFastRoads";

                std::lock_guard<std::mutex> defer{turn_diagnostics_lock};

                TurnDiagnostic diag{from_nid, via_nid, to_nid, handler, std::to_string(road.angle)};
                turn_diagnostics.push_back(std::move(diag));
            }
        }
    }

    void checkForSharpDualCarriagewayUturns(const NodeID from_nid,
                                            const EdgeID via_eid,
                                            const Intersection &intersection) const
    {
        // Detect situations where dual-carriageway fans-in from two ways into one and
        // the Uturn from a dual-carriageway onto the dual-carriageway should be possible.
        //
        // . .
        //     > .
        // . .
        //

        if (intersection.size() < 3)
        {
            return;
        }

        const auto &leftmost_road = intersection.getLeftmostRoad();
        const auto &rightmost_road = intersection.getRightmostRoad();

        if (!leftmost_road.entry_allowed || !rightmost_road.entry_allowed)
        {
            return;
        }

        const auto &via_road_data = node_based_graph.GetEdgeData(via_eid);
        const auto &leftmost_road_data = node_based_graph.GetEdgeData(leftmost_road.eid);
        const auto &rightmost_road_data = node_based_graph.GetEdgeData(rightmost_road.eid);

        if (via_road_data.name_id == EMPTY_NAMEID || leftmost_road_data.name_id == EMPTY_NAMEID ||
            rightmost_road_data.name_id == EMPTY_NAMEID)
        {
            return;
        }

        if (via_road_data.roundabout || leftmost_road_data.roundabout ||
            rightmost_road_data.roundabout)
        {
            return;
        }

        const auto via_road_low_prio_class =
            via_road_data.road_classification.IsLowPriorityRoadClass();
        const auto leftmost_road_low_prio_class =
            leftmost_road_data.road_classification.IsLowPriorityRoadClass();
        const auto rightmost_road_low_prio_class =
            rightmost_road_data.road_classification.IsLowPriorityRoadClass();

        if (via_road_low_prio_class || leftmost_road_low_prio_class ||
            rightmost_road_low_prio_class)
        {
            return;
        }

        auto intersection_node_id = node_based_graph.GetTarget(via_eid);

        const NodeID leftmost_to_nid = node_based_graph.GetTarget(leftmost_road.eid);
        const NodeID rightmost_to_nid = node_based_graph.GetTarget(rightmost_road.eid);

        const EdgeID leftmost_rev_eid =
            node_based_graph.FindEdge(leftmost_to_nid, intersection_node_id);
        const EdgeID rightmost_rev_eid =
            node_based_graph.FindEdge(rightmost_to_nid, intersection_node_id);

        if (leftmost_rev_eid == SPECIAL_EDGEID || rightmost_rev_eid == SPECIAL_EDGEID)
        {
            return;
        }

        const auto &leftmost_rev_data = node_based_graph.GetEdgeData(leftmost_rev_eid);
        const auto &rightmost_rev_data = node_based_graph.GetEdgeData(rightmost_rev_eid);

        if (!leftmost_rev_data.reversed || !rightmost_rev_data.reversed)
        {
            return;
        }

        const auto same_name_left =
            !util::guidance::requiresNameAnnounced(via_road_data.name_id,
                                                   leftmost_road_data.name_id,
                                                   name_table,
                                                   street_name_suffix_table);
        const auto same_name_right =
            !util::guidance::requiresNameAnnounced(via_road_data.name_id,
                                                   rightmost_road_data.name_id,
                                                   name_table,
                                                   street_name_suffix_table);

        if (!same_name_left || !same_name_right)
        {
            return;
        }

        const auto left_turn_angle = osrm::util::angularDeviation(0, leftmost_road.angle);
        const auto right_turn_angle = osrm::util::angularDeviation(0, rightmost_road.angle);

        const auto sharp_left_turn = left_turn_angle <= 2 * NARROW_TURN_ANGLE;
        const auto sharp_right_turn = right_turn_angle <= 2 * NARROW_TURN_ANGLE;

        static const auto handler = "SharpTurnsOnDualCarriageways";

        if (sharp_left_turn)
        {
            std::lock_guard<std::mutex> defer{turn_diagnostics_lock};

            TurnDiagnostic diag{from_nid,
                                intersection_node_id,
                                leftmost_to_nid,
                                handler,
                                std::to_string(leftmost_road.angle)};
            turn_diagnostics.push_back(std::move(diag));
        }

        if (sharp_right_turn)
        {
            std::lock_guard<std::mutex> defer{turn_diagnostics_lock};

            TurnDiagnostic diag{from_nid,
                                intersection_node_id,
                                rightmost_to_nid,
                                handler,
                                std::to_string(rightmost_road.angle)};
            turn_diagnostics.push_back(std::move(diag));
        }
    }

    void checkForSharpTurnsInRoundabouts(const NodeID from_nid,
                                         const EdgeID via_eid,
                                         const Intersection &intersection) const
    {
        const bool via_is_roundabout = node_based_graph.GetEdgeData(via_eid).roundabout;

        // Index 0 is UTurn road
        for (std::size_t i = 1; i < intersection.size(); ++i)
        {
            const auto &road = intersection[i];

            if (!road.entry_allowed)
            {
                continue;
            }

            const bool is_roundabout = node_based_graph.GetEdgeData(road.eid).roundabout;

            if (osrm::util::angularDeviation(0, road.angle) <= 2 * NARROW_TURN_ANGLE &&
                (is_roundabout || via_is_roundabout))
            {
                const NodeID via_nid = node_based_graph.GetTarget(via_eid);
                const NodeID to_nid = node_based_graph.GetTarget(road.eid);

                static const auto handler = "SharpTurnsInRoundabouts";

                std::lock_guard<std::mutex> defer{turn_diagnostics_lock};

                TurnDiagnostic diag{from_nid, via_nid, to_nid, handler, std::to_string(road.angle)};
                turn_diagnostics.push_back(std::move(diag));
            }
        }
    }
};

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_GUIDANCE_VALIDATION_HANDLER_HPP_

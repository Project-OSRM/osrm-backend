#include "engine/plugins/tree.hpp"
#include "engine/api/tree_parameters.hpp"

#include "engine/datafacade/algorithm_datafacade.hpp"
#include "engine/polyline_compressor.hpp"

#include "guidance/turn_instruction.hpp"

#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>

namespace osrm::engine::plugins
{

namespace
{
using MLDFacade = datafacade::AlgorithmDataFacade<datafacade::MLD>;
namespace TT = osrm::guidance::TurnType;

// Safety bounds. hard_cap_m bounds each root-to-leaf path length, but a dense region (the
// Randstad) can still fan out combinatorially, so cap total segments and recursion depth. The JS
// quota layer (plan Sec 4.3) is the real horizon control; these just stop a pathological tree.
constexpr int MAX_SEGMENTS = 400;
constexpr int MAX_DEPTH = 10;
constexpr int MAX_RAMP_HOPS = 15;

// Turn types that mean "stay on the current road". Stage 1 report finding 7.3: the mainline
// continuation at a diverge is Suppressed/NoTurn, never classified Continue.
bool isContinuationTurn(const guidance::TurnType::Enum type)
{
    return type == TT::NoTurn || type == TT::Suppressed || type == TT::NewName ||
           type == TT::Continue;
}

double bearingDelta(const double a, const double b)
{
    auto diff = std::fmod(std::abs(a - b), 360.0);
    return diff > 180.0 ? 360.0 - diff : diff;
}

util::json::Object makeCoordinate(const util::Coordinate coordinate)
{
    util::json::Object out;
    out.values["lng"] = static_cast<double>(util::toFloating(coordinate.lon));
    out.values["lat"] = static_cast<double>(util::toFloating(coordinate.lat));
    return out;
}

std::vector<util::Coordinate> nodeForwardCoordinates(const datafacade::BaseDataFacade &facade,
                                                     const NodeID node)
{
    std::vector<util::Coordinate> coords;
    const auto geometry_id = facade.GetGeometryIndex(node).id;
    for (const auto node_based_node : facade.GetUncompressedForwardGeometry(geometry_id))
    {
        coords.push_back(facade.GetCoordinateOfNode(node_based_node));
    }
    return coords;
}

double polylineLength(const std::vector<util::Coordinate> &coords)
{
    double length = 0.0;
    for (std::size_t i = 1; i < coords.size(); ++i)
    {
        length += util::coordinate_calculation::greatCircleDistance(coords[i - 1], coords[i]);
    }
    return length;
}

bool isMotorwayNode(const datafacade::BaseDataFacade &facade, const NodeID node)
{
    const auto classes = facade.GetClasses(facade.GetClassData(node));
    return std::find(classes.begin(), classes.end(), "motorway") != classes.end();
}

// Split a road ref on ';' into its concurrency components (OSRM stores concurrent refs as
// "A2;A67"). Trimmed + uppercased. Stage 2 finding: matching must be component-wise, not string
// equality, or a concurrency-unbundling fork looks like a branch toward a different road.
std::vector<std::string> refComponents(const std::string &ref)
{
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= ref.size())
    {
        const auto sep = ref.find(';', start);
        const auto end = sep == std::string::npos ? ref.size() : sep;
        auto piece = ref.substr(start, end - start);
        const auto first = piece.find_first_not_of(' ');
        const auto last = piece.find_last_not_of(' ');
        if (first != std::string::npos)
        {
            auto token = piece.substr(first, last - first + 1);
            for (auto &c : token)
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            parts.push_back(std::move(token));
        }
        if (sep == std::string::npos)
            break;
        start = sep + 1;
    }
    return parts;
}

bool shareComponent(const std::vector<std::string> &a, const std::vector<std::string> &b)
{
    for (const auto &x : a)
        for (const auto &y : b)
            if (x == y)
                return true;
    return false;
}

// The road-ref tokens of a destination signage string. GetDestinationsForID returns one merged
// string (Stage 1 finding 7.2): "REF: places" / "REF" / "RING A16" / "A2;A67: places" / place.
// Take the part before the first ':' and pull every road-ref token out of it (so an "N44" in a
// parenthesised place on the far side of the colon can't be mistaken for the ref).
std::vector<std::string> parseDestinationRefs(const std::string_view destinations)
{
    std::string head(destinations);
    const auto colon = head.find(':');
    if (colon != std::string::npos)
    {
        head = head.substr(0, colon);
    }

    static const std::regex ref_pattern(R"([A-Za-z]{1,3}\s?\d+)");
    std::vector<std::string> refs;
    for (auto it = std::sregex_iterator(head.begin(), head.end(), ref_pattern);
         it != std::sregex_iterator();
         ++it)
    {
        std::string ref = it->str();
        ref.erase(std::remove_if(
                      ref.begin(), ref.end(), [](unsigned char c) { return std::isspace(c); }),
                  ref.end());
        for (auto &c : ref)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        refs.push_back(std::move(ref));
    }
    return refs;
}

// Motorway pattern from the spec (^[AE]\d+): A-roads and E-roads qualify, N-roads do not.
bool isMotorwayRef(const std::string &ref)
{
    static const std::regex motorway_pattern(R"(^[AE]\d+$)");
    return std::regex_match(ref, motorway_pattern);
}

bool anyMotorwayRef(const std::vector<std::string> &refs)
{
    return std::any_of(refs.begin(), refs.end(), isMotorwayRef);
}

// The branch's road identity is its first motorway ref, not necessarily the first token: a ramp
// signed "N201; A9" qualifies via A9 and the child is the A9, not the N-road.
std::string firstMotorwayRef(const std::vector<std::string> &refs)
{
    const auto it = std::find_if(refs.begin(), refs.end(), isMotorwayRef);
    return it == refs.end() ? std::string() : *it;
}

// Split a destinations string into ref + place names for the "toward" array.
util::json::Array towardArray(const std::string_view destinations)
{
    util::json::Array toward;
    for (const auto &ref : parseDestinationRefs(destinations))
    {
        toward.values.emplace_back(ref);
    }

    const auto colon = destinations.find(':');
    if (colon != std::string_view::npos)
    {
        std::string places(destinations.substr(colon + 1));
        std::size_t start = 0;
        while (start <= places.size())
        {
            const auto comma = places.find(',', start);
            const auto end = comma == std::string::npos ? places.size() : comma;
            auto piece = places.substr(start, end - start);
            const auto first = piece.find_first_not_of(' ');
            const auto last = piece.find_last_not_of(' ');
            if (first != std::string::npos)
            {
                toward.values.emplace_back(piece.substr(first, last - first + 1));
            }
            if (comma == std::string::npos)
                break;
            start = comma + 1;
        }
    }
    return toward;
}

// One outgoing turn of the edge-based graph, with the target segment's attributes pre-read.
struct Outgoing
{
    NodeID target;
    guidance::TurnInstruction turn;
    std::string ref;          // target's raw ref (may be concurrency "A2;A67", or gore-mainline)
    std::string destinations; // raw GetDestinationsForID
    std::vector<std::string> branch_refs; // parsed from destinations (the branch identity)
};

std::vector<Outgoing> gatherForwardOutgoing(const datafacade::BaseDataFacade &facade,
                                            const MLDFacade &mld,
                                            const NodeID node)
{
    std::vector<Outgoing> out;
    for (const auto edge : mld.GetAdjacentEdgeRange(node))
    {
        if (!mld.IsForwardEdge(edge))
            continue;

        const auto target = mld.GetTarget(edge);
        const auto turn_id = mld.GetEdgeData(edge).turn_id;
        const auto name_id = facade.GetNameIndex(target);
        std::string destinations(facade.GetDestinationsForID(name_id));
        out.push_back({target,
                       facade.GetTurnInstructionForEdgeID(turn_id),
                       std::string(facade.GetRefForID(name_id)),
                       destinations,
                       parseDestinationRefs(destinations)});
    }
    return out;
}

// Pick the edge that stays on the mainline. Preference: a Suppressed/NoTurn/NewName/Continue turn,
// then a fork/other arm sharing a ref component with the road we are already on (Stage 2 finding:
// pure forks - Ridderkerk A15/A16, the A4/N-ring split, the A2/A67 concurrency unbundling - have
// no continuation turn; the mainline leaves via a Fork arm sharing the current ref). Returns
// out.end() when the road genuinely ends.
std::vector<Outgoing>::const_iterator
selectContinuation(const std::vector<Outgoing> &out, const std::vector<std::string> &current_refs)
{
    auto best = out.end();
    int best_rank = 0;
    for (auto it = out.begin(); it != out.end(); ++it)
    {
        int rank = 0;
        if (isContinuationTurn(it->turn.type))
            rank = 3;
        else if (shareComponent(current_refs, it->branch_refs))
            rank = 2;
        else if (it->branch_refs.empty())
            rank = 1;

        if (rank > best_rank)
        {
            best_rank = rank;
            best = it;
        }
    }
    return best;
}

// Follow a ramp connector from the branch's first node until motorway class resumes, accumulating
// connector length. exit_node is the first motorway-class node on the branch.
struct RampResult
{
    double connector_length = 0.0;
    NodeID exit_node = SPECIAL_NODEID;
    bool ok = false;
};

RampResult traverseRamp(const datafacade::BaseDataFacade &facade,
                        const MLDFacade &mld,
                        const NodeID first,
                        const double budget)
{
    RampResult result;
    std::unordered_set<NodeID> ramp_visited;
    NodeID node = first;
    for (int hops = 0; hops <= MAX_RAMP_HOPS; ++hops)
    {
        if (isMotorwayNode(facade, node))
        {
            result.exit_node = node;
            result.ok = true;
            return result;
        }
        result.connector_length += polylineLength(nodeForwardCoordinates(facade, node));
        if (result.connector_length > budget)
            return result;

        const auto outgoing = gatherForwardOutgoing(facade, mld, node);
        auto next = outgoing.end();
        for (auto it = outgoing.begin(); it != outgoing.end(); ++it)
        {
            if (isContinuationTurn(it->turn.type))
            {
                next = it;
                break;
            }
        }
        if (next == outgoing.end() && !outgoing.empty())
            next = outgoing.begin();
        if (next == outgoing.end())
            return result;

        if (!ramp_visited.insert(next->target).second)
            return result;
        node = next->target;
    }
    return result;
}

// Recursively walk one mainline segment: follow the continuation, accumulate geometry, and recurse
// into each qualifying motorway branch. Returns the Contract 1 route object.
util::json::Object walkSegment(const datafacade::BaseDataFacade &facade,
                               const MLDFacade &mld,
                               const NodeID start_node,
                               const std::string &reported_ref,
                               std::vector<util::Coordinate> current_coords,
                               const double start_offset,
                               const double hard_cap,
                               std::unordered_set<NodeID> visited,
                               const bool debug,
                               const int depth,
                               int &segment_count)
{
    std::vector<util::Coordinate> polyline;
    util::json::Array branches;
    util::json::Array junctions_debug;

    NodeID current = start_node;
    // Initialise from the ref we branched onto, not the start node's own ref: at a gore the first
    // segment still carries the mainline ref (Stage 1 finding 7.4), which would defeat the
    // same-road suppression on the child's very first node.
    std::string current_ref = reported_ref;
    // The committed road identity for this whole segment; unlike current_ref it never drifts, so
    // it is the stable backstop for same-road suppression at depth.
    const auto reported_refs = refComponents(reported_ref);
    double offset = start_offset;

    while (true)
    {
        const double node_end_offset = offset + polylineLength(current_coords);
        const auto current_refs = refComponents(current_ref);

        const auto outgoing = gatherForwardOutgoing(facade, mld, current);
        const auto continuation = selectContinuation(outgoing, current_refs);

        // Ref components of the arm we are following, for the same-road split test below.
        std::vector<std::string> continuation_refs;
        if (continuation != outgoing.end())
        {
            continuation_refs = refComponents(continuation->ref);
            continuation_refs.insert(continuation_refs.end(),
                                     continuation->branch_refs.begin(),
                                     continuation->branch_refs.end());
        }

        for (auto it = outgoing.begin(); it != outgoing.end(); ++it)
        {
            if (it == continuation)
                continue;
            if (it->turn.type != TT::OffRamp && it->turn.type != TT::Fork)
                continue;
            // A fork arm sharing a ref component with the road we are on - either the tracked road
            // identity or the arm we are actually following - is a parallel-carriageway split or a
            // concurrency unbundling (e.g. A2;A67 -> A2 + A67), not a branch to a different
            // motorway. Skip it. The continuation check also catches cases where current_ref has
            // drifted (it only updates on NewName).
            if (shareComponent(current_refs, it->branch_refs) ||
                shareComponent(continuation_refs, it->branch_refs) ||
                shareComponent(reported_refs, it->branch_refs))
                continue;

            const bool qualifies = anyMotorwayRef(it->branch_refs);
            // Report the qualifying motorway ref when it qualifies (a "N201; A9" ramp is the A9),
            // otherwise the leading token (an N-road local exit).
            const std::string primary_ref =
                qualifies ? firstMotorwayRef(it->branch_refs)
                          : (it->branch_refs.empty() ? std::string() : it->branch_refs.front());

            if (debug)
            {
                util::json::Object dbg;
                dbg.values["name"] = "";
                dbg.values["at_offset_m"] = node_end_offset;
                dbg.values["turn_type"] = osrm::guidance::instructionTypeToString(it->turn.type);
                dbg.values["toward"] = towardArray(it->destinations);
                dbg.values["toward_ref"] = primary_ref.empty()
                                               ? util::json::Value(util::json::Null())
                                               : util::json::Value(primary_ref);
                dbg.values["qualifies"] = qualifies ? util::json::Value(util::json::True())
                                                    : util::json::Value(util::json::False());
                junctions_debug.values.emplace_back(std::move(dbg));
            }

            if (!qualifies || node_end_offset >= hard_cap || segment_count >= MAX_SEGMENTS ||
                depth >= MAX_DEPTH)
                continue;

            const auto ramp = traverseRamp(facade, mld, it->target, hard_cap - node_end_offset);
            if (!ramp.ok)
                continue;

            const double child_start_offset = node_end_offset + ramp.connector_length;
            if (child_start_offset >= hard_cap)
                continue;
            if (visited.count(ramp.exit_node))
                continue; // branch loops back onto our own path

            auto child_visited = visited;
            child_visited.insert(ramp.exit_node);
            ++segment_count;

            auto child_route = walkSegment(facade,
                                           mld,
                                           ramp.exit_node,
                                           firstMotorwayRef(it->branch_refs),
                                           nodeForwardCoordinates(facade, ramp.exit_node),
                                           child_start_offset,
                                           hard_cap,
                                           std::move(child_visited),
                                           debug,
                                           depth + 1,
                                           segment_count);

            util::json::Object junction;
            junction.values["name"] = ""; // never derivable in-plugin (Stage 2 finding); app models
                                          // require a non-null string
            const auto exits = facade.GetExitsForID(facade.GetNameIndex(it->target));
            junction.values["exit_ref"] =
                exits.empty() ? util::json::Value(util::json::Null())
                              : util::json::Value(std::string(exits));
            junction.values["at_offset_m"] = node_end_offset;
            junction.values["connector_length_m"] = ramp.connector_length;
            junction.values["toward"] = towardArray(it->destinations);

            util::json::Object branch;
            branch.values["junction"] = std::move(junction);
            branch.values["route"] = std::move(child_route);
            branches.values.emplace_back(std::move(branch));
        }

        polyline.insert(polyline.end(), current_coords.begin(), current_coords.end());

        if (continuation == outgoing.end())
            break;
        if (node_end_offset >= hard_cap)
            break;

        const NodeID next = continuation->target;
        if (!visited.insert(next).second)
            break; // cycle (e.g. the A10 ring): the distance budget also guards this

        // Update the road identity only when OSRM signals a genuine renumbering (NewName). Plain
        // continuations (NoTurn/Suppressed) through a knooppunt gore carry the *crossing* motorway's
        // stale ref (Stage 1 finding 7.4); overwriting on those loses the road we branched onto and
        // makes the same-road fork suppression fail.
        if (continuation->turn.type == TT::NewName && !continuation->ref.empty())
            current_ref = continuation->ref;

        auto next_coords = nodeForwardCoordinates(facade, next);
        // The shared junction vertex is the last point of `current` and the first of `next`.
        current_coords.assign(next_coords.begin() + (next_coords.empty() ? 0 : 1),
                              next_coords.end());
        offset = node_end_offset;
        current = next;
    }

    util::json::Object route;
    route.values["ref"] = reported_ref;
    route.values["start_offset_m"] = start_offset;
    route.values["length_m"] = polylineLength(polyline);
    route.values["polyline"] = encodePolyline<100000>(polyline.cbegin(), polyline.cend());
    route.values["branches"] = std::move(branches);
    if (debug)
        route.values["junctions"] = std::move(junctions_debug);
    return route;
}
} // namespace

TreePlugin::TreePlugin(const std::optional<double> default_radius) : BasePlugin(default_radius) {}

Status TreePlugin::HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                                 const api::TreeParameters &params,
                                 osrm::engine::api::ResultT &result) const
{
    BOOST_ASSERT(params.IsValid());

    if (!CheckAlgorithms(params, algorithms, result))
        return Status::Error;

    const auto &facade = algorithms.GetFacade();

    const auto *mld = dynamic_cast<const MLDFacade *>(&facade);
    if (mld == nullptr)
    {
        return Error("NotImplemented", "The tree service requires the MLD algorithm", result);
    }

    if (!CheckAllCoordinates(params.coordinates))
        return Error("InvalidOptions", "Coordinates are invalid", result);

    if (params.coordinates.size() != 1)
    {
        return Error("InvalidOptions", "Only one input coordinate is supported", result);
    }

    if (params.bearings.empty() || !params.bearings.front())
    {
        return Error("InvalidOptions", "A bearing is required to disambiguate direction", result);
    }
    const double heading = params.bearings.front()->bearing;

    auto phantom_nodes = GetPhantomNodes(facade, params, 1);
    if (phantom_nodes.front().empty())
    {
        return Error("NoSegment", "Could not find a matching segment for coordinate", result);
    }
    const auto &phantom = phantom_nodes.front().front().phantom_node;

    // Pick the carriageway whose onward heading matches the request.
    const bool forward_ok = phantom.forward_segment_id.enabled;
    const bool reverse_ok = phantom.reverse_segment_id.enabled;
    const auto forward_delta =
        forward_ok ? bearingDelta(heading, phantom.GetBearing(false)) : 360.0;
    const auto reverse_delta =
        reverse_ok ? bearingDelta(heading, phantom.GetBearing(true)) : 360.0;
    const NodeID start_node = (forward_delta <= reverse_delta) ? phantom.forward_segment_id.id
                                                               : phantom.reverse_segment_id.id;

    // Motorway class post-filter (Stage 1 finding 7.7: the snap API has no class filter, and an
    // off-network point snaps to the nearest road rather than failing). Off-motorway snap becomes
    // NoSegment so the app maps it to the off-motorway state.
    if (!isMotorwayNode(facade, start_node))
    {
        return Error("NoSegment", "Snapped coordinate is not on a motorway", result);
    }

    // Begin the root polyline at the snap: start from the vertex of the start node nearest the snap.
    auto node_coords = nodeForwardCoordinates(facade, start_node);
    std::size_t snap_index = 0;
    double closest = std::numeric_limits<double>::max();
    for (std::size_t i = 0; i < node_coords.size(); ++i)
    {
        const auto d =
            util::coordinate_calculation::greatCircleDistance(node_coords[i], phantom.location);
        if (d < closest)
        {
            closest = d;
            snap_index = i;
        }
    }
    std::vector<util::Coordinate> root_coords(node_coords.begin() + snap_index, node_coords.end());

    std::unordered_set<NodeID> visited;
    visited.insert(start_node);
    int segment_count = 1;
    const std::string root_ref(facade.GetRefForID(facade.GetNameIndex(start_node)));

    auto root_route = walkSegment(facade,
                                  *mld,
                                  start_node,
                                  root_ref,
                                  std::move(root_coords),
                                  0.0,
                                  static_cast<double>(params.hard_cap_m),
                                  std::move(visited),
                                  params.debug,
                                  0,
                                  segment_count);

    result = util::json::Object();
    auto &response = std::get<util::json::Object>(result);
    response.values["code"] = "Ok";
    response.values["ref"] = std::move(root_route.values["ref"]);
    response.values["start_offset_m"] = std::move(root_route.values["start_offset_m"]);
    response.values["length_m"] = std::move(root_route.values["length_m"]);
    response.values["polyline"] = std::move(root_route.values["polyline"]);
    response.values["branches"] = std::move(root_route.values["branches"]);
    if (params.debug)
        response.values["junctions"] = std::move(root_route.values["junctions"]);
    response.values["segment_count"] = static_cast<double>(segment_count);

    util::json::Object snapped;
    snapped.values["location"] = makeCoordinate(phantom.location);
    snapped.values["input"] = makeCoordinate(params.coordinates.front());
    snapped.values["requested_bearing"] = heading;
    response.values["snapped"] = std::move(snapped);

    return Status::Ok;
}
} // namespace osrm::engine::plugins

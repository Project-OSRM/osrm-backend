#include "engine/plugins/tree.hpp"
#include "engine/api/tree_parameters.hpp"

#include "engine/datafacade/algorithm_datafacade.hpp"
#include "engine/polyline_compressor.hpp"

#include "guidance/turn_instruction.hpp"

#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <regex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
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
// Landing retry: a branch whose walk ends in a cycle/dead-end (not the distance cap) after less
// than this, having spawned no children, is treated as a bad ramp landing at a complex interchange
// and retried from an alternative motorway alignment at the landing node (bounded attempts).
constexpr double RETRY_MIN_M = 5000.0;
constexpr std::size_t MAX_LANDING_RETRIES = 6;
// Root snap retry: inside a stack the nearest bearing-valid motorway alignment can be a service
// loop that truncates the whole corridor; if the root walk lands badly, fall back to the next
// snap candidate. Bounded number of motorway candidates tried.
constexpr std::size_t MAX_ROOT_CANDIDATES = 3;
// Crossing-road directions: a single ramp onto a crossing road reaches both its travel directions
// via a fork a short way in; the walk follows one and (via the landing retry) keeps the longest.
// These bound how many *other* directions - fork alternatives that reach a real road end rather
// than a cycle - are additionally spawned as sibling branches, and the minimum length one must
// reach to count (excludes tiny stubs; the Utrechtsebaan-style dead-end is a few km).
constexpr std::size_t MAX_CROSSING_DIRECTIONS = 3;
constexpr double MIN_DIRECTION_M = 1500.0;
// A same-ref arm suppressed this close to a branch's start is the crossing road's other direction
// (not a mainline parallelbaan); it is spawned as a sibling if it reaches a real road.
constexpr double CONNECTOR_M = 3000.0;
// Same-road parallel carriageway (hoofdbaan/parallelbaan split, e.g. the A2 through
// 's-Hertogenbosch): the through carriageway the walk follows can skip motorway junctions that only
// the parallel carriageway serves (KP Empel/Hintham, where the A59 leaves). The parallelbaan is
// never emitted (it is the same road), but each such arm is probed this far - bounded; it stops
// earlier where the parallelbaan rejoins the through path - to lift those junctions onto the
// mainline. See §S3-fix9.
constexpr std::size_t MAX_PARALLEL_FORKS = 12;
constexpr double PARALLELBAAN_PROBE_M = 12000.0;
// Two direction candidates ending within this of each other are the same alignment reached via
// duplicate forks - keep one.
constexpr double SAME_DIRECTION_M = 500.0;
// Same-direction overlap de-dup: two same-ref siblings whose arc-distance samples stay within
// DIR_MATCH_M of each other for at least DIR_OVERLAP_FRAC of the shared samples are the same
// physical corridor reached twice (twin entry ramps / parallel carriageways onto one crossing
// direction) even when they end far enough apart to slip past the end-coordinate check.
constexpr double DIR_MATCH_M = 300.0;
constexpr double DIR_OVERLAP_FRAC = 0.75;

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

// Arc-distance sample distances (metres) used to fingerprint a branch's alignment for
// same-direction de-duplication. Two branches from a coincident start that run together (parallel
// carriageways / twin entry ramps onto the same crossing direction) coincide at every distance;
// opposite directions diverge within the first km. Geometrically-spaced so a few points span both
// short and 150 km branches.
constexpr std::array<double, 7> DIR_SAMPLE_M = {1000, 2000, 4000, 8000, 16000, 32000, 64000};

// The branch's coordinate at each DIR_SAMPLE_M distance that is within its length (a prefix of the
// list), for the same-direction overlap test in the per-parent de-dup.
std::vector<util::Coordinate> sampleAlong(const std::vector<util::Coordinate> &coords)
{
    std::vector<util::Coordinate> samples;
    if (coords.empty())
        return samples;
    double accumulated = 0.0;
    std::size_t next = 0;
    for (std::size_t i = 1; i < coords.size() && next < DIR_SAMPLE_M.size(); ++i)
    {
        accumulated += util::coordinate_calculation::greatCircleDistance(coords[i - 1], coords[i]);
        while (next < DIR_SAMPLE_M.size() && accumulated >= DIR_SAMPLE_M[next])
        {
            samples.push_back(coords[i]);
            ++next;
        }
    }
    return samples;
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

constexpr std::size_t NO_PARENT = std::numeric_limits<std::size_t>::max();

// One pending segment to expand: a walk-start plus everything needed to place it in the tree.
struct WorkItem
{
    std::size_t id;
    std::size_t parent_id;
    NodeID start_node;
    std::string reported_ref;
    std::vector<util::Coordinate> start_coords;
    double start_offset;
    std::unordered_set<NodeID> visited; // per-path visited set (copied from the parent)
    int depth;
    // A sibling direction branch is walked from the same start but forced down a fork alternative;
    // these pin that fork (SPECIAL_NODEID = the primary branch, which also spawns the siblings).
    NodeID override_node = SPECIAL_NODEID;
    NodeID override_target = SPECIAL_NODEID;
    // The start_offset of this item's FIRST-LEVEL ancestor (the spur head it hangs off), for the
    // class-weighted budget ordering (§S3-fix9). -1 on the root; == start_offset on a first-level
    // branch (it is its own spur head); inherited from the parent deeper down.
    double spur_head_offset = -1.0;
    util::json::Object junction; // how this segment attaches to its parent (empty for the root)
};

// One walked segment: its route object plus the wiring to reattach it to its parent afterwards.
struct SegmentResult
{
    std::size_t id;
    std::size_t parent_id;
    double order_key; // start_offset, for stable nearest-first sibling ordering
    std::string ref;
    util::Coordinate end_coord; // last geometry point, for de-duplicating same-direction branches
    std::vector<util::Coordinate> dir_samples; // arc-distance fingerprint for the overlap de-dup
    util::json::Object junction;
    util::json::Object route; // {ref, start_offset_m, length_m, polyline, [junctions]}
};

// Min-heap key for the class-weighted best-first budget (§S3-fix9): (priority_class, start_offset,
// work id). Lower class pops first; within a class, nearest-first. Class 0 = the root and every
// first-level spur head (guaranteed a slot at any distance); class 1 = a spur's subtree within
// SPUR_RESERVED_DEPTH_M of its head; class 2 = deep fill (the old pure nearest-first behaviour).
using Pending = std::tuple<int, double, std::size_t>;

// A first-level spur is guaranteed its head plus this much of its own subtree before deep fill
// starts, so every spur can report an honest first-charger distance / desert bound rather than a
// stub. Mirrors the BE stage-A SPUR_RESERVED_DEPTH_M.
constexpr double SPUR_RESERVED_DEPTH_M = 25000.0;

// The budget priority class of a pending work item (see Pending). parent_id 0 is the root's id.
int priorityClass(const WorkItem &w)
{
    if (w.parent_id == NO_PARENT || w.parent_id == 0)
        return 0; // the root chain and every first-level spur head
    return (w.start_offset - w.spur_head_offset) <= SPUR_RESERVED_DEPTH_M ? 1 : 2;
}

// Output of walking one segment: its route object, the children it would spawn (id assigned by the
// driver), the walked length, and whether it ended badly - a cycle / dead-end / class downgrade
// rather than the distance cap - which is the trigger for a landing retry.
struct WalkOutput
{
    util::json::Object route;
    std::vector<WorkItem> children;
    std::vector<WorkItem> siblings; // extra direction branches (children of this segment's parent)
    util::Coordinate end_coord;     // last geometry point (for de-duplicating sibling directions)
    std::vector<util::Coordinate> dir_samples; // arc-distance fingerprint for the overlap de-dup
    double length = 0.0;
    bool bad_ending = false;
    bool rejoined_ancestor = false; // walk merged back onto an ancestor path (parallel carriageway)
    bool ended_by_cycle = false;    // stopped because a motorway continuation was already visited
    // Every edge-based node this walk occupied, so a descendant can inherit the ancestor's FULL
    // path and detect a rejoin onto it (the per-path visited set the child was spawned with only
    // covers the ancestor up to the spawn junction - see the driver's ancestor-path completion).
    std::vector<NodeID> path_nodes;
    // The post-split signage of the direction this walk actually took, when the segment forks into
    // multiple same-ref directions near its start (a shared slip-road gantry carries the union of
    // both directions' destinations; each direction's refined signage lives on its own arm).
    std::string refined_toward;
    // Early forks (node, an untaken usable motorway arm) - retry candidates for a bad landing.
    std::vector<std::pair<NodeID, NodeID>> forks;
    // Same-ref arms suppressed near the start - the crossing road's other travel direction(s).
    std::vector<std::pair<NodeID, NodeID>> direction_forks;
    // Same-road parallel carriageways (hoofdbaan/parallelbaan split): (offset, first node on the
    // arm). expandSegment probes each to lift the motorway junctions the through carriageway skipped
    // (the A59 interchanges on the A2 parallelbaan at 's-Hertogenbosch). See §S3-fix9.
    std::vector<std::pair<double, NodeID>> parallel_forks;
};

// Walk one mainline segment from item.start_node (no recursion, no side effects): follow the
// continuation, accumulate geometry, record junctions, and collect a child WorkItem for each
// qualifying motorway branch. An optional fork override forces the continuation at one node onto a
// given target (used by the landing retry to explore an alternative early arm).
WalkOutput walkOne(const datafacade::BaseDataFacade &facade,
                   const MLDFacade &mld,
                   const WorkItem &item,
                   const double hard_cap,
                   const bool debug,
                   const NodeID override_node = SPECIAL_NODEID,
                   const NodeID override_target = SPECIAL_NODEID)
{
    std::vector<util::Coordinate> current_coords = item.start_coords;
    std::unordered_set<NodeID> visited = item.visited;
    std::vector<util::Coordinate> polyline;
    util::json::Array junctions_debug;
    std::vector<WorkItem> children;
    std::vector<NodeID> path_nodes;
    std::vector<std::pair<NodeID, NodeID>> forks;
    std::vector<std::pair<NodeID, NodeID>> direction_forks;
    std::vector<std::pair<double, NodeID>> parallel_forks;
    std::string refined_toward;
    bool stopped_by_cap = false;
    bool rejoined_ancestor = false;
    bool ended_by_cycle = false;

    NodeID current = item.start_node;
    // Initialise from the ref we branched onto, not the start node's own ref: at a gore the first
    // segment still carries the mainline ref (Stage 1 finding 7.4), which would defeat the
    // same-road suppression on the child's very first node.
    std::string current_ref = item.reported_ref;
    // The committed road identity for this whole segment; unlike current_ref it never drifts, so
    // it is the stable backstop for same-road suppression at depth.
    const auto reported_refs = refComponents(item.reported_ref);
    double offset = item.start_offset;
    // Diagnostic: metres walked on non-motorway-class nodes. Should always be 0 - a non-zero value
    // means the walk escaped the motorway network (see the motorway-class continuation guard below).
    double non_motorway_len = 0.0;

    while (true)
    {
        path_nodes.push_back(current);
        const double node_len = polylineLength(current_coords);
        const double node_end_offset = offset + node_len;
        if (!isMotorwayNode(facade, current))
            non_motorway_len += node_len;
        const auto current_refs = refComponents(current_ref);
        // Set when this node forks off a same-ref direction (the crossing road's other way): the
        // signage captured at the parent's junction is then the pre-split slip-road union, so the
        // arm we take here carries this direction's refined destinations.
        bool direction_fork_here = false;

        const auto outgoing = gatherForwardOutgoing(facade, mld, current);
        const auto continuation = selectContinuation(outgoing, current_refs);

        // Ref components of the arm we are following, for the same-road split test below. Only the
        // continuation's RAW ref identifies the road we stay on; its DESTINATION signage must be
        // excluded, because at a knooppunt gore the through-lane's sign merges the mainline with the
        // crossing motorway (A50's through-sign reads "A50, A1: Zwolle, ..., Amsterdam"). Folding
        // those destination refs in makes the crossing motorway's own off-ramp look like a
        // same-road split, and one direction of every such crossing gets wrongly suppressed
        // (§S3-fix8: A1-east at KP Beekbergen, A28-north at KP Hattemerbroek).
        std::vector<std::string> continuation_refs;
        if (continuation != outgoing.end())
            continuation_refs = refComponents(continuation->ref);

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
            {
                // Near a branch's start, a suppressed arm carrying the branch's own crossing ref is
                // the crossing road's OTHER travel direction (the single A4->A12 ramp reaches both
                // A12 west and east), not a mainline parallelbaan. Record it for a sibling spawn.
                if (item.parent_id != NO_PARENT &&
                    (node_end_offset - item.start_offset) < CONNECTOR_M &&
                    shareComponent(reported_refs, it->branch_refs) &&
                    isMotorwayNode(facade, it->target) && !visited.count(it->target))
                {
                    direction_fork_here = true;
                    if (direction_forks.size() < MAX_CROSSING_DIRECTIONS)
                        direction_forks.push_back({current, it->target});
                }
                continue;
            }

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

            // Same-road parallel carriageway (hoofdbaan/parallelbaan split): a Fork/OffRamp arm whose
            // RAW ref is the road we are on but which advertises no crossing motorway of its own (the
            // parallelbaan entry has an empty destination). It is not a branch (same road) and not the
            // through carriageway (we took that arm), yet it can serve motorway junctions the through
            // lane skips - the A59 interchanges on the A2 parallelbaan through 's-Hertogenbosch.
            // Record it so expandSegment can probe it and lift those junctions onto this segment.
            // Keyed on the RAW ref (the destination is empty) but only when the arm does NOT qualify
            // as a different motorway - this excludes the documented "gore ref = mainline ref" stale
            // edge-ref case (the A27 off-ramp at Everdingen reads raw ref A2 but is really the A27,
            // which already qualifies as its own branch). Only on the current road and the first
            // spurs (depth <= 1): that is where the parallelstructuur matters for the driver, and it
            // bounds the probing cost (the ruling's "prioritise the current road and the first
            // spurs"). See §S3-fix9.
            if (!qualifies && item.depth <= 1 && continuation != outgoing.end() &&
                isMotorwayNode(facade, it->target) && !visited.count(it->target) &&
                shareComponent(current_refs, refComponents(it->ref)) &&
                parallel_forks.size() < MAX_PARALLEL_FORKS)
            {
                parallel_forks.push_back({node_end_offset, it->target});
            }

            if (!qualifies || node_end_offset >= hard_cap || item.depth >= MAX_DEPTH)
                continue;

            const auto ramp = traverseRamp(facade, mld, it->target, hard_cap - node_end_offset);
            if (!ramp.ok)
                continue;

            const double child_start_offset = node_end_offset + ramp.connector_length;
            if (child_start_offset >= hard_cap)
                continue;
            if (visited.count(ramp.exit_node))
                continue; // branch loops back onto our own path

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

            WorkItem child;
            child.parent_id = item.id;
            child.start_node = ramp.exit_node;
            child.reported_ref = primary_ref;
            child.start_coords = nodeForwardCoordinates(facade, ramp.exit_node);
            child.start_offset = child_start_offset;
            child.visited = visited;
            child.visited.insert(ramp.exit_node);
            child.depth = item.depth + 1;
            child.junction = std::move(junction);
            children.push_back(std::move(child));
        }

        polyline.insert(polyline.end(), current_coords.begin(), current_coords.end());

        if (node_end_offset >= hard_cap)
        {
            stopped_by_cap = true;
            break;
        }

        // Choose the next node. Normally the continuation, but never advance onto a non-motorway
        // node - the A12 Utrechtsebaan into Den Haag, the A13 ending at Ypenburg - nor onto a
        // visited node (a cycle). A usable arm stays on the motorway network and off our own path.
        const auto usable = [&](const Outgoing &o)
        { return isMotorwayNode(facade, o.target) && !visited.count(o.target); };

        auto chosen = (continuation != outgoing.end() && usable(*continuation)) ? continuation
                                                                                : outgoing.end();

        // Landing retry: at the designated node, force the alternative arm instead.
        if (current == override_node)
            for (auto it = outgoing.begin(); it != outgoing.end(); ++it)
                if (it->target == override_target && usable(*it))
                {
                    chosen = it;
                    break;
                }

        // At a same-ref direction split, the arm we take carries this direction's own refined
        // signage (the parent-junction signage was the shared pre-split union). Capture the first
        // one so the driver can replace this segment's merged junction "toward".
        if (direction_fork_here && refined_toward.empty() && chosen != outgoing.end() &&
            !chosen->destinations.empty())
            refined_toward = chosen->destinations;

        // Record early forks (usable arms we did not take) so a bad landing can be retried down an
        // alternative - a stack ramp can offer two motorway arms, one of them a service loop that
        // only reveals itself as a cycle several km later.
        if (chosen != outgoing.end() && (node_end_offset - item.start_offset) < RETRY_MIN_M)
            for (auto it = outgoing.begin(); it != outgoing.end() && forks.size() < 8; ++it)
                if (it != chosen && usable(*it))
                    forks.push_back({current, it->target});

        if (chosen == outgoing.end())
        {
            // Did we stop because a motorway continuation merges back onto an ancestor's path? That
            // is a parallel carriageway (parallelbaan) rejoining the main line - the caller drops
            // such a stub if it carried no exits of its own. item.visited is exactly the inherited
            // ancestor path (plus this segment's start); the local `visited` also holds our own
            // nodes, so test against the inherited set.
            for (const auto &o : outgoing)
                if (isMotorwayNode(facade, o.target) && visited.count(o.target))
                {
                    ended_by_cycle = true; // a motorway continuation exists but is already on our path
                    if (item.visited.count(o.target))
                        rejoined_ancestor = true; // ... specifically an ancestor's path (parallelbaan)
                }
            break;
        }

        const NodeID next = chosen->target;
        visited.insert(next);

        // Update the road identity only when OSRM signals a genuine renumbering (NewName). Plain
        // continuations (NoTurn/Suppressed) through a knooppunt gore carry the *crossing* motorway's
        // stale ref (Stage 1 finding 7.4); overwriting on those loses the road we branched onto and
        // makes the same-road fork suppression fail.
        if (chosen->turn.type == TT::NewName && !chosen->ref.empty())
            current_ref = chosen->ref;

        auto next_coords = nodeForwardCoordinates(facade, next);
        // The shared junction vertex is the last point of `current` and the first of `next`.
        current_coords.assign(next_coords.begin() + (next_coords.empty() ? 0 : 1),
                              next_coords.end());
        offset = node_end_offset;
        current = next;
    }

    util::json::Object route;
    route.values["ref"] = item.reported_ref;
    route.values["start_offset_m"] = item.start_offset;
    const double length = polylineLength(polyline);
    route.values["length_m"] = length;
    route.values["polyline"] = encodePolyline<100000>(polyline.cbegin(), polyline.cend());
    if (debug)
    {
        route.values["junctions"] = std::move(junctions_debug);
        route.values["non_motorway_m"] = non_motorway_len;
    }

    WalkOutput out;
    out.route = std::move(route);
    out.children = std::move(children);
    out.length = length;
    out.dir_samples = sampleAlong(polyline);
    out.end_coord = polyline.empty() ? item.start_coords.front() : polyline.back();
    out.bad_ending = !stopped_by_cap;
    out.rejoined_ancestor = rejoined_ancestor;
    out.ended_by_cycle = ended_by_cycle;
    out.path_nodes = std::move(path_nodes);
    out.refined_toward = std::move(refined_toward);
    out.forks = std::move(forks);
    out.direction_forks = std::move(direction_forks);
    out.parallel_forks = std::move(parallel_forks);
    return out;
}

// Walk a segment, then for a primary branch (not itself a forced sibling) add two repairs:
//  * Landing rescue - if it lands badly (short, childless, ended in a cycle rather than the cap),
//    retry its early forks and keep the alternative that reaches furthest (the through-lane past a
//    stack service loop, e.g. the A12 east at Prins Clausplein).
//  * Crossing directions - a single ramp onto a crossing road reaches both its travel directions
//    via a fork; the walk follows one and suppresses the other. Spawn each suppressed same-ref
//    direction that reaches a real road (not a cycle) as a sibling branch of this segment's parent
//    (the A12 west / Utrechtsebaan alongside the A12 east). Bounded for determinism and cost.
WalkOutput expandSegment(const datafacade::BaseDataFacade &facade,
                         const MLDFacade &mld,
                         const WorkItem &item,
                         const double hard_cap,
                         const bool debug)
{
    WalkOutput best =
        walkOne(facade, mld, item, hard_cap, debug, item.override_node, item.override_target);
    if (item.override_node != SPECIAL_NODEID)
        return best; // a sibling is itself a forced direction; no further rescue or spawning

    const auto forks = best.forks;
    const auto direction_forks = best.direction_forks;

    // Landing rescue.
    if (best.bad_ending && best.children.empty() && best.length < RETRY_MIN_M)
    {
        std::size_t tries = 0;
        for (const auto &fork : forks)
        {
            if (tries >= MAX_LANDING_RETRIES)
                break;
            ++tries;
            WalkOutput candidate =
                walkOne(facade, mld, item, hard_cap, debug, fork.first, fork.second);
            if (candidate.length > best.length)
                best = std::move(candidate);
        }
    }

    // Crossing directions -> siblings. Dedupe by where they end: several forks can lead to the
    // same alignment, and the primary already covers its own direction.
    std::vector<util::Coordinate> accepted_ends{best.end_coord};
    std::size_t dtries = 0;
    for (const auto &df : direction_forks)
    {
        if (dtries >= MAX_CROSSING_DIRECTIONS)
            break;
        ++dtries;
        const WalkOutput candidate = walkOne(facade, mld, item, hard_cap, debug, df.first, df.second);
        if (candidate.ended_by_cycle || candidate.rejoined_ancestor ||
            candidate.length < MIN_DIRECTION_M)
            continue; // a loop/stub, not a genuine second direction
        if (std::any_of(accepted_ends.begin(),
                        accepted_ends.end(),
                        [&](const util::Coordinate c) {
                            return util::coordinate_calculation::greatCircleDistance(
                                       c, candidate.end_coord) < SAME_DIRECTION_M;
                        }))
            continue; // same alignment as an already-kept direction
        accepted_ends.push_back(candidate.end_coord);

        WorkItem sib = item;
        sib.override_node = df.first;
        sib.override_target = df.second;
        auto arm_toward = towardArray(
            std::string(facade.GetDestinationsForID(facade.GetNameIndex(df.second))));
        if (!arm_toward.values.empty())
            sib.junction.values["toward"] = std::move(arm_toward);
        best.siblings.push_back(std::move(sib));
    }

    // Same-road parallel carriageways -> lift the junctions the through carriageway skipped. Probe
    // each recorded parallelbaan arm from the split, seeded with the through walk's full path so it
    // stops where the parallelbaan rejoins us (it then only surfaces the junctions between split and
    // rejoin, not a parallel copy of the rest of the tree). The parallelbaan itself is never emitted
    // - only its crossing children, which become children of this segment and are de-duplicated at
    // assembly against the through carriageway's own (same-ref end-coord / arc-overlap). See §S3-fix9.
    for (const auto &pf : best.parallel_forks)
    {
        WorkItem probe = item;
        probe.override_node = SPECIAL_NODEID;
        probe.override_target = SPECIAL_NODEID;
        probe.start_node = pf.second;
        probe.start_coords = nodeForwardCoordinates(facade, pf.second);
        probe.start_offset = pf.first;
        probe.reported_ref = item.reported_ref;
        for (const auto n : best.path_nodes)
            probe.visited.insert(n);
        const double probe_cap = std::min(hard_cap, pf.first + PARALLELBAAN_PROBE_M);
        WalkOutput pout = walkOne(facade, mld, probe, probe_cap, debug);
        for (auto &child : pout.children)
            best.children.push_back(std::move(child));
    }
    return best;
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

    // Snap to several bearing-filtered candidates (the R-tree returns them nearest-first). Inside a
    // stack the nearest motorway alignment can be a service loop that truncates the whole corridor
    // ~2 km ahead; if the root walk from a candidate lands badly, fall back to the next.
    auto phantom_nodes = GetPhantomNodes(facade, params, 2 * MAX_ROOT_CANDIDATES);
    if (phantom_nodes.front().empty())
    {
        return Error("NoSegment", "Could not find a matching segment for coordinate", result);
    }

    // Build a root WorkItem for a snap candidate, or nullopt if its heading-matched carriageway is
    // not on a motorway (Stage 1 finding 7.7: the snap API has no class filter, so an off-network
    // point snaps to the nearest road rather than failing).
    const auto make_root = [&](const PhantomNode &phantom) -> std::optional<WorkItem>
    {
        const bool forward_ok = phantom.forward_segment_id.enabled;
        const bool reverse_ok = phantom.reverse_segment_id.enabled;
        const auto forward_delta =
            forward_ok ? bearingDelta(heading, phantom.GetBearing(false)) : 360.0;
        const auto reverse_delta =
            reverse_ok ? bearingDelta(heading, phantom.GetBearing(true)) : 360.0;
        const NodeID start_node = (forward_delta <= reverse_delta) ? phantom.forward_segment_id.id
                                                                   : phantom.reverse_segment_id.id;
        if (start_node == SPECIAL_NODEID || !isMotorwayNode(facade, start_node))
            return std::nullopt;

        // Begin the root polyline at the vertex of the start node nearest the snap.
        const auto node_coords = nodeForwardCoordinates(facade, start_node);
        std::size_t snap_index = 0;
        double closest = std::numeric_limits<double>::max();
        for (std::size_t i = 0; i < node_coords.size(); ++i)
        {
            const auto d = util::coordinate_calculation::greatCircleDistance(node_coords[i],
                                                                             phantom.location);
            if (d < closest)
            {
                closest = d;
                snap_index = i;
            }
        }

        WorkItem root;
        root.id = 0;
        root.parent_id = NO_PARENT;
        root.start_node = start_node;
        root.reported_ref = std::string(facade.GetRefForID(facade.GetNameIndex(start_node)));
        root.start_coords.assign(node_coords.begin() + snap_index, node_coords.end());
        root.start_offset = 0.0;
        root.visited = {start_node};
        root.depth = 0;
        return root;
    };

    // Try candidates in nearest-first order; take the first whose root walk lands well (reaches the
    // retry threshold, spawns a branch, or runs to the cap), else keep the furthest-reaching one.
    std::optional<WorkItem> chosen;
    util::Coordinate snapped_location;
    double chosen_len = -1.0;
    std::size_t tried = 0;
    for (const auto &candidate : phantom_nodes.front())
    {
        if (tried >= MAX_ROOT_CANDIDATES)
            break;
        auto root = make_root(candidate.phantom_node);
        if (!root)
            continue;
        ++tried;
        const auto probe =
            expandSegment(facade, *mld, *root, static_cast<double>(params.hard_cap_m), false);
        const bool good =
            probe.length >= RETRY_MIN_M || !probe.children.empty() || !probe.bad_ending;
        if (!chosen || good || probe.length > chosen_len)
        {
            chosen = std::move(root);
            snapped_location = candidate.phantom_node.location;
            chosen_len = probe.length;
        }
        if (good)
            break;
    }

    if (!chosen)
    {
        return Error("NoSegment", "Snapped coordinate is not on a motorway", result);
    }

    // Class-weighted best-first expansion (§S3-fix9, the 2026-07-08 budget-priority ruling). The
    // heap orders by (priority_class, start_offset): the root chain and every first-level spur head
    // are class 0 (guaranteed a slot at ANY distance - "always prioritise the current road and the
    // first spurs"), each spur's subtree within SPUR_RESERVED_DEPTH_M of its head is class 1, and
    // everything deeper is class 2 filled nearest-first (the old behaviour). So when MAX_SEGMENTS
    // truncates a dense tree, a far first-level spur is no longer starved by a near spur's deep
    // subtree. Parent-before-child still holds structurally: a child is only enqueued after its
    // parent is expanded and recorded, independent of class.
    std::vector<WorkItem> pool;
    std::vector<std::size_t> parent_of; // pool id -> parent pool id (for climbing the ancestor path)
    std::unordered_map<std::size_t, std::vector<NodeID>> segment_path; // pool id -> its walked nodes
    std::priority_queue<Pending, std::vector<Pending>, std::greater<>> pq;
    const auto enqueue = [&](WorkItem &&w)
    {
        // Propagate the spur head: the root has none (-1); a first-level branch (child of the root,
        // parent_id 0) is its own spur head; deeper items inherit their parent's.
        if (w.parent_id == NO_PARENT)
            w.spur_head_offset = -1.0;
        else if (w.parent_id == 0)
            w.spur_head_offset = w.start_offset;
        else
            w.spur_head_offset = pool[w.parent_id].spur_head_offset;
        parent_of.push_back(w.parent_id);
        pq.push({priorityClass(w), w.start_offset, w.id});
        pool.push_back(std::move(w));
    };

    enqueue(std::move(*chosen));

    std::vector<SegmentResult> segments;
    util::json::Array dropped_stubs; // debug-only: parallel-carriageway stubs dropped at rejoin
    while (!pq.empty() && segments.size() < static_cast<std::size_t>(MAX_SEGMENTS))
    {
        const auto id = std::get<2>(pq.top());
        pq.pop();
        WorkItem item = std::move(pool[id]);

        // Complete the per-path visited set with every ancestor's FULL walked path. The set the
        // child was spawned with only reached the spawn junction; best-first guarantees each
        // ancestor is fully walked (and its path recorded) before this item pops. Without this a
        // parallel carriageway that leaves the mainline and rejoins it further on (the A4
        // parallelbaan mis-signed A13 at KP Ypenburg) never sees the rejoin and re-expands a
        // duplicate copy of the whole tree.
        for (std::size_t p = item.parent_id; p != NO_PARENT; p = parent_of[p])
        {
            const auto it = segment_path.find(p);
            if (it != segment_path.end())
                item.visited.insert(it->second.begin(), it->second.end());
        }

        WalkOutput out =
            expandSegment(facade, *mld, item, static_cast<double>(params.hard_cap_m), params.debug);
        segment_path[item.id] = std::move(out.path_nodes);

        // Drop a childless branch that just merges back onto an ancestor path: it is a parallel
        // carriageway of a road already in the tree, its stations are within metres of the main
        // polyline, so the corridor join loses nothing. Branches that spawned children stay
        // (parallelbanen carrying real exits; genuine ring routes) - only the bare stub is dropped.
        if (item.parent_id != NO_PARENT && out.rejoined_ancestor && out.children.empty())
        {
            if (params.debug)
            {
                util::json::Object d;
                d.values["ref"] = item.reported_ref;
                d.values["start_offset_m"] = item.start_offset;
                d.values["length_m"] = out.length;
                d.values["polyline"] = std::move(out.route.values["polyline"]);
                dropped_stubs.values.emplace_back(std::move(d));
            }
            continue;
        }

        // Enqueue the chosen walk's children (id assigned here = its future pool index).
        for (auto &child : out.children)
        {
            child.id = pool.size();
            child.parent_id = item.id;
            enqueue(std::move(child));
        }

        // Sibling direction branches attach to this segment's PARENT (both directions of a crossing
        // road diverge from the same junction), so leave their parent_id as set by expandSegment.
        for (auto &sibling : out.siblings)
        {
            sibling.id = pool.size();
            enqueue(std::move(sibling));
        }

        // Refine merged junction signage: when this (primary) branch forks into same-ref directions
        // just past the junction, the signage captured at spawn was the shared slip-road union;
        // replace it with the destinations of the arm this direction actually took. Siblings already
        // carry their own arm's signage (set in expandSegment), so leave those untouched.
        if (item.override_node == SPECIAL_NODEID && !out.refined_toward.empty())
            item.junction.values["toward"] = towardArray(out.refined_toward);

        SegmentResult segment;
        segment.id = item.id;
        segment.parent_id = item.parent_id;
        segment.order_key = item.start_offset;
        segment.ref = item.reported_ref;
        segment.end_coord = out.end_coord;
        segment.dir_samples = std::move(out.dir_samples);
        segment.junction = std::move(item.junction);
        segment.route = std::move(out.route);
        segments.push_back(std::move(segment));
    }

    // Reattach children to parents (a parent always completes before its children, since a child's
    // start_offset exceeds its parent's), each parent's branches sorted nearest-first.
    std::unordered_map<std::size_t, std::size_t> result_index;
    for (std::size_t i = 0; i < segments.size(); ++i)
        result_index[segments[i].id] = i;
    std::unordered_map<std::size_t, std::vector<std::size_t>> children;
    for (std::size_t i = 0; i < segments.size(); ++i)
        if (segments[i].parent_id != NO_PARENT)
            children[segments[i].parent_id].push_back(i);
    for (auto &entry : children)
    {
        std::sort(entry.second.begin(),
                  entry.second.end(),
                  [&](std::size_t a, std::size_t b)
                  {
                      return segments[a].order_key < segments[b].order_key ||
                             (segments[a].order_key == segments[b].order_key &&
                              segments[a].id < segments[b].id);
                  });

        // Drop same-direction duplicates. A crossing's two directions end far apart, but the same
        // physical corridor can appear twice as same-ref siblings: a direction sibling landing on an
        // already-kept child's alignment (different spawn paths), or two qualifying entry ramps onto
        // one crossing direction (KP Muiderberg's twin A6 lanes). The first ends at the same node
        // (end-coord check); the second runs together from a coincident start but ends km apart when
        // the copies truncate differently (arc-distance overlap check). Keep the nearest (first after
        // the sort).
        const auto same_corridor = [&](std::size_t a, std::size_t b)
        {
            if (segments[a].ref != segments[b].ref)
                return false;
            if (util::coordinate_calculation::greatCircleDistance(
                    segments[a].end_coord, segments[b].end_coord) < SAME_DIRECTION_M)
                return true;
            const auto &sa = segments[a].dir_samples;
            const auto &sb = segments[b].dir_samples;
            const std::size_t n = std::min(sa.size(), sb.size());
            if (n < 2)
                return false;
            std::size_t matched = 0;
            for (std::size_t i = 0; i < n; ++i)
                if (util::coordinate_calculation::greatCircleDistance(sa[i], sb[i]) < DIR_MATCH_M)
                    ++matched;
            return matched >= static_cast<std::size_t>(DIR_OVERLAP_FRAC * n + 0.5);
        };
        std::vector<std::size_t> kept;
        for (const auto idx : entry.second)
        {
            const bool dup = std::any_of(kept.begin(), kept.end(),
                                         [&](std::size_t k) { return same_corridor(k, idx); });
            if (!dup)
                kept.push_back(idx);
        }
        entry.second = std::move(kept);
    }

    std::function<util::json::Object(std::size_t)> assemble = [&](std::size_t idx)
    {
        util::json::Object route = std::move(segments[idx].route);
        util::json::Array branches;
        const auto found = children.find(segments[idx].id);
        if (found != children.end())
        {
            for (const auto child_idx : found->second)
            {
                util::json::Object branch;
                branch.values["junction"] = std::move(segments[child_idx].junction);
                branch.values["route"] = assemble(child_idx);
                branches.values.emplace_back(std::move(branch));
            }
        }
        route.values["branches"] = std::move(branches);
        return route;
    };
    auto root_route = assemble(result_index[0]);

    result = util::json::Object();
    auto &response = std::get<util::json::Object>(result);
    response.values["code"] = "Ok";
    response.values["ref"] = std::move(root_route.values["ref"]);
    response.values["start_offset_m"] = std::move(root_route.values["start_offset_m"]);
    response.values["length_m"] = std::move(root_route.values["length_m"]);
    response.values["polyline"] = std::move(root_route.values["polyline"]);
    response.values["branches"] = std::move(root_route.values["branches"]);
    if (params.debug)
    {
        response.values["junctions"] = std::move(root_route.values["junctions"]);
        response.values["dropped_stubs"] = std::move(dropped_stubs);
    }
    response.values["segment_count"] = static_cast<double>(segments.size());

    util::json::Object snapped;
    snapped.values["location"] = makeCoordinate(snapped_location);
    snapped.values["input"] = makeCoordinate(params.coordinates.front());
    snapped.values["requested_bearing"] = heading;
    response.values["snapped"] = std::move(snapped);

    return Status::Ok;
}
} // namespace osrm::engine::plugins

#include "extractor/scripting_environment_lua.hpp"

#include "extractor/extraction_helper_functions.hpp"
#include "extractor/extraction_node.hpp"
#include "extractor/extraction_relation.hpp"
#include "extractor/extraction_segment.hpp"
#include "extractor/extraction_turn.hpp"
#include "extractor/extraction_way.hpp"
#include "extractor/internal_extractor_edge.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/query_node.hpp"
#include "extractor/raster_source.hpp"
#include "extractor/restriction_parser.hpp"
#include "util/coordinate.hpp"
#include "util/exception.hpp"
#include "util/log.hpp"
#include "util/lua_util.hpp"
#include "util/typedefs.hpp"

#include <osmium/osm.hpp>

#include <tbb/parallel_for.h>

#include <memory>
#include <sstream>

namespace sol
{
template <> struct is_container<osmium::Node> : std::false_type
{
};
template <> struct is_container<osmium::Way> : std::false_type
{
};
template <> struct is_container<osmium::Relation> : std::false_type
{
};
}

namespace osrm
{
namespace extractor
{

template <class T>
auto get_value_by_key(T const &object, const char *key) -> decltype(object.get_value_by_key(key))
{
    auto v = object.get_value_by_key(key);
    if (v && *v)
    { // non-empty string?
        return v;
    }
    else
    {
        return nullptr;
    }
}

template <class T, class D>
const char *get_value_by_key(T const &object, const char *key, D const default_value)
{
    auto v = get_value_by_key(object, key);
    if (v && *v)
    {
        return v;
    }
    else
    {
        return default_value;
    }
}

template <class T> double latToDouble(T const &object)
{
    return static_cast<double>(util::toFloating(object.lat));
}

template <class T> double lonToDouble(T const &object)
{
    return static_cast<double>(util::toFloating(object.lon));
}

Sol2ScriptingEnvironment::Sol2ScriptingEnvironment(
    const std::string &file_name, const boost::filesystem::path &location_dependent_data_path)
    : file_name(file_name), location_dependent_data(location_dependent_data_path)
{
    util::Log() << "Using script " << file_name;
}

void Sol2ScriptingEnvironment::InitContext(LuaScriptingContext &context)
{
    context.state.open_libraries();

    context.state["durationIsValid"] = durationIsValid;
    context.state["parseDuration"] = parseDuration;
    context.state["trimLaneString"] = trimLaneString;
    context.state["applyAccessTokens"] = applyAccessTokens;
    context.state["canonicalizeStringList"] = canonicalizeStringList;

    context.state.new_enum("mode",
                           "inaccessible",
                           TRAVEL_MODE_INACCESSIBLE,
                           "driving",
                           TRAVEL_MODE_DRIVING,
                           "cycling",
                           TRAVEL_MODE_CYCLING,
                           "walking",
                           TRAVEL_MODE_WALKING,
                           "ferry",
                           TRAVEL_MODE_FERRY,
                           "train",
                           TRAVEL_MODE_TRAIN,
                           "pushing_bike",
                           TRAVEL_MODE_PUSHING_BIKE,
                           "steps_up",
                           TRAVEL_MODE_STEPS_UP,
                           "steps_down",
                           TRAVEL_MODE_STEPS_DOWN,
                           "river_up",
                           TRAVEL_MODE_RIVER_UP,
                           "river_down",
                           TRAVEL_MODE_RIVER_DOWN,
                           "route",
                           TRAVEL_MODE_ROUTE);

    context.state.new_enum("road_priority_class",
                           "motorway",
                           extractor::guidance::RoadPriorityClass::MOTORWAY,
                           "trunk",
                           extractor::guidance::RoadPriorityClass::TRUNK,
                           "primary",
                           extractor::guidance::RoadPriorityClass::PRIMARY,
                           "secondary",
                           extractor::guidance::RoadPriorityClass::SECONDARY,
                           "tertiary",
                           extractor::guidance::RoadPriorityClass::TERTIARY,
                           "main_residential",
                           extractor::guidance::RoadPriorityClass::MAIN_RESIDENTIAL,
                           "side_residential",
                           extractor::guidance::RoadPriorityClass::SIDE_RESIDENTIAL,
                           "link_road",
                           extractor::guidance::RoadPriorityClass::LINK_ROAD,
                           "bike_path",
                           extractor::guidance::RoadPriorityClass::BIKE_PATH,
                           "foot_path",
                           extractor::guidance::RoadPriorityClass::FOOT_PATH,
                           "connectivity",
                           extractor::guidance::RoadPriorityClass::CONNECTIVITY);

    context.state.new_enum("turn_type",
                           "invalid",
                           extractor::guidance::TurnType::Invalid,
                           "new_name",
                           extractor::guidance::TurnType::NewName,
                           "continue",
                           extractor::guidance::TurnType::Continue,
                           "turn",
                           extractor::guidance::TurnType::Turn,
                           "merge",
                           extractor::guidance::TurnType::Merge,
                           "on_ramp",
                           extractor::guidance::TurnType::OnRamp,
                           "off_ramp",
                           extractor::guidance::TurnType::OffRamp,
                           "fork",
                           extractor::guidance::TurnType::Fork,
                           "end_of_road",
                           extractor::guidance::TurnType::EndOfRoad,
                           "notification",
                           extractor::guidance::TurnType::Notification,
                           "enter_roundabout",
                           extractor::guidance::TurnType::EnterRoundabout,
                           "enter_and_exit_roundabout",
                           extractor::guidance::TurnType::EnterAndExitRoundabout,
                           "enter_rotary",
                           extractor::guidance::TurnType::EnterRotary,
                           "enter_and_exit_rotary",
                           extractor::guidance::TurnType::EnterAndExitRotary,
                           "enter_roundabout_intersection",
                           extractor::guidance::TurnType::EnterRoundaboutIntersection,
                           "enter_and_exit_roundabout_intersection",
                           extractor::guidance::TurnType::EnterAndExitRoundaboutIntersection,
                           "use_lane",
                           extractor::guidance::TurnType::Suppressed,
                           "no_turn",
                           extractor::guidance::TurnType::NoTurn,
                           "suppressed",
                           extractor::guidance::TurnType::Suppressed,
                           "enter_roundabout_at_exit",
                           extractor::guidance::TurnType::EnterRoundaboutAtExit,
                           "exit_roundabout",
                           extractor::guidance::TurnType::ExitRoundabout,
                           "enter_rotary_at_exit",
                           extractor::guidance::TurnType::EnterRotaryAtExit,
                           "exit_rotary",
                           extractor::guidance::TurnType::ExitRotary,
                           "enter_roundabout_intersection_at_exit",
                           extractor::guidance::TurnType::EnterRoundaboutIntersectionAtExit,
                           "exit_roundabout_intersection",
                           extractor::guidance::TurnType::ExitRoundaboutIntersection,
                           "stay_on_roundabout",
                           extractor::guidance::TurnType::StayOnRoundabout,
                           "sliproad",
                           extractor::guidance::TurnType::Sliproad);

    context.state.new_enum("direction_modifier",
                           "u_turn",
                           extractor::guidance::DirectionModifier::UTurn,
                           "sharp_right",
                           extractor::guidance::DirectionModifier::SharpRight,
                           "right",
                           extractor::guidance::DirectionModifier::Right,
                           "slight_right",
                           extractor::guidance::DirectionModifier::SlightRight,
                           "straight",
                           extractor::guidance::DirectionModifier::Straight,
                           "slight_left",
                           extractor::guidance::DirectionModifier::SlightLeft,
                           "left",
                           extractor::guidance::DirectionModifier::Left,
                           "sharp_left",
                           extractor::guidance::DirectionModifier::SharpLeft);

    context.state.new_enum("item_type",
                           "node",
                           osmium::item_type::node,
                           "way",
                           osmium::item_type::way,
                           "relation",
                           osmium::item_type::relation);

    context.state.new_usertype<RasterContainer>("raster",
                                                "load",
                                                &RasterContainer::LoadRasterSource,
                                                "query",
                                                &RasterContainer::GetRasterDataFromSource,
                                                "interpolate",
                                                &RasterContainer::GetRasterInterpolateFromSource);

    context.state.new_usertype<ProfileProperties>(
        "ProfileProperties",
        "traffic_signal_penalty",
        sol::property(&ProfileProperties::GetTrafficSignalPenalty,
                      &ProfileProperties::SetTrafficSignalPenalty),
        "u_turn_penalty",
        sol::property(&ProfileProperties::GetUturnPenalty, &ProfileProperties::SetUturnPenalty),
        "max_speed_for_map_matching",
        sol::property(&ProfileProperties::GetMaxSpeedForMapMatching,
                      &ProfileProperties::SetMaxSpeedForMapMatching),
        "continue_straight_at_waypoint",
        &ProfileProperties::continue_straight_at_waypoint,
        "use_turn_restrictions",
        &ProfileProperties::use_turn_restrictions,
        "left_hand_driving",
        &ProfileProperties::left_hand_driving,
        "weight_precision",
        &ProfileProperties::weight_precision,
        "weight_name",
        sol::property(&ProfileProperties::SetWeightName, &ProfileProperties::GetWeightName),
        "max_turn_weight",
        sol::property(&ProfileProperties::GetMaxTurnWeight),
        "force_split_edges",
        &ProfileProperties::force_split_edges,
        "call_tagless_node_function",
        &ProfileProperties::call_tagless_node_function);

    context.state.new_usertype<std::vector<std::string>>(
        "vector",
        "Add",
        static_cast<void (std::vector<std::string>::*)(const std::string &)>(
            &std::vector<std::string>::push_back));

    context.state.new_usertype<osmium::Location>("Location",
                                                 "lat",
                                                 &osmium::Location::lat,
                                                 "lon",
                                                 &osmium::Location::lon,
                                                 "valid",
                                                 &osmium::Location::valid);

    context.state.new_usertype<osmium::Way>(
        "Way",
        "get_value_by_key",
        &get_value_by_key<osmium::Way>,
        "id",
        &osmium::Way::id,
        "get_nodes",
        [](const osmium::Way &way) { return sol::as_table(way.nodes()); },
        "version",
        &osmium::Way::version);

    context.state.new_usertype<osmium::RelationMember>(
        "RelationMember",
        "role",
        &osmium::RelationMember::role,
        "type",
        &osmium::RelationMember::type,
        "id",
        [](const osmium::RelationMember &member) -> osmium::object_id_type {
            return member.ref();
        });

    context.state.new_usertype<osmium::Relation>(
        "Relation",
        "get_value_by_key",
        &get_value_by_key<osmium::Relation>,
        "id",
        &osmium::Relation::id,
        "version",
        &osmium::Relation::version,
        "members",
        [](const osmium::Relation &rel) -> const osmium::RelationMemberList & {
            return rel.members();
        });

    context.state.new_usertype<osmium::Node>("Node",
                                             "location",
                                             &osmium::Node::location,
                                             "get_value_by_key",
                                             &get_value_by_key<osmium::Node>,
                                             "id",
                                             &osmium::Node::id,
                                             "version",
                                             &osmium::Node::version);

    context.state.new_usertype<ExtractionNode>("ResultNode",
                                               "traffic_lights",
                                               &ExtractionNode::traffic_lights,
                                               "barrier",
                                               &ExtractionNode::barrier);

    context.state.new_usertype<guidance::RoadClassification>(
        "RoadClassification",
        "motorway_class",
        sol::property(&guidance::RoadClassification::IsMotorwayClass,
                      &guidance::RoadClassification::SetMotorwayFlag),
        "link_class",
        sol::property(&guidance::RoadClassification::IsLinkClass,
                      &guidance::RoadClassification::SetLinkClass),
        "may_be_ignored",
        sol::property(&guidance::RoadClassification::IsLowPriorityRoadClass,
                      &guidance::RoadClassification::SetLowPriorityFlag),
        "road_priority_class",
        sol::property(&guidance::RoadClassification::GetClass,
                      &guidance::RoadClassification::SetClass),
        "num_lanes",
        sol::property(&guidance::RoadClassification::GetNumberOfLanes,
                      &guidance::RoadClassification::SetNumberOfLanes));

    context.state.new_usertype<ExtractionWay>(
        "ResultWay",
        "forward_speed",
        &ExtractionWay::forward_speed,
        "backward_speed",
        &ExtractionWay::backward_speed,
        "forward_rate",
        &ExtractionWay::forward_rate,
        "backward_rate",
        &ExtractionWay::backward_rate,
        "name",
        sol::property(&ExtractionWay::GetName, &ExtractionWay::SetName),
        "ref",
        sol::property(&ExtractionWay::GetRef, &ExtractionWay::SetRef),
        "pronunciation",
        sol::property(&ExtractionWay::GetPronunciation, &ExtractionWay::SetPronunciation),
        "destinations",
        sol::property(&ExtractionWay::GetDestinations, &ExtractionWay::SetDestinations),
        "exits",
        sol::property(&ExtractionWay::GetExits, &ExtractionWay::SetExits),
        "turn_lanes_forward",
        sol::property(&ExtractionWay::GetTurnLanesForward, &ExtractionWay::SetTurnLanesForward),
        "turn_lanes_backward",
        sol::property(&ExtractionWay::GetTurnLanesBackward, &ExtractionWay::SetTurnLanesBackward),
        "duration",
        &ExtractionWay::duration,
        "weight",
        &ExtractionWay::weight,
        "road_classification",
        &ExtractionWay::road_classification,
        "forward_classes",
        &ExtractionWay::forward_classes,
        "backward_classes",
        &ExtractionWay::backward_classes,
        "forward_mode",
        sol::property([](const ExtractionWay &way) { return way.forward_travel_mode; },
                      [](ExtractionWay &way, TravelMode mode) { way.forward_travel_mode = mode; }),
        "backward_mode",
        sol::property([](const ExtractionWay &way) { return way.backward_travel_mode; },
                      [](ExtractionWay &way, TravelMode mode) { way.backward_travel_mode = mode; }),
        "roundabout",
        sol::property([](const ExtractionWay &way) { return way.roundabout; },
                      [](ExtractionWay &way, bool flag) { way.roundabout = flag; }),
        "circular",
        sol::property([](const ExtractionWay &way) { return way.circular; },
                      [](ExtractionWay &way, bool flag) { way.circular = flag; }),
        "is_startpoint",
        sol::property([](const ExtractionWay &way) { return way.is_startpoint; },
                      [](ExtractionWay &way, bool flag) { way.is_startpoint = flag; }),
        "forward_restricted",
        sol::property([](const ExtractionWay &way) { return way.forward_restricted; },
                      [](ExtractionWay &way, bool flag) { way.forward_restricted = flag; }),
        "backward_restricted",
        sol::property([](const ExtractionWay &way) { return way.backward_restricted; },
                      [](ExtractionWay &way, bool flag) { way.backward_restricted = flag; }),
        "is_left_hand_driving",
        sol::property([](const ExtractionWay &way) { return way.is_left_hand_driving; },
                      [](ExtractionWay &way, bool flag) { way.is_left_hand_driving = flag; }));

    context.state.new_usertype<ExtractionRelation>(
        "ExtractionRelation",
        sol::meta_function::new_index,
        [](ExtractionRelation &rel, const osmium::RelationMember &member)
            -> ExtractionRelation::AttributesMap & { return rel.GetMember(member); },
        sol::meta_function::index,
        [](ExtractionRelation &rel, const osmium::RelationMember &member)
            -> ExtractionRelation::AttributesMap & { return rel.GetMember(member); },
        "restriction",
        sol::property([](const ExtractionRelation &rel) { return rel.is_restriction; },
                      [](ExtractionRelation &rel, bool flag) { rel.is_restriction = flag; }));

    context.state.new_usertype<ExtractionSegment>("ExtractionSegment",
                                                  "source",
                                                  &ExtractionSegment::source,
                                                  "target",
                                                  &ExtractionSegment::target,
                                                  "distance",
                                                  &ExtractionSegment::distance,
                                                  "weight",
                                                  &ExtractionSegment::weight,
                                                  "duration",
                                                  &ExtractionSegment::duration);

    context.state.new_usertype<ExtractionTurn>("ExtractionTurn",
                                               "angle",
                                               &ExtractionTurn::angle,
                                               "turn_type",
                                               &ExtractionTurn::turn_type,
                                               "direction_modifier",
                                               &ExtractionTurn::direction_modifier,
                                               "has_traffic_light",
                                               &ExtractionTurn::has_traffic_light,
                                               "weight",
                                               &ExtractionTurn::weight,
                                               "duration",
                                               &ExtractionTurn::duration,
                                               "source_restricted",
                                               &ExtractionTurn::source_restricted,
                                               "target_restricted",
                                               &ExtractionTurn::target_restricted);

    // Keep in mind .location is available only if .pbf is preprocessed to set the location with the
    // ref using osmium command "osmium add-locations-to-ways"
    context.state.new_usertype<osmium::NodeRef>(
        "NodeRef", "id", &osmium::NodeRef::ref, "location", [](const osmium::NodeRef &nref) {
            return nref.location();
        });

    context.state.new_usertype<InternalExtractorEdge>("EdgeSource",
                                                      "source_coordinate",
                                                      &InternalExtractorEdge::source_coordinate,
                                                      "weight",
                                                      &InternalExtractorEdge::weight_data,
                                                      "duration",
                                                      &InternalExtractorEdge::duration_data);

    context.state.new_usertype<QueryNode>(
        "EdgeTarget", "lon", &lonToDouble<QueryNode>, "lat", &latToDouble<QueryNode>);

    context.state.new_usertype<util::Coordinate>("Coordinate",
                                                 "lon",
                                                 sol::property(&lonToDouble<util::Coordinate>),
                                                 "lat",
                                                 sol::property(&latToDouble<util::Coordinate>));

    context.state.new_usertype<RasterDatum>(
        "RasterDatum", "datum", &RasterDatum::datum, "invalid_data", &RasterDatum::get_invalid);

    // the "properties" global is only used in v1 of the api, but we don't know
    // the version until we have read the file. so we have to declare it in any case.
    // we will then clear it for v2 profiles after reading the file
    context.state["properties"] = &context.properties;

    //
    // end of register block
    //

    util::luaAddScriptFolderToLoadPath(context.state.lua_state(), file_name.c_str());

    sol::optional<sol::table> function_table = context.state.script_file(file_name);

    // Check profile API version
    auto maybe_version = context.state.get<sol::optional<int>>("api_version");
    if (maybe_version)
        context.api_version = *maybe_version;
    else
    {
        context.api_version = 0;
    }

    if (context.api_version < SUPPORTED_MIN_API_VERSION ||
        context.api_version > SUPPORTED_MAX_API_VERSION)
    {
        throw util::exception("Invalid profile API version " + std::to_string(context.api_version) +
                              " only versions from " + std::to_string(SUPPORTED_MIN_API_VERSION) +
                              " to " + std::to_string(SUPPORTED_MAX_API_VERSION) +
                              " are supported." + SOURCE_REF);
    }

    util::Log() << "Using profile api version " << context.api_version;

    // version-dependent parts of the api
    auto initV2Context = [&]() {
        // clear global not used in v2
        context.state["properties"] = sol::nullopt;

        // check function table
        if (function_table == sol::nullopt)
            throw util::exception("Profile must return a function table.");

        // setup helpers
        context.state["raster"] = &context.raster_sources;

        // set constants
        context.state.new_enum("constants",
                               "precision",
                               COORDINATE_PRECISION,
                               "max_turn_weight",
                               std::numeric_limits<TurnPenalty>::max());

        // call initialize function
        sol::function setup_function = function_table.value()["setup"];
        if (!setup_function.valid())
            throw util::exception("Profile must have an setup() function.");
        sol::optional<sol::table> profile_table = setup_function();
        if (profile_table == sol::nullopt)
            throw util::exception("Profile setup() must return a table.");
        else
            context.profile_table = profile_table.value();

        // store functions
        context.turn_function = function_table.value()["process_turn"];
        context.node_function = function_table.value()["process_node"];
        context.way_function = function_table.value()["process_way"];
        context.segment_function = function_table.value()["process_segment"];

        context.has_turn_penalty_function = context.turn_function.valid();
        context.has_node_function = context.node_function.valid();
        context.has_way_function = context.way_function.valid();
        context.has_segment_function = context.segment_function.valid();

        // read properties from 'profile.properties' table
        sol::table properties = context.profile_table["properties"];
        if (properties.valid())
        {
            sol::optional<std::string> weight_name = properties["weight_name"];
            if (weight_name != sol::nullopt)
                context.properties.SetWeightName(weight_name.value());

            sol::optional<std::int32_t> traffic_signal_penalty =
                properties["traffic_signal_penalty"];
            if (traffic_signal_penalty != sol::nullopt)
                context.properties.SetTrafficSignalPenalty(traffic_signal_penalty.value());

            sol::optional<std::int32_t> u_turn_penalty = properties["u_turn_penalty"];
            if (u_turn_penalty != sol::nullopt)
                context.properties.SetUturnPenalty(u_turn_penalty.value());

            sol::optional<double> max_speed_for_map_matching =
                properties["max_speed_for_map_matching"];
            if (max_speed_for_map_matching != sol::nullopt)
                context.properties.SetMaxSpeedForMapMatching(max_speed_for_map_matching.value());

            sol::optional<bool> continue_straight_at_waypoint =
                properties["continue_straight_at_waypoint"];
            if (continue_straight_at_waypoint != sol::nullopt)
                context.properties.continue_straight_at_waypoint =
                    continue_straight_at_waypoint.value();

            sol::optional<bool> use_turn_restrictions = properties["use_turn_restrictions"];
            if (use_turn_restrictions != sol::nullopt)
                context.properties.use_turn_restrictions = use_turn_restrictions.value();

            sol::optional<bool> left_hand_driving = properties["left_hand_driving"];
            if (left_hand_driving != sol::nullopt)
                context.properties.left_hand_driving = left_hand_driving.value();

            sol::optional<unsigned> weight_precision = properties["weight_precision"];
            if (weight_precision != sol::nullopt)
                context.properties.weight_precision = weight_precision.value();

            sol::optional<bool> force_split_edges = properties["force_split_edges"];
            if (force_split_edges != sol::nullopt)
                context.properties.force_split_edges = force_split_edges.value();
        }
    };

    switch (context.api_version)
    {
    case 3:
    {
        initV2Context();
        context.relation_function = function_table.value()["process_relation"];

        context.has_relation_function = context.relation_function.valid();
        break;
    }

    case 2:
    {
        initV2Context();
        break;
    }
    case 1:
    {
        // cache references to functions for faster execution
        context.turn_function = context.state["turn_function"];
        context.node_function = context.state["node_function"];
        context.way_function = context.state["way_function"];
        context.segment_function = context.state["segment_function"];

        context.has_turn_penalty_function = context.turn_function.valid();
        context.has_node_function = context.node_function.valid();
        context.has_way_function = context.way_function.valid();
        context.has_segment_function = context.segment_function.valid();

        // setup helpers
        context.state["sources"] = &context.raster_sources;

        // set constants
        context.state.new_enum("constants", "precision", COORDINATE_PRECISION);

        BOOST_ASSERT(context.properties.GetUturnPenalty() == 0);
        BOOST_ASSERT(context.properties.GetTrafficSignalPenalty() == 0);

        // call source_function if present
        sol::function source_function = context.state["source_function"];
        if (source_function.valid())
        {
            source_function();
        }

        break;
    }
    case 0:
        // cache references to functions for faster execution
        context.turn_function = context.state["turn_function"];
        context.node_function = context.state["node_function"];
        context.way_function = context.state["way_function"];
        context.segment_function = context.state["segment_function"];

        context.has_turn_penalty_function = context.turn_function.valid();
        context.has_node_function = context.node_function.valid();
        context.has_way_function = context.way_function.valid();
        context.has_segment_function = context.segment_function.valid();

        BOOST_ASSERT(context.properties.GetWeightName() == "duration");
        break;
    }
}

const ProfileProperties &Sol2ScriptingEnvironment::GetProfileProperties()
{
    return GetSol2Context().properties;
}

LuaScriptingContext &Sol2ScriptingEnvironment::GetSol2Context()
{
    std::lock_guard<std::mutex> lock(init_mutex);
    bool initialized = false;
    auto &ref = script_contexts.local(initialized);
    if (!initialized)
    {
        ref = std::make_unique<LuaScriptingContext>(location_dependent_data);
        InitContext(*ref);
    }

    return *ref;
}

void Sol2ScriptingEnvironment::ProcessElements(
    const osmium::memory::Buffer &buffer,
    const RestrictionParser &restriction_parser,
    const ExtractionRelationContainer &relations,
    std::vector<std::pair<const osmium::Node &, ExtractionNode>> &resulting_nodes,
    std::vector<std::pair<const osmium::Way &, ExtractionWay>> &resulting_ways,
    std::vector<std::pair<const osmium::Relation &, ExtractionRelation>> &resulting_relations,
    std::vector<InputConditionalTurnRestriction> &resulting_restrictions)
{
    ExtractionNode result_node;
    ExtractionWay result_way;
    ExtractionRelation result_relation;
    auto &local_context = this->GetSol2Context();

    for (auto entity = buffer.cbegin(), end = buffer.cend(); entity != end; ++entity)
    {
        switch (entity->type())
        {
        case osmium::item_type::node:
        {
            const auto &node = static_cast<const osmium::Node &>(*entity);
            result_node.clear();
            if (local_context.has_node_function &&
                (!node.tags().empty() || local_context.properties.call_tagless_node_function))
            {
                const auto &id = ExtractionRelation::OsmIDTyped(node.id(), osmium::item_type::node);
                local_context.ProcessNode(node, result_node, relations.Get(id));
            }
            resulting_nodes.push_back({node, std::move(result_node)});
        }
        break;
        case osmium::item_type::way:
        {
            const osmium::Way &way = static_cast<const osmium::Way &>(*entity);
            result_way.clear();
            if (local_context.has_way_function)
            {
                const auto &id = ExtractionRelation::OsmIDTyped(way.id(), osmium::item_type::way);
                local_context.ProcessWay(way, result_way, relations.Get(id));
            }
            resulting_ways.push_back({way, std::move(result_way)});
        }
        break;
        case osmium::item_type::relation:
        {
            const auto &relation = static_cast<const osmium::Relation &>(*entity);
            if (auto result_res = restriction_parser.TryParse(relation))
            {
                resulting_restrictions.push_back(*result_res);
            }

            if (local_context.api_version > 2)
            {
                result_relation.clear();
                if (local_context.has_relation_function)
                {
                    local_context.ProcessRelation(relation, result_relation);
                }
                resulting_relations.push_back({relation, std::move(result_relation)});
            }
        }
        break;
        default:
            break;
        }
    }
}

std::vector<std::string>
Sol2ScriptingEnvironment::GetStringListFromFunction(const std::string &function_name)
{
    auto &context = GetSol2Context();
    BOOST_ASSERT(context.state.lua_state() != nullptr);
    std::vector<std::string> strings;
    sol::function function = context.state[function_name];
    if (function.valid())
    {
        function(strings);
    }
    return strings;
}

std::vector<std::string>
Sol2ScriptingEnvironment::GetStringListFromTable(const std::string &table_name)
{
    auto &context = GetSol2Context();
    BOOST_ASSERT(context.state.lua_state() != nullptr);
    std::vector<std::string> strings;
    sol::table table = context.profile_table[table_name];
    if (table.valid())
    {
        for (auto &&pair : table)
        {
            strings.push_back(pair.second.as<std::string>());
        }
    }
    return strings;
}

std::vector<std::vector<std::string>>
Sol2ScriptingEnvironment::GetStringListsFromTable(const std::string &table_name)
{
    std::vector<std::vector<std::string>> string_lists;

    auto &context = GetSol2Context();
    BOOST_ASSERT(context.state.lua_state() != nullptr);
    sol::table table = context.profile_table[table_name];
    if (!table.valid())
    {
        return string_lists;
    }

    for (const auto &pair : table)
    {
        sol::table inner_table = pair.second;
        if (!inner_table.valid())
        {
            throw util::exception("Expected a sub-table at " + table_name + "[" +
                                  pair.first.as<std::string>() + "]");
        }

        std::vector<std::string> inner_vector;
        for (const auto &inner_pair : inner_table)
        {
            inner_vector.push_back(inner_pair.first.as<std::string>());
        }
        string_lists.push_back(std::move(inner_vector));
    }

    return string_lists;
}

std::vector<std::vector<std::string>> Sol2ScriptingEnvironment::GetExcludableClasses()
{
    auto &context = GetSol2Context();
    switch (context.api_version)
    {
    case 3:
    case 2:
        return Sol2ScriptingEnvironment::GetStringListsFromTable("excludable");
    default:
        return {};
    }
}

std::vector<std::string> Sol2ScriptingEnvironment::GetClassNames()
{
    auto &context = GetSol2Context();
    switch (context.api_version)
    {
    case 3:
    case 2:
        return Sol2ScriptingEnvironment::GetStringListFromTable("classes");
    default:
        return {};
    }
}

std::vector<std::string> Sol2ScriptingEnvironment::GetNameSuffixList()
{
    auto &context = GetSol2Context();
    switch (context.api_version)
    {
    case 3:
    case 2:
        return Sol2ScriptingEnvironment::GetStringListFromTable("suffix_list");
    case 1:
        return Sol2ScriptingEnvironment::GetStringListFromFunction("get_name_suffix_list");
    default:
        return {};
    }
}

std::vector<std::string> Sol2ScriptingEnvironment::GetRestrictions()
{
    auto &context = GetSol2Context();
    switch (context.api_version)
    {
    case 3:
    case 2:
        return Sol2ScriptingEnvironment::GetStringListFromTable("restrictions");
    case 1:
        return Sol2ScriptingEnvironment::GetStringListFromFunction("get_restrictions");
    default:
        return {};
    }
}

void Sol2ScriptingEnvironment::ProcessTurn(ExtractionTurn &turn)
{
    auto &context = GetSol2Context();

    switch (context.api_version)
    {
    case 3:
    case 2:
        if (context.has_turn_penalty_function)
        {
            context.turn_function(context.profile_table, turn);

            // Turn weight falls back to the duration value in deciseconds
            // or uses the extracted unit-less weight value
            if (context.properties.fallback_to_duration)
                turn.weight = turn.duration;
            else
                // cap turn weight to max turn weight, which depend on weight precision
                turn.weight = std::min(turn.weight, context.properties.GetMaxTurnWeight());
        }

        break;
    case 1:
        if (context.has_turn_penalty_function)
        {
            context.turn_function(turn);

            // Turn weight falls back to the duration value in deciseconds
            // or uses the extracted unit-less weight value
            if (context.properties.fallback_to_duration)
                turn.weight = turn.duration;
        }

        break;
    case 0:
        if (context.has_turn_penalty_function)
        {
            if (turn.turn_type != guidance::TurnType::NoTurn)
            {
                // Get turn duration and convert deci-seconds to seconds
                turn.duration = static_cast<double>(context.turn_function(turn.angle)) / 10.;
                BOOST_ASSERT(turn.weight == 0);

                // add U-turn penalty
                if (turn.direction_modifier == guidance::DirectionModifier::UTurn)
                    turn.duration += context.properties.GetUturnPenalty();
            }
            else
            {
                // Use zero turn penalty if it is not an actual turn. This heuristic is necessary
                // since OSRM cannot handle looping roads/parallel roads
                turn.duration = 0.;
            }
        }

        // Add traffic light penalty, back-compatibility of api_version=0
        if (turn.has_traffic_light)
            turn.duration += context.properties.GetTrafficSignalPenalty();

        // Turn weight falls back to the duration value in deciseconds
        turn.weight = turn.duration;
        break;
    }
}

void Sol2ScriptingEnvironment::ProcessSegment(ExtractionSegment &segment)
{
    auto &context = GetSol2Context();

    if (context.has_segment_function)
    {
        switch (context.api_version)
        {
        case 3:
        case 2:
            context.segment_function(context.profile_table, segment);
            break;
        case 1:
            context.segment_function(segment);
            break;
        case 0:
            context.segment_function(
                segment.source, segment.target, segment.distance, segment.duration);
            segment.weight = segment.duration; // back-compatibility fallback to duration
            break;
        }
    }
}

void LuaScriptingContext::ProcessNode(const osmium::Node &node,
                                      ExtractionNode &result,
                                      const ExtractionRelationContainer::RelationList &relations)
{
    BOOST_ASSERT(state.lua_state() != nullptr);

    switch (api_version)
    {
    case 3:
        node_function(profile_table, node, result, relations);
        break;
    case 2:
        node_function(profile_table, node, result);
        break;
    case 1:
    case 0:
        node_function(node, result);
        break;
    }
}

void LuaScriptingContext::ProcessWay(const osmium::Way &way,
                                     ExtractionWay &result,
                                     const ExtractionRelationContainer::RelationList &relations)
{
    BOOST_ASSERT(state.lua_state() != nullptr);

    switch (api_version)
    {
    case 3:
        way_function(profile_table, way, result, relations);
        break;
    case 2:
        way_function(profile_table, way, result, location_dependent_data(state, way));
        break;
    case 1:
    case 0:
        way_function(way, result);
        break;
    }
}

void LuaScriptingContext::ProcessRelation(const osmium::Relation &relation,
                                          ExtractionRelation &result)
{
    BOOST_ASSERT(state.lua_state() != nullptr);
    BOOST_ASSERT(api_version > 2);

    relation_function(profile_table, relation, result);
}

} // namespace extractor
} // namespace osrm

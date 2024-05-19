#include "extractor/scripting_environment_lua.hpp"

#include "extractor/extraction_helper_functions.hpp"
#include "extractor/extraction_node.hpp"
#include "extractor/extraction_relation.hpp"
#include "extractor/extraction_segment.hpp"
#include "extractor/extraction_turn.hpp"
#include "extractor/extraction_way.hpp"
#include "extractor/internal_extractor_edge.hpp"
#include "extractor/maneuver_override_relation_parser.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/query_node.hpp"
#include "extractor/raster_source.hpp"
#include "extractor/restriction_parser.hpp"

#include "guidance/turn_instruction.hpp"

#include "util/coordinate.hpp"
#include "util/exception.hpp"
#include "util/log.hpp"
#include "util/lua_util.hpp"
#include "util/typedefs.hpp"

#include <osmium/osm.hpp>

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
} // namespace sol

namespace osrm::extractor
{

namespace
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

struct to_lua_object : public boost::static_visitor<sol::object>
{
    to_lua_object(sol::state &state) : state(state) {}
    template <typename T> auto operator()(T &v) const { return sol::make_object(state, v); }
    auto operator()(boost::blank &) const { return sol::lua_nil; }
    sol::state &state;
};
} // namespace

// Handle a lua error thrown in a protected function by printing the traceback and bubbling
// exception up to caller. Lua errors are generally unrecoverable, so this exception should not be
// caught but instead should terminate the process. The point of having this error handler rather
// than just using unprotected Lua functions which terminate the process automatically is that this
// function provides more useful error messages including Lua tracebacks and line numbers.
void handle_lua_error(const sol::protected_function_result &luares)
{
    sol::error luaerr = luares;
    const auto msg = luaerr.what();
    if (msg != nullptr)
    {
        std::cerr << msg << "\n";
    }
    else
    {
        std::cerr << "unknown error\n";
    }
    throw util::exception("Lua error (see stderr for traceback)");
}

Sol2ScriptingEnvironment::Sol2ScriptingEnvironment(
    const std::string &file_name,
    const std::vector<std::filesystem::path> &location_dependent_data_paths)
    : file_name(file_name), location_dependent_data(location_dependent_data_paths)
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
                           extractor::TRAVEL_MODE_INACCESSIBLE,
                           "driving",
                           extractor::TRAVEL_MODE_DRIVING,
                           "cycling",
                           extractor::TRAVEL_MODE_CYCLING,
                           "walking",
                           extractor::TRAVEL_MODE_WALKING,
                           "ferry",
                           extractor::TRAVEL_MODE_FERRY,
                           "train",
                           extractor::TRAVEL_MODE_TRAIN,
                           "pushing_bike",
                           extractor::TRAVEL_MODE_PUSHING_BIKE,
                           "steps_up",
                           extractor::TRAVEL_MODE_STEPS_UP,
                           "steps_down",
                           extractor::TRAVEL_MODE_STEPS_DOWN,
                           "river_up",
                           extractor::TRAVEL_MODE_RIVER_UP,
                           "river_down",
                           extractor::TRAVEL_MODE_RIVER_DOWN,
                           "route",
                           extractor::TRAVEL_MODE_ROUTE);

    context.state.new_enum("road_priority_class",
                           "motorway",
                           extractor::RoadPriorityClass::MOTORWAY,
                           "motorway_link",
                           extractor::RoadPriorityClass::MOTORWAY_LINK,
                           "trunk",
                           extractor::RoadPriorityClass::TRUNK,
                           "trunk_link",
                           extractor::RoadPriorityClass::TRUNK_LINK,
                           "primary",
                           extractor::RoadPriorityClass::PRIMARY,
                           "primary_link",
                           extractor::RoadPriorityClass::PRIMARY_LINK,
                           "secondary",
                           extractor::RoadPriorityClass::SECONDARY,
                           "secondary_link",
                           extractor::RoadPriorityClass::SECONDARY_LINK,
                           "tertiary",
                           extractor::RoadPriorityClass::TERTIARY,
                           "tertiary_link",
                           extractor::RoadPriorityClass::TERTIARY_LINK,
                           "main_residential",
                           extractor::RoadPriorityClass::MAIN_RESIDENTIAL,
                           "side_residential",
                           extractor::RoadPriorityClass::SIDE_RESIDENTIAL,
                           "alley",
                           extractor::RoadPriorityClass::ALLEY,
                           "parking",
                           extractor::RoadPriorityClass::PARKING,
                           "link_road",
                           extractor::RoadPriorityClass::LINK_ROAD,
                           "unclassified",
                           extractor::RoadPriorityClass::UNCLASSIFIED,
                           "bike_path",
                           extractor::RoadPriorityClass::BIKE_PATH,
                           "foot_path",
                           extractor::RoadPriorityClass::FOOT_PATH,
                           "connectivity",
                           extractor::RoadPriorityClass::CONNECTIVITY);

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

    auto get_location_tag = [](auto &context, const auto &location, const char *key)
    {
        if (context.location_dependent_data.empty())
            return sol::object(context.state);

        const LocationDependentData::point_t point{location.lon(), location.lat()};
        if (!boost::geometry::equals(context.last_location_point, point))
        {
            context.last_location_point = point;
            context.last_location_indexes =
                context.location_dependent_data.GetPropertyIndexes(point);
        }

        auto value = context.location_dependent_data.FindByKey(context.last_location_indexes, key);
        return boost::apply_visitor(to_lua_object(context.state), value);
    };

    context.state.new_usertype<osmium::Way>(
        "Way",
        "get_value_by_key",
        &get_value_by_key<osmium::Way>,
        "id",
        &osmium::Way::id,
        "version",
        &osmium::Way::version,
        "get_nodes",
        [](const osmium::Way &way) { return sol::as_table(&way.nodes()); },
        "get_location_tag",
        [&context, &get_location_tag](const osmium::Way &way, const char *key)
        {
            // HEURISTIC: use a single node (last) of the way to localize the way
            // For more complicated scenarios a proper merging of multiple tags
            // at one or many locations must be provided
            const auto &nodes = way.nodes();
            const auto &location = nodes.back().location();
            return get_location_tag(context, location, key);
        });

    context.state.new_usertype<osmium::Node>(
        "Node",
        "location",
        &osmium::Node::location,
        "get_value_by_key",
        &get_value_by_key<osmium::Node>,
        "id",
        &osmium::Node::id,
        "version",
        &osmium::Node::version,
        "get_location_tag",
        [&context, &get_location_tag](const osmium::Node &node, const char *key)
        { return get_location_tag(context, node.location(), key); });

    context.state.new_enum("traffic_lights",
                           "none",
                           extractor::TrafficLightClass::NONE,
                           "direction_all",
                           extractor::TrafficLightClass::DIRECTION_ALL,
                           "direction_forward",
                           extractor::TrafficLightClass::DIRECTION_FORWARD,
                           "direction_reverse",
                           extractor::TrafficLightClass::DIRECTION_REVERSE);

    context.state.new_usertype<ExtractionNode>(
        "ResultNode",
        "traffic_lights",
        sol::property([](const ExtractionNode &node) { return node.traffic_lights; },
                      [](ExtractionNode &node, const sol::object &obj)
                      {
                          if (obj.is<bool>())
                          {
                              // The old approach of assigning a boolean traffic light
                              // state to the node is converted to the class enum
                              // TODO: Make a breaking API change and remove this option.
                              bool val = obj.as<bool>();
                              node.traffic_lights = (val) ? TrafficLightClass::DIRECTION_ALL
                                                          : TrafficLightClass::NONE;
                              return;
                          }

                          BOOST_ASSERT(obj.is<TrafficLightClass::Direction>());
                          {
                              TrafficLightClass::Direction val =
                                  obj.as<TrafficLightClass::Direction>();
                              node.traffic_lights = val;
                          }
                      }),
        "barrier",
        &ExtractionNode::barrier);

    context.state.new_usertype<RoadClassification>(
        "RoadClassification",
        "motorway_class",
        sol::property(&RoadClassification::IsMotorwayClass, &RoadClassification::SetMotorwayFlag),
        "link_class",
        sol::property(&RoadClassification::IsLinkClass, &RoadClassification::SetLinkClass),
        "may_be_ignored",
        sol::property(&RoadClassification::IsLowPriorityRoadClass,
                      &RoadClassification::SetLowPriorityFlag),
        "road_priority_class",
        sol::property(&RoadClassification::GetClass, &RoadClassification::SetClass),
        "num_lanes",
        sol::property(&RoadClassification::GetNumberOfLanes,
                      &RoadClassification::SetNumberOfLanes));

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
        "ref", // backward compatibility
        sol::property(&ExtractionWay::GetForwardRef,
                      [](ExtractionWay &way, const char *ref)
                      {
                          way.SetForwardRef(ref);
                          way.SetBackwardRef(ref);
                      }),
        "forward_ref",
        sol::property(&ExtractionWay::GetForwardRef, &ExtractionWay::SetForwardRef),
        "backward_ref",
        sol::property(&ExtractionWay::GetBackwardRef, &ExtractionWay::SetBackwardRef),
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
                      [](ExtractionWay &way, bool flag) { way.is_left_hand_driving = flag; }),
        "highway_turn_classification",
        sol::property([](const ExtractionWay &way) { return way.highway_turn_classification; },
                      [](ExtractionWay &way, int flag) { way.highway_turn_classification = flag; }),
        "access_turn_classification",
        sol::property([](const ExtractionWay &way) { return way.access_turn_classification; },
                      [](ExtractionWay &way, int flag) { way.access_turn_classification = flag; }));

    auto getTypedRefBySol = [](const sol::object &obj) -> ExtractionRelation::OsmIDTyped
    {
        if (obj.is<osmium::Way>())
        {
            osmium::Way *way = obj.as<osmium::Way *>();
            return {way->id(), osmium::item_type::way};
        }

        if (obj.is<ExtractionRelation>())
        {
            ExtractionRelation *rel = obj.as<ExtractionRelation *>();
            return rel->id;
        }

        if (obj.is<osmium::Node>())
        {
            osmium::Node *node = obj.as<osmium::Node *>();
            return {node->id(), osmium::item_type::node};
        }

        return ExtractionRelation::OsmIDTyped(0, osmium::item_type::undefined);
    };

    context.state.new_usertype<ExtractionRelation::OsmIDTyped>(
        "OsmIDTyped",
        "id",
        &ExtractionRelation::OsmIDTyped::GetID,
        "type",
        &ExtractionRelation::OsmIDTyped::GetType);

    context.state.new_usertype<ExtractionRelation>(
        "ExtractionRelation",
        "id",
        [](ExtractionRelation &rel) { return rel.id.GetID(); },
        "get_value_by_key",
        [](ExtractionRelation &rel, const char *key) -> const char * { return rel.GetAttr(key); },
        "get_role",
        [&getTypedRefBySol](ExtractionRelation &rel, const sol::object &obj) -> const char *
        { return rel.GetRole(getTypedRefBySol(obj)); });

    context.state.new_usertype<ExtractionRelationContainer>(
        "ExtractionRelationContainer",
        "get_relations",
        [&getTypedRefBySol](ExtractionRelationContainer &cont, const sol::object &obj)
            -> const ExtractionRelationContainer::RelationIDList &
        { return cont.GetRelations(getTypedRefBySol(obj)); },
        "relation",
        [](ExtractionRelationContainer &cont,
           const ExtractionRelation::OsmIDTyped &rel_id) -> const ExtractionRelation &
        { return cont.GetRelationData(rel_id); });

    context.state.new_usertype<NodeBasedEdgeClassification>(
        "NodeBasedEdgeClassification",
        "forward",
        // can't just do &NodeBasedEdgeClassification::forward with bitfields
        sol::property([](NodeBasedEdgeClassification &c) -> bool { return c.forward; }),
        "backward",
        sol::property([](NodeBasedEdgeClassification &c) -> bool { return c.backward; }),
        "is_split",
        sol::property([](NodeBasedEdgeClassification &c) -> bool { return c.is_split; }),
        "roundabout",
        sol::property([](NodeBasedEdgeClassification &c) -> bool { return c.roundabout; }),
        "circular",
        sol::property([](NodeBasedEdgeClassification &c) -> bool { return c.circular; }),
        "startpoint",
        sol::property([](NodeBasedEdgeClassification &c) -> bool { return c.startpoint; }),
        "restricted",
        sol::property([](NodeBasedEdgeClassification &c) -> bool { return c.restricted; }),
        "road_classification",
        sol::property([](NodeBasedEdgeClassification &c) -> RoadClassification
                      { return c.road_classification; }),
        "highway_turn_classification",
        sol::property([](NodeBasedEdgeClassification &c) -> uint8_t
                      { return c.highway_turn_classification; }),
        "access_turn_classification",
        sol::property([](NodeBasedEdgeClassification &c) -> uint8_t
                      { return c.access_turn_classification; }));

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
                                                  &ExtractionSegment::duration,
                                                  "flags",
                                                  &ExtractionSegment::flags);

    // Keep in mind .location is available only if .pbf is preprocessed to set the location with the
    // ref using osmium command "osmium add-locations-to-ways"
    context.state.new_usertype<osmium::NodeRef>("NodeRef",
                                                "id",
                                                &osmium::NodeRef::ref,
                                                "location",
                                                [](const osmium::NodeRef &nref)
                                                { return nref.location(); });

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
    auto initV2Context = [&]()
    {
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
                               std::numeric_limits<TurnPenalty::value_type>::max());

        // call initialize function
        sol::protected_function setup_function = function_table.value()["setup"];
        if (!setup_function.valid())
            throw util::exception("Profile must have an setup() function.");

        auto setup_result = setup_function();

        if (!setup_result.valid())
            handle_lua_error(setup_result);

        sol::optional<sol::table> profile_table = setup_result;
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

    auto initialize_V3_extraction_turn = [&]()
    {
        context.state.new_usertype<ExtractionTurn>(
            "ExtractionTurn",
            "angle",
            &ExtractionTurn::angle,
            "turn_type",
            sol::property(
                [](const ExtractionTurn &turn)
                {
                    if (turn.number_of_roads > 2 || turn.source_mode != turn.target_mode ||
                        turn.is_u_turn)
                        return osrm::guidance::TurnType::Turn;
                    else
                        return osrm::guidance::TurnType::NoTurn;
                }),
            "direction_modifier",
            sol::property(
                [](const ExtractionTurn &turn)
                {
                    if (turn.is_u_turn)
                        return osrm::guidance::DirectionModifier::UTurn;
                    else
                        return osrm::guidance::DirectionModifier::Straight;
                }),
            "has_traffic_light",
            &ExtractionTurn::has_traffic_light,
            "weight",
            &ExtractionTurn::weight,
            "duration",
            &ExtractionTurn::duration,
            "source_restricted",
            &ExtractionTurn::source_restricted,
            "target_restricted",
            &ExtractionTurn::target_restricted,
            "is_left_hand_driving",
            &ExtractionTurn::is_left_hand_driving);

        context.state.new_enum("turn_type",
                               "invalid",
                               osrm::guidance::TurnType::Invalid,
                               "new_name",
                               osrm::guidance::TurnType::NewName,
                               "continue",
                               osrm::guidance::TurnType::Continue,
                               "turn",
                               osrm::guidance::TurnType::Turn,
                               "merge",
                               osrm::guidance::TurnType::Merge,
                               "on_ramp",
                               osrm::guidance::TurnType::OnRamp,
                               "off_ramp",
                               osrm::guidance::TurnType::OffRamp,
                               "fork",
                               osrm::guidance::TurnType::Fork,
                               "end_of_road",
                               osrm::guidance::TurnType::EndOfRoad,
                               "notification",
                               osrm::guidance::TurnType::Notification,
                               "enter_roundabout",
                               osrm::guidance::TurnType::EnterRoundabout,
                               "enter_and_exit_roundabout",
                               osrm::guidance::TurnType::EnterAndExitRoundabout,
                               "enter_rotary",
                               osrm::guidance::TurnType::EnterRotary,
                               "enter_and_exit_rotary",
                               osrm::guidance::TurnType::EnterAndExitRotary,
                               "enter_roundabout_intersection",
                               osrm::guidance::TurnType::EnterRoundaboutIntersection,
                               "enter_and_exit_roundabout_intersection",
                               osrm::guidance::TurnType::EnterAndExitRoundaboutIntersection,
                               "use_lane",
                               osrm::guidance::TurnType::Suppressed,
                               "no_turn",
                               osrm::guidance::TurnType::NoTurn,
                               "suppressed",
                               osrm::guidance::TurnType::Suppressed,
                               "enter_roundabout_at_exit",
                               osrm::guidance::TurnType::EnterRoundaboutAtExit,
                               "exit_roundabout",
                               osrm::guidance::TurnType::ExitRoundabout,
                               "enter_rotary_at_exit",
                               osrm::guidance::TurnType::EnterRotaryAtExit,
                               "exit_rotary",
                               osrm::guidance::TurnType::ExitRotary,
                               "enter_roundabout_intersection_at_exit",
                               osrm::guidance::TurnType::EnterRoundaboutIntersectionAtExit,
                               "exit_roundabout_intersection",
                               osrm::guidance::TurnType::ExitRoundaboutIntersection,
                               "stay_on_roundabout",
                               osrm::guidance::TurnType::StayOnRoundabout,
                               "sliproad",
                               osrm::guidance::TurnType::Sliproad);

        context.state.new_enum("direction_modifier",
                               "u_turn",
                               osrm::guidance::DirectionModifier::UTurn,
                               "sharp_right",
                               osrm::guidance::DirectionModifier::SharpRight,
                               "right",
                               osrm::guidance::DirectionModifier::Right,
                               "slight_right",
                               osrm::guidance::DirectionModifier::SlightRight,
                               "straight",
                               osrm::guidance::DirectionModifier::Straight,
                               "slight_left",
                               osrm::guidance::DirectionModifier::SlightLeft,
                               "left",
                               osrm::guidance::DirectionModifier::Left,
                               "sharp_left",
                               osrm::guidance::DirectionModifier::SharpLeft);
    };

    switch (context.api_version)
    {
    case 4:
    {
        context.state.new_usertype<ExtractionTurnLeg>(
            "ExtractionTurnLeg",
            "is_restricted",
            &ExtractionTurnLeg::is_restricted,
            "is_motorway",
            &ExtractionTurnLeg::is_motorway,
            "is_link",
            &ExtractionTurnLeg::is_link,
            "number_of_lanes",
            &ExtractionTurnLeg::number_of_lanes,
            "highway_turn_classification",
            &ExtractionTurnLeg::highway_turn_classification,
            "access_turn_classification",
            &ExtractionTurnLeg::access_turn_classification,
            "speed",
            &ExtractionTurnLeg::speed,
            "priority_class",
            &ExtractionTurnLeg::priority_class,
            "is_incoming",
            &ExtractionTurnLeg::is_incoming,
            "is_outgoing",
            &ExtractionTurnLeg::is_outgoing);

        context.state.new_usertype<ExtractionTurn>(
            "ExtractionTurn",
            "angle",
            &ExtractionTurn::angle,
            "number_of_roads",
            &ExtractionTurn::number_of_roads,
            "is_u_turn",
            &ExtractionTurn::is_u_turn,
            "has_traffic_light",
            &ExtractionTurn::has_traffic_light,
            "is_left_hand_driving",
            &ExtractionTurn::is_left_hand_driving,

            "source_restricted",
            &ExtractionTurn::source_restricted,
            "source_mode",
            &ExtractionTurn::source_mode,
            "source_is_motorway",
            &ExtractionTurn::source_is_motorway,
            "source_is_link",
            &ExtractionTurn::source_is_link,
            "source_number_of_lanes",
            &ExtractionTurn::source_number_of_lanes,
            "source_highway_turn_classification",
            &ExtractionTurn::source_highway_turn_classification,
            "source_access_turn_classification",
            &ExtractionTurn::source_access_turn_classification,
            "source_speed",
            &ExtractionTurn::source_speed,
            "source_priority_class",
            &ExtractionTurn::source_priority_class,

            "target_restricted",
            &ExtractionTurn::target_restricted,
            "target_mode",
            &ExtractionTurn::target_mode,
            "target_is_motorway",
            &ExtractionTurn::target_is_motorway,
            "target_is_link",
            &ExtractionTurn::target_is_link,
            "target_number_of_lanes",
            &ExtractionTurn::target_number_of_lanes,
            "target_highway_turn_classification",
            &ExtractionTurn::target_highway_turn_classification,
            "target_access_turn_classification",
            &ExtractionTurn::target_access_turn_classification,
            "target_speed",
            &ExtractionTurn::target_speed,
            "target_priority_class",
            &ExtractionTurn::target_priority_class,

            "roads_on_the_right",
            &ExtractionTurn::roads_on_the_right,
            "roads_on_the_left",
            &ExtractionTurn::roads_on_the_left,
            "weight",
            &ExtractionTurn::weight,
            "duration",
            &ExtractionTurn::duration);
        initV2Context();
        break;
    }
    case 3:
    case 2:
    {
        initialize_V3_extraction_turn();
        initV2Context();
        break;
    }
    case 1:
    {
        initialize_V3_extraction_turn();

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
        initialize_V3_extraction_turn();

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
    const ManeuverOverrideRelationParser &maneuver_override_parser,
    const ExtractionRelationContainer &relations,
    std::vector<std::pair<const osmium::Node &, ExtractionNode>> &resulting_nodes,
    std::vector<std::pair<const osmium::Way &, ExtractionWay>> &resulting_ways,
    std::vector<InputTurnRestriction> &resulting_restrictions,
    std::vector<InputManeuverOverride> &resulting_maneuver_overrides)
{
    ExtractionNode result_node;
    ExtractionWay result_way;
    auto &local_context = this->GetSol2Context();

    for (auto entity = buffer.cbegin(), end = buffer.cend(); entity != end; ++entity)
    {
        switch (entity->type())
        {
        case osmium::item_type::node:
        {
            const auto &node = static_cast<const osmium::Node &>(*entity);
            // NOLINTNEXTLINE(bugprone-use-after-move)
            result_node.clear();
            if (local_context.has_node_function &&
                (!node.tags().empty() || local_context.properties.call_tagless_node_function))
            {
                local_context.ProcessNode(node, result_node, relations);
            }
            resulting_nodes.push_back({node, result_node});
        }
        break;
        case osmium::item_type::way:
        {
            const osmium::Way &way = static_cast<const osmium::Way &>(*entity);
            // NOLINTNEXTLINE(bugprone-use-after-move)
            result_way.clear();
            if (local_context.has_way_function)
            {
                local_context.ProcessWay(way, result_way, relations);
            }
            resulting_ways.push_back({way, std::move(result_way)});
        }
        break;
        case osmium::item_type::relation:
        {
            const auto &relation = static_cast<const osmium::Relation &>(*entity);
            auto results = restriction_parser.TryParse(relation);
            if (!results.empty())
            {
                std::move(
                    results.begin(), results.end(), std::back_inserter(resulting_restrictions));
            }
            else if (auto result_res = maneuver_override_parser.TryParse(relation))
            {
                resulting_maneuver_overrides.push_back(std::move(*result_res));
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
    BOOST_ASSERT(context.state.lua_state());
    std::vector<std::string> strings;
    sol::function function = context.state[function_name];
    if (function.valid())
    {
        function(strings);
    }
    return strings;
}

namespace
{

// string list can be defined either as a Set(see profiles/lua/set.lua) or as a Sequence (see
// profiles/lua/sequence.lua) `Set` is a table with keys that are actual values we are looking for
// and values that always `true`. `Sequence` is a table with keys that are indices and values that
// are actual values we are looking for.

std::string GetSetOrSequenceValue(const std::pair<sol::object, sol::object> &pair)
{
    if (pair.second.is<std::string>())
    {
        return pair.second.as<std::string>();
    }
    BOOST_ASSERT(pair.first.is<std::string>());
    return pair.first.as<std::string>();
}

} // namespace

std::vector<std::string>
Sol2ScriptingEnvironment::GetStringListFromTable(const std::string &table_name)
{
    auto &context = GetSol2Context();
    BOOST_ASSERT(context.state.lua_state() != nullptr);
    std::vector<std::string> strings;
    sol::optional<sol::table> table = context.profile_table[table_name];
    if (table && table->valid())
    {
        for (auto &&pair : *table)
        {
            strings.emplace_back(GetSetOrSequenceValue(pair));
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
    sol::optional<sol::table> table = context.profile_table[table_name];
    if (!table || !table->valid())
    {
        return string_lists;
    }

    for (const auto &pair : *table)
    {
        const sol::table &inner_table = pair.second;
        if (!inner_table.valid())
        {
            throw util::exception("Expected a sub-table at " + table_name + "[" +
                                  pair.first.as<std::string>() + "]");
        }

        std::vector<std::string> inner_vector;
        for (const auto &inner_pair : inner_table)
        {
            inner_vector.emplace_back(GetSetOrSequenceValue(inner_pair));
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
    case 4:
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
    case 4:
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
    case 4:
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
    case 4:
    case 3:
    case 2:
        return Sol2ScriptingEnvironment::GetStringListFromTable("restrictions");
    case 1:
        return Sol2ScriptingEnvironment::GetStringListFromFunction("get_restrictions");
    default:
        return {};
    }
}

std::vector<std::string> Sol2ScriptingEnvironment::GetRelations()
{
    auto &context = GetSol2Context();
    switch (context.api_version)
    {
    case 4:
    case 3:
        return Sol2ScriptingEnvironment::GetStringListFromTable("relation_types");
    default:
        return {};
    }
}

void Sol2ScriptingEnvironment::ProcessTurn(ExtractionTurn &turn)
{
    auto &context = GetSol2Context();

    switch (context.api_version)
    {
    case 4:
    case 3:
    case 2:
        if (context.has_turn_penalty_function)
        {
            auto luares = context.turn_function(context.profile_table, std::ref(turn));
            if (!luares.valid())
                handle_lua_error(luares);

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
            auto luares = context.turn_function(std::ref(turn));
            if (!luares.valid())
                handle_lua_error(luares);

            // Turn weight falls back to the duration value in deciseconds
            // or uses the extracted unit-less weight value
            if (context.properties.fallback_to_duration)
                turn.weight = turn.duration;
        }

        break;
    case 0:
        if (context.has_turn_penalty_function)
        {
            if (turn.number_of_roads > 2)
            {
                // Get turn duration and convert deci-seconds to seconds
                turn.duration = static_cast<double>(context.turn_function(turn.angle)) / 10.;
                BOOST_ASSERT(turn.weight == 0);

                // add U-turn penalty
                if (turn.is_u_turn)
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
        sol::protected_function_result luares;
        switch (context.api_version)
        {
        case 4:
        case 3:
        case 2:
            luares = context.segment_function(context.profile_table, std::ref(segment));
            break;
        case 1:
            luares = context.segment_function(std::ref(segment));
            break;
        case 0:
            luares = context.segment_function(std::ref(segment.source),
                                              std::ref(segment.target),
                                              segment.distance,
                                              segment.duration);
            segment.weight = segment.duration; // back-compatibility fallback to duration
            break;
        }

        if (!luares.valid())
            handle_lua_error(luares);
    }
}

void LuaScriptingContext::ProcessNode(const osmium::Node &node,
                                      ExtractionNode &result,
                                      const ExtractionRelationContainer &relations)
{
    BOOST_ASSERT(state.lua_state() != nullptr);

    sol::protected_function_result luares;

    // TODO check for api version, make sure luares is always set
    switch (api_version)
    {
    case 4:
    case 3:
        luares =
            node_function(profile_table, std::cref(node), std::ref(result), std::cref(relations));
        break;
    case 2:
        luares = node_function(profile_table, std::cref(node), std::ref(result));
        break;
    case 1:
    case 0:
        luares = node_function(std::cref(node), std::ref(result));
        break;
    }

    if (!luares.valid())
        handle_lua_error(luares);
}

void LuaScriptingContext::ProcessWay(const osmium::Way &way,
                                     ExtractionWay &result,
                                     const ExtractionRelationContainer &relations)
{
    BOOST_ASSERT(state.lua_state() != nullptr);

    sol::protected_function_result luares;

    // TODO check for api version, make sure luares is always set
    switch (api_version)
    {
    case 4:
    case 3:
        luares =
            way_function(profile_table, std::cref(way), std::ref(result), std::cref(relations));
        break;
    case 2:
        luares = way_function(profile_table, std::cref(way), std::ref(result));
        break;
    case 1:
    case 0:
        luares = way_function(std::cref(way), std::ref(result));
        break;
    }

    if (!luares.valid())
        handle_lua_error(luares);
}

} // namespace osrm::extractor

#ifndef ENGINE_API_ROUTE_HPP
#define ENGINE_API_ROUTE_HPP

#include "extractor/maneuver_override.hpp"
#include "engine/api/base_api.hpp"
#include "engine/api/json_factory.hpp"
#include "engine/api/route_parameters.hpp"

#include "engine/datafacade/datafacade_base.hpp"

#include "engine/guidance/assemble_geometry.hpp"
#include "engine/guidance/assemble_leg.hpp"
#include "engine/guidance/assemble_overview.hpp"
#include "engine/guidance/assemble_route.hpp"
#include "engine/guidance/assemble_steps.hpp"
#include "engine/guidance/collapse_turns.hpp"
#include "engine/guidance/lane_processing.hpp"
#include "engine/guidance/post_processing.hpp"
#include "engine/guidance/verbosity_reduction.hpp"

#include "engine/internal_route_result.hpp"

#include "guidance/turn_instruction.hpp"

#include "util/coordinate.hpp"
#include "util/integer_range.hpp"
#include "util/json_util.hpp"

#include <iterator>
#include <vector>

namespace osrm
{
namespace engine
{
namespace api
{

class RouteAPI : public BaseAPI {
public:
    RouteAPI(const datafacade::BaseDataFacade &facade_, const RouteParameters &parameters_)
            : BaseAPI(facade_, parameters_), parameters(parameters_) {
    }

    virtual void
    MakeResponse(const InternalManyRoutesResult &raw_routes,
                 const std::vector<PhantomNodes>
                 &all_start_end_points, // all used coordinates, ignoring waypoints= parameter
                 osrm::engine::api::ResultT &response) const {
        BOOST_ASSERT(!raw_routes.routes.empty());

        if (response.is<flatbuffers::FlatBufferBuilder>()) {
            auto &fb_result = response.get<flatbuffers::FlatBufferBuilder>();
            MakeResponse(raw_routes, all_start_end_points, fb_result);
        } else {
            auto &json_result = response.get<util::json::Object>();
            MakeResponse(raw_routes, all_start_end_points, json_result);
        }
    }

    void
    MakeResponse(const InternalManyRoutesResult &raw_routes,
                 const std::vector<PhantomNodes>
                 &all_start_end_points, // all used coordinates, ignoring waypoints= parameter
                 flatbuffers::FlatBufferBuilder &fb_result) const
    {

        fbresult::FBResultBuilder response(fb_result);
        response.add_code(fb_result.CreateString("Ok"));
        response.add_response_type(osrm::engine::api::fbresult::ServiceResponse::ServiceResponse_route);

        fbresult::RouteBuilder route(fb_result);
        std::vector<flatbuffers::Offset<fbresult::RouteObject>> routes;
        for (const auto &raw_route : raw_routes.routes)
        {
            if (!raw_route.is_valid())
                continue;

            routes.push_back(MakeRoute(fb_result,
                                       raw_route.segment_end_coordinates,
                                       raw_route.unpacked_path_segments,
                                       raw_route.source_traversed_in_reverse,
                                       raw_route.target_traversed_in_reverse));
        }

        auto routes_vector = fb_result.CreateVector(routes);
        route.add_routes(routes_vector);
        route.add_waypoints(BaseAPI::MakeWaypoints(fb_result, all_start_end_points));
        response.add_response(route.Finish().Union());
        
        auto data_timestamp = facade.GetTimestamp();
        if (!data_timestamp.empty())
        {
            auto data_version_string = fb_result.CreateString(data_timestamp);
            response.add_data_version(data_version_string);
        }

        fb_result.Finish(response.Finish());
    }
    void
    MakeResponse(const InternalManyRoutesResult &raw_routes,
                 const std::vector<PhantomNodes>
                     &all_start_end_points, // all used coordinates, ignoring waypoints= parameter
                 util::json::Object &response) const
    {
        util::json::Array jsRoutes;

        for (const auto &route : raw_routes.routes)
        {
            if (!route.is_valid())
                continue;

            jsRoutes.values.push_back(MakeRoute(route.segment_end_coordinates,
                                                route.unpacked_path_segments,
                                                route.source_traversed_in_reverse,
                                                route.target_traversed_in_reverse));
        }

        response.values["waypoints"] = BaseAPI::MakeWaypoints(all_start_end_points);
        response.values["routes"] = std::move(jsRoutes);
        response.values["code"] = "Ok";
        auto data_timestamp = facade.GetTimestamp();
        if (!data_timestamp.empty())
        {
            response.values["data_version"] = data_timestamp;
        }
    }

  protected:
    template <typename ForwardIter>
    flatbuffers::Offset<fbresult::Geometry> MakeGeometry(flatbuffers::FlatBufferBuilder& builder, ForwardIter begin, ForwardIter end) const
    {
        fbresult::GeometryBuilder geometry(builder);
        if (parameters.geometries == RouteParameters::GeometriesType::Polyline) {
            auto polyline_string = builder.CreateString(encodePolyline<100000>(begin, end));
            geometry.add_polyline(polyline_string);
        } else if (parameters.geometries == RouteParameters::GeometriesType::Polyline6) {
            auto polyline_string = builder.CreateString(encodePolyline<1000000>(begin, end));
            geometry.add_polyline6(polyline_string);
        } else {
            std::vector<fbresult::Position> coordinates;
            coordinates.resize(std::distance(begin, end));
            std::transform(begin, end, coordinates.begin(), [](const Coordinate &c) {
                return fbresult::Position{static_cast<double>(util::toFloating(c.lon)),
                                     static_cast<double>(util::toFloating(c.lat))};
            });
            auto coordinates_vector = builder.CreateVectorOfStructs(coordinates);
            geometry.add_coordinates(coordinates_vector);
        }
        return geometry.Finish();
    }

    boost::optional<util::json::Value> MakeGeometry(boost::optional<std::vector<Coordinate>>&& annotations) const
    {
        boost::optional<util::json::Value> json_geometry;
        if (annotations) {
            auto begin = annotations->begin();
            auto end = annotations->end();
            if (parameters.geometries == RouteParameters::GeometriesType::Polyline)
            {
                json_geometry = json::makePolyline<100000>(begin, end);
            } else if (parameters.geometries == RouteParameters::GeometriesType::Polyline6)
            {
                json_geometry = json::makePolyline<1000000>(begin, end);
            } else {
                BOOST_ASSERT(parameters.geometries == RouteParameters::GeometriesType::GeoJSON);
                json_geometry = json::makeGeoJSONGeometry(begin, end);
            }
        }
        return json_geometry;
    }

    template <typename ValueType, typename GetFn>
    flatbuffers::Offset<flatbuffers::Vector<ValueType>> GetAnnotations(flatbuffers::FlatBufferBuilder& fb_result, guidance::LegGeometry &leg, GetFn Get) const
    {
        std::vector<ValueType> annotations_store;
        annotations_store.reserve(leg.annotations.size());

        for (const auto &step : leg.annotations)
        {
            annotations_store.push_back(Get(step));
        }

        return fb_result.CreateVector(annotations_store);
    }

    template <typename GetFn>
    util::json::Array GetAnnotations(const guidance::LegGeometry &leg, GetFn Get) const
    {
        util::json::Array annotations_store;
        annotations_store.values.reserve(leg.annotations.size());

        for (const auto &step : leg.annotations)
        {
            annotations_store.values.push_back(Get(step));
        }

        return annotations_store;
    }

    fbresult::ManeuverType WaypointTypeToFB(guidance::WaypointType type) const {
        switch(type) {
            case guidance::WaypointType::Arrive:
                return fbresult::ManeuverType_Arrive;
            case guidance::WaypointType::Depart:
                return fbresult::ManeuverType_Depart;
            default:
                return fbresult::ManeuverType_Notification;
        }
    }

    fbresult::ManeuverType TurnTypeToFB(osrm::guidance::TurnType::Enum turn) const {
        static std::map<osrm::guidance::TurnType::Enum, fbresult::ManeuverType> mappings={
                {osrm::guidance::TurnType::Invalid, fbresult::ManeuverType_Notification},
                {osrm::guidance::TurnType::NewName, fbresult::ManeuverType_NewName},
                {osrm::guidance::TurnType::Continue, fbresult::ManeuverType_Continue},
                {osrm::guidance::TurnType::Turn, fbresult::ManeuverType_Turn},
                {osrm::guidance::TurnType::Merge, fbresult::ManeuverType_Merge},
                {osrm::guidance::TurnType::OnRamp, fbresult::ManeuverType_OnRamp},
                {osrm::guidance::TurnType::OffRamp, fbresult::ManeuverType_OffRamp},
                {osrm::guidance::TurnType::Fork, fbresult::ManeuverType_Fork},
                {osrm::guidance::TurnType::EndOfRoad, fbresult::ManeuverType_EndOfRoad},
                {osrm::guidance::TurnType::Notification, fbresult::ManeuverType_Notification},
                {osrm::guidance::TurnType::EnterRoundabout, fbresult::ManeuverType_Roundabout},
                {osrm::guidance::TurnType::EnterAndExitRoundabout, fbresult::ManeuverType_ExitRoundabout},
                {osrm::guidance::TurnType::EnterRotary, fbresult::ManeuverType_Rotary},
                {osrm::guidance::TurnType::EnterAndExitRotary, fbresult::ManeuverType_ExitRotary},
                {osrm::guidance::TurnType::EnterRoundaboutIntersection, fbresult::ManeuverType_Roundabout},
                {osrm::guidance::TurnType::EnterAndExitRoundaboutIntersection, fbresult::ManeuverType_ExitRoundabout},
                {osrm::guidance::TurnType::NoTurn, fbresult::ManeuverType_Notification},
                {osrm::guidance::TurnType::Suppressed, fbresult::ManeuverType_Notification},
                {osrm::guidance::TurnType::EnterRoundaboutAtExit, fbresult::ManeuverType_Roundabout},
                {osrm::guidance::TurnType::ExitRoundabout, fbresult::ManeuverType_ExitRoundabout},
                {osrm::guidance::TurnType::EnterRotaryAtExit, fbresult::ManeuverType_Rotary},
                {osrm::guidance::TurnType::ExitRotary, fbresult::ManeuverType_ExitRotary},
                {osrm::guidance::TurnType::EnterRoundaboutIntersectionAtExit, fbresult::ManeuverType_Roundabout},
                {osrm::guidance::TurnType::ExitRoundaboutIntersection, fbresult::ManeuverType_ExitRoundabout},
                {osrm::guidance::TurnType::StayOnRoundabout, fbresult::ManeuverType_RoundaboutTurn},
                {osrm::guidance::TurnType::Sliproad, fbresult::ManeuverType_Notification},
                {osrm::guidance::TurnType::MaxTurnType, fbresult::ManeuverType_Notification}
        };
        return mappings[turn];
    }

    fbresult::Turn TurnModifierToFB(osrm::guidance::DirectionModifier::Enum modifier) const {
        static std::map<osrm::guidance::DirectionModifier::Enum, fbresult::Turn> mappings={
                {osrm::guidance::DirectionModifier::UTurn, fbresult::Turn_UTurn},
                {osrm::guidance::DirectionModifier::SharpRight, fbresult::Turn_SharpRight},
                {osrm::guidance::DirectionModifier::Right, fbresult::Turn_Right},
                {osrm::guidance::DirectionModifier::SlightRight, fbresult::Turn_SlightRight},
                {osrm::guidance::DirectionModifier::Straight, fbresult::Turn_Straight},
                {osrm::guidance::DirectionModifier::SlightLeft, fbresult::Turn_SlightLeft},
                {osrm::guidance::DirectionModifier::Left, fbresult::Turn_Left},
                {osrm::guidance::DirectionModifier::SharpLeft, fbresult::Turn_SharpLeft},
        };
        return mappings[modifier];
    }

    std::vector<int8_t> TurnLaneTypeToFB(const extractor::TurnLaneType::Mask lane_type) const {
        const static fbresult::Turn mapping[] = {fbresult::Turn_None,
                                              fbresult::Turn_Straight,
                                              fbresult::Turn_SharpLeft,
                                              fbresult::Turn_Left,
                                              fbresult::Turn_SlightLeft,
                                              fbresult::Turn_SlightRight,
                                              fbresult::Turn_Right,
                                              fbresult::Turn_SharpRight,
                                              fbresult::Turn_UTurn,
                                              fbresult::Turn_SlightLeft,
                                              fbresult::Turn_SlightRight};
        std::vector<int8_t> result;
        std::bitset<8 * sizeof(extractor::TurnLaneType::Mask)> mask(lane_type);
        for (auto index : util::irange<std::size_t>(0, extractor::TurnLaneType::NUM_TYPES))
        {
            if (mask[index])
            {
                result.push_back(mapping[index]);
            }
        }
        return result;

    }
    flatbuffers::Offset<fbresult::RouteObject>
            MakeRoute(flatbuffers::FlatBufferBuilder &fb_result,
                      const std::vector<PhantomNodes> &segment_end_coordinates,
                      const std::vector<std::vector<PathData>> &unpacked_path_segments,
                      const std::vector<bool> &source_traversed_in_reverse,
                      const std::vector<bool> &target_traversed_in_reverse) const
    {
        fbresult::RouteObjectBuilder routeObject(fb_result);

        auto legs_info = MakeLegs(segment_end_coordinates, unpacked_path_segments, source_traversed_in_reverse, target_traversed_in_reverse);
        std::vector<guidance::RouteLeg> legs = legs_info.first;
        std::vector<guidance::LegGeometry> leg_geometries = legs_info.second;

        //Fill basix route info
        auto route = guidance::assembleRoute(legs);
        auto weight_name_string = fb_result.CreateString(facade.GetWeightName());
        routeObject.add_weight_name(weight_name_string);
        routeObject.add_distance(route.distance);
        routeObject.add_duration(route.duration);
        routeObject.add_weight(route.weight);

        //Fill geometry
        auto overview = MakeOverview(leg_geometries);
        if(overview) {
            auto geometry = MakeGeometry(fb_result, overview->begin(), overview->end());
            routeObject.add_geometry(geometry);
        }

        //Fill legs
        std::vector<flatbuffers::Offset<fbresult::Leg>> routeLegs;
        routeLegs.reserve(legs.size());
        for (const auto idx : util::irange<std::size_t>(0UL, legs.size())) {
            auto leg = legs[idx];
            auto &leg_geometry = leg_geometries[idx];
            fbresult::LegBuilder legBuilder(fb_result);
            legBuilder.add_distance(leg.distance);
            legBuilder.add_duration(leg.duration);
            legBuilder.add_weight(leg.weight);
            if (!leg.summary.empty()) {
                auto summary_string = fb_result.CreateString(leg.summary);
                legBuilder.add_summary(summary_string);
            }

            //Fill steps
            if (!leg.steps.empty()) {
                std::vector<flatbuffers::Offset<fbresult::Step>> legSteps;
                legSteps.resize(leg.steps.size());
                std::transform(leg.steps.begin(), leg.steps.end(), legSteps.begin(), [this, &leg_geometry, &fb_result](const guidance::RouteStep& step) {
                    fbresult::StepBuilder stepBuilder(fb_result);
                    stepBuilder.add_duration(step.duration);
                    stepBuilder.add_distance(step.distance);
                    stepBuilder.add_weight(step.weight);
                    auto name_string = fb_result.CreateString(step.name);
                    stepBuilder.add_name(name_string);
                    if (!step.ref.empty()) {
                        auto ref_string = fb_result.CreateString(step.ref);
                        stepBuilder.add_ref(ref_string);
                    }
                    if (!step.pronunciation.empty()) {
                        auto pronunciation_string = fb_result.CreateString(step.pronunciation);
                        stepBuilder.add_pronunciation(pronunciation_string);
                    }
                    if (!step.destinations.empty()) {
                        auto destinations_string = fb_result.CreateString(step.destinations);
                        stepBuilder.add_destinations(destinations_string);
                    }
                    if (!step.exits.empty()) {
                        auto exists_string = fb_result.CreateString(step.exits);
                        stepBuilder.add_exits(exists_string);
                    }
                    if(!step.rotary_name.empty()) {
                        auto rotary_name_string = fb_result.CreateString(step.rotary_name);
                        stepBuilder.add_rotary_name(rotary_name_string);
                        if (!step.rotary_pronunciation.empty()) {
                            auto rotary_pronunciation_string = fb_result.CreateString(step.rotary_pronunciation);
                            stepBuilder.add_rotary_pronunciation(rotary_pronunciation_string);
                        }
                    }
                    auto mode_string = fb_result.CreateString(extractor::travelModeToString(step.mode));
                    stepBuilder.add_mode(mode_string);
                    stepBuilder.add_driving_side(step.is_left_hand_driving);

                    //Geometry
                    auto geometry = MakeGeometry(fb_result, leg_geometry.locations.begin() + step.geometry_begin, leg_geometry.locations.begin() + step.geometry_end);
                    stepBuilder.add_geometry(geometry);
                    //Maneuver
                    fbresult::StepManeuverBuilder maneuver(fb_result);
                    fbresult::Position maneuverPosition{static_cast<double>(util::toFloating(step.maneuver.location.lon)),
                                                        static_cast<double>(util::toFloating(step.maneuver.location.lat))};
                    maneuver.add_location(&maneuverPosition);
                    maneuver.add_bearing_before(step.maneuver.bearing_before);
                    maneuver.add_bearing_after(step.maneuver.bearing_after);
                    if (step.maneuver.waypoint_type == guidance::WaypointType::None)
                        maneuver.add_type(TurnTypeToFB(step.maneuver.instruction.type));
                    else
                        maneuver.add_type(WaypointTypeToFB(step.maneuver.waypoint_type));
                    if (osrm::engine::api::json::detail::isValidModifier(step.maneuver)) {
                        maneuver.add_modifier(TurnModifierToFB(step.maneuver.instruction.direction_modifier));
                    }
                    if (step.maneuver.exit != 0) {
                        maneuver.add_exit(step.maneuver.exit);
                    }

                    //intersections
                    std::vector<flatbuffers::Offset<fbresult::Intersection>> intersections;
                    intersections.resize(step.intersections.size());
                    std::transform(step.intersections.begin(), step.intersections.end(), intersections.begin(), [&fb_result, this](const guidance::IntermediateIntersection& intersection) {
                        fbresult::IntersectionBuilder intersectionBuilder(fb_result);
                        fbresult::Position maneuverPosition{static_cast<double>(util::toFloating(intersection.location.lon)),
                                                            static_cast<double>(util::toFloating(intersection.location.lat))};
                        intersectionBuilder.add_location(&maneuverPosition);
                        auto bearings_vector = fb_result.CreateVector(intersection.bearings);
                        intersectionBuilder.add_bearings(bearings_vector);
                        std::vector<flatbuffers::Offset<flatbuffers::String>> classes;
                        classes.resize(intersection.classes.size());
                        std::transform(intersection.classes.begin(), intersection.classes.end(), classes.begin(), [&fb_result](const std::string cls) {
                            return fb_result.CreateString(cls);
                        });
                        auto classes_vector = fb_result.CreateVector(classes);
                        intersectionBuilder.add_classes(classes_vector);
                        auto entry_vector = fb_result.CreateVector(intersection.entry);
                        intersectionBuilder.add_entry(entry_vector);
                        intersectionBuilder.add_in(intersection.in);
                        intersectionBuilder.add_out(intersection.out);
                        if (api::json::detail::hasValidLanes(intersection)) {
                            BOOST_ASSERT(intersection.lanes.lanes_in_turn >= 1);
                            std::vector<flatbuffers::Offset<fbresult::Lane>> lanes;
                            lanes.resize(intersection.lane_description.size());
                            LaneID lane_id = intersection.lane_description.size();

                            for (const auto &lane_desc : intersection.lane_description)
                            {
                                --lane_id;
                                fbresult::LaneBuilder laneBuilder(fb_result);
                                auto indications_vector = fb_result.CreateVector(TurnLaneTypeToFB(lane_desc));
                                laneBuilder.add_indications(indications_vector);
                                if (lane_id >= intersection.lanes.first_lane_from_the_right &&
                                    lane_id <
                                    intersection.lanes.first_lane_from_the_right + intersection.lanes.lanes_in_turn)
                                    laneBuilder.add_valid(true);
                                else
                                    laneBuilder.add_valid(false);

                                lanes.emplace_back(laneBuilder.Finish());
                            }
                            auto lanes_vector = fb_result.CreateVector(lanes);
                            intersectionBuilder.add_lanes(lanes_vector);
                        }

                        return intersectionBuilder.Finish();
                    });
                    auto intersections_vector = fb_result.CreateVector(intersections);
                    stepBuilder.add_intersections(intersections_vector);
                    return stepBuilder.Finish();
                });
                auto steps_vector = fb_result.CreateVector(legSteps);
                legBuilder.add_steps(steps_vector);
            }

            //Fill annotations
            // To maintain support for uses of the old default constructors, we check
            // if annotations property was set manually after default construction
            auto requested_annotations = parameters.annotations_type;
            if ((parameters.annotations == true) &&
                (parameters.annotations_type == RouteParameters::AnnotationsType::None))
            {
                requested_annotations = RouteParameters::AnnotationsType::All;
            }

            if (requested_annotations != RouteParameters::AnnotationsType::None)
            {
                fbresult::AnnotationBuilder annotation(fb_result);

                // AnnotationsType uses bit flags, & operator checks if a property is set
                if (parameters.annotations_type & RouteParameters::AnnotationsType::Speed)
                {
                    double prev_speed = 0;
                    auto speed = GetAnnotations<double>(
                            fb_result, leg_geometry, [&prev_speed](const guidance::LegGeometry::Annotation &anno) {
                                if (anno.duration < std::numeric_limits<double>::min())
                                {
                                    return prev_speed;
                                }
                                else
                                {
                                    auto speed = std::round(anno.distance / anno.duration * 10.) / 10.;
                                    prev_speed = speed;
                                    return util::json::clamp_float(speed);
                                }
                            });
                    annotation.add_speed(speed);
                }

                if (requested_annotations & RouteParameters::AnnotationsType::Duration)
                {
                    auto duration = GetAnnotations<uint32_t>(
                            fb_result, leg_geometry, [](const guidance::LegGeometry::Annotation &anno) {
                                return anno.duration;
                            });
                    annotation.add_duration(duration);
                }
                if (requested_annotations & RouteParameters::AnnotationsType::Distance)
                {
                    auto distance = GetAnnotations<uint32_t>(
                            fb_result, leg_geometry, [](const guidance::LegGeometry::Annotation &anno) {
                                return anno.distance;
                            });
                    annotation.add_distance(distance);
                }
                if (requested_annotations & RouteParameters::AnnotationsType::Weight)
                {
                    auto weight = GetAnnotations<uint32_t>(
                            fb_result, leg_geometry,
                            [](const guidance::LegGeometry::Annotation &anno) { return anno.weight; });
                    annotation.add_weight(weight);
                }
                if (requested_annotations & RouteParameters::AnnotationsType::Datasources)
                {
                    auto datasources = GetAnnotations<uint32_t>(
                            fb_result, leg_geometry, [](const guidance::LegGeometry::Annotation &anno) {
                                return anno.datasource;
                            });
                    annotation.add_datasources(datasources);
                }
                if (requested_annotations & RouteParameters::AnnotationsType::Nodes)
                {
                    std::vector<uint32_t> nodes;
                    nodes.reserve(leg_geometry.osm_node_ids.size());
                    for (const auto node_id : leg_geometry.osm_node_ids)
                    {
                        nodes.emplace_back(static_cast<std::uint64_t>(node_id));
                    }
                    auto nodes_vector = fb_result.CreateVector(nodes);
                    annotation.add_nodes(nodes_vector);
                }
                // Add any supporting metadata, if needed
                if (requested_annotations & RouteParameters::AnnotationsType::Datasources)
                {
                    const auto MAX_DATASOURCE_ID = 255u;
                    fbresult::MetadataBuilder metadata(fb_result);
                    std::vector<flatbuffers::Offset<flatbuffers::String>> names;
                    for (auto i = 0u; i < MAX_DATASOURCE_ID; i++)
                    {
                        const auto name = facade.GetDatasourceName(i);
                        // Length of 0 indicates the first empty name, so we can stop here
                        if (name.size() == 0)
                            break;
                        names.emplace_back(fb_result.CreateString(std::string(facade.GetDatasourceName(i))));
                    }
                    auto datasource_names_vector = fb_result.CreateVector(names);
                    metadata.add_datasource_names(datasource_names_vector);
                    annotation.add_metadata(metadata.Finish());
                }

                legBuilder.add_annotations(annotation.Finish());
            }

            routeLegs.emplace_back(legBuilder.Finish());
        }
        auto legs_vector = fb_result.CreateVector(routeLegs);
        routeObject.add_legs(legs_vector);

        return routeObject.Finish();
    }

    util::json::Object MakeRoute(const std::vector<PhantomNodes> &segment_end_coordinates,
                                 const std::vector<std::vector<PathData>> &unpacked_path_segments,
                                 const std::vector<bool> &source_traversed_in_reverse,
                                 const std::vector<bool> &target_traversed_in_reverse) const
    {
        auto legs_info = MakeLegs(segment_end_coordinates, unpacked_path_segments, source_traversed_in_reverse, target_traversed_in_reverse);
        std::vector<guidance::RouteLeg> legs = legs_info.first;
        std::vector<guidance::LegGeometry> leg_geometries = legs_info.second;

        auto route = guidance::assembleRoute(legs);
        boost::optional<util::json::Value> json_overview = MakeGeometry(MakeOverview(leg_geometries));

        std::vector<util::json::Value> step_geometries;
        const auto total_step_count =
            std::accumulate(legs.begin(), legs.end(), 0, [](const auto &v, const auto &leg) {
                return v + leg.steps.size();
            });
        step_geometries.reserve(total_step_count);

        for (const auto idx : util::irange<std::size_t>(0UL, legs.size()))
        {
            auto &leg_geometry = leg_geometries[idx];

            std::transform(
                legs[idx].steps.begin(),
                legs[idx].steps.end(),
                std::back_inserter(step_geometries),
                [this, &leg_geometry](const guidance::RouteStep &step) {
                    if (parameters.geometries == RouteParameters::GeometriesType::Polyline)
                    {
                        return static_cast<util::json::Value>(json::makePolyline<100000>(
                            leg_geometry.locations.begin() + step.geometry_begin,
                            leg_geometry.locations.begin() + step.geometry_end));
                    }

                    if (parameters.geometries == RouteParameters::GeometriesType::Polyline6)
                    {
                        return static_cast<util::json::Value>(json::makePolyline<1000000>(
                            leg_geometry.locations.begin() + step.geometry_begin,
                            leg_geometry.locations.begin() + step.geometry_end));
                    }

                    BOOST_ASSERT(parameters.geometries == RouteParameters::GeometriesType::GeoJSON);
                    return static_cast<util::json::Value>(json::makeGeoJSONGeometry(
                        leg_geometry.locations.begin() + step.geometry_begin,
                        leg_geometry.locations.begin() + step.geometry_end));
                });
        }

        std::vector<util::json::Object> annotations;

        // To maintain support for uses of the old default constructors, we check
        // if annotations property was set manually after default construction
        auto requested_annotations = parameters.annotations_type;
        if ((parameters.annotations == true) &&
            (parameters.annotations_type == RouteParameters::AnnotationsType::None))
        {
            requested_annotations = RouteParameters::AnnotationsType::All;
        }

        if (requested_annotations != RouteParameters::AnnotationsType::None)
        {
            for (const auto idx : util::irange<std::size_t>(0UL, leg_geometries.size()))
            {
                auto &leg_geometry = leg_geometries[idx];
                util::json::Object annotation;

                // AnnotationsType uses bit flags, & operator checks if a property is set
                if (parameters.annotations_type & RouteParameters::AnnotationsType::Speed)
                {
                    double prev_speed = 0;
                    annotation.values["speed"] = GetAnnotations(
                        leg_geometry, [&prev_speed](const guidance::LegGeometry::Annotation &anno) {
                            if (anno.duration < std::numeric_limits<double>::min())
                            {
                                return prev_speed;
                            }
                            else
                            {
                                auto speed = std::round(anno.distance / anno.duration * 10.) / 10.;
                                prev_speed = speed;
                                return util::json::clamp_float(speed);
                            }
                        });
                }

                if (requested_annotations & RouteParameters::AnnotationsType::Duration)
                {
                    annotation.values["duration"] = GetAnnotations(
                        leg_geometry, [](const guidance::LegGeometry::Annotation &anno) {
                            return anno.duration;
                        });
                }
                if (requested_annotations & RouteParameters::AnnotationsType::Distance)
                {
                    annotation.values["distance"] = GetAnnotations(
                        leg_geometry, [](const guidance::LegGeometry::Annotation &anno) {
                            return anno.distance;
                        });
                }
                if (requested_annotations & RouteParameters::AnnotationsType::Weight)
                {
                    annotation.values["weight"] = GetAnnotations(
                        leg_geometry,
                        [](const guidance::LegGeometry::Annotation &anno) { return anno.weight; });
                }
                if (requested_annotations & RouteParameters::AnnotationsType::Datasources)
                {
                    annotation.values["datasources"] = GetAnnotations(
                        leg_geometry, [](const guidance::LegGeometry::Annotation &anno) {
                            return anno.datasource;
                        });
                }
                if (requested_annotations & RouteParameters::AnnotationsType::Nodes)
                {
                    util::json::Array nodes;
                    nodes.values.reserve(leg_geometry.osm_node_ids.size());
                    for (const auto node_id : leg_geometry.osm_node_ids)
                    {
                        nodes.values.push_back(static_cast<std::uint64_t>(node_id));
                    }
                    annotation.values["nodes"] = std::move(nodes);
                }
                // Add any supporting metadata, if needed
                if (requested_annotations & RouteParameters::AnnotationsType::Datasources)
                {
                    const auto MAX_DATASOURCE_ID = 255u;
                    util::json::Object metadata;
                    util::json::Array datasource_names;
                    for (auto i = 0u; i < MAX_DATASOURCE_ID; i++)
                    {
                        const auto name = facade.GetDatasourceName(i);
                        // Length of 0 indicates the first empty name, so we can stop here
                        if (name.size() == 0)
                            break;
                        datasource_names.values.push_back(std::string(facade.GetDatasourceName(i)));
                    }
                    metadata.values["datasource_names"] = datasource_names;
                    annotation.values["metadata"] = metadata;
                }

                annotations.push_back(std::move(annotation));
            }
        }

        auto result = json::makeRoute(route,
                                      json::makeRouteLegs(std::move(legs),
                                                          std::move(step_geometries),
                                                          std::move(annotations)),
                                      std::move(json_overview),
                                      facade.GetWeightName());

        return result;
    }

    const RouteParameters &parameters;

    std::pair<std::vector<guidance::RouteLeg>, std::vector<guidance::LegGeometry>>
    MakeLegs(const std::vector<PhantomNodes> &segment_end_coordinates,
             const std::vector<std::vector<PathData>> &unpacked_path_segments,
             const std::vector<bool> &source_traversed_in_reverse,
             const std::vector<bool> &target_traversed_in_reverse) const {
        auto result = std::make_pair(std::vector<guidance::RouteLeg>(), std::vector<guidance::LegGeometry>());
        auto& legs = result.first;
        auto& leg_geometries = result.second;
        auto number_of_legs = segment_end_coordinates.size();
        legs.reserve(number_of_legs);
        leg_geometries.reserve(number_of_legs);

        for (auto idx : util::irange<std::size_t>(0UL, number_of_legs))
        {
            const auto &phantoms = segment_end_coordinates[idx];
            const auto &path_data = unpacked_path_segments[idx];

            const bool reversed_source = source_traversed_in_reverse[idx];
            const bool reversed_target = target_traversed_in_reverse[idx];

            auto leg_geometry = guidance::assembleGeometry(BaseAPI::facade,
                                                           path_data,
                                                           phantoms.source_phantom,
                                                           phantoms.target_phantom,
                                                           reversed_source,
                                                           reversed_target);
            auto leg = guidance::assembleLeg(facade,
                                             path_data,
                                             leg_geometry,
                                             phantoms.source_phantom,
                                             phantoms.target_phantom,
                                             reversed_target,
                                             parameters.steps);

            util::Log(logDEBUG) << "Assembling steps " << std::endl;
            if (parameters.steps)
            {
                auto steps = guidance::assembleSteps(BaseAPI::facade,
                                                     path_data,
                                                     leg_geometry,
                                                     phantoms.source_phantom,
                                                     phantoms.target_phantom,
                                                     reversed_source,
                                                     reversed_target);

                // Apply maneuver overrides before any other post
                // processing is performed
                guidance::applyOverrides(BaseAPI::facade, steps, leg_geometry);

                // Collapse segregated steps before others
                steps = guidance::collapseSegregatedTurnInstructions(std::move(steps));

                /* Perform step-based post-processing.
                 *
                 * Using post-processing on basis of route-steps for a single leg at a time
                 * comes at the cost that we cannot count the correct exit for roundabouts.
                 * We can only emit the exit nr/intersections up to/starting at a part of the leg.
                 * If a roundabout is not terminated in a leg, we will end up with a
                 *enter-roundabout
                 * and exit-roundabout-nr where the exit nr is out of sync with the previous enter.
                 *
                 *         | S |
                 *         *   *
                 *  ----*        * ----
                 *                  T
                 *  ----*        * ----
                 *       V *   *
                 *         |   |
                 *         |   |
                 *
                 * Coming from S via V to T, we end up with the legs S->V and V->T. V-T will say to
                 *take
                 * the second exit, even though counting from S it would be the third.
                 * For S, we only emit `roundabout` without an exit number, showing that we enter a
                 *roundabout
                 * to find a via point.
                 * The same exit will be emitted, though, if we should start routing at S, making
                 * the overall response consistent.
                 *
                 * ⚠ CAUTION: order of post-processing steps is important
                 *    - handleRoundabouts must be called before collapseTurnInstructions that
                 *      expects post-processed roundabouts
                 */

                guidance::trimShortSegments(steps, leg_geometry);
                leg.steps = guidance::handleRoundabouts(std::move(steps));
                leg.steps = guidance::collapseTurnInstructions(std::move(leg.steps));
                leg.steps = guidance::anticipateLaneChange(std::move(leg.steps));
                leg.steps = guidance::buildIntersections(std::move(leg.steps));
                leg.steps = guidance::suppressShortNameSegments(std::move(leg.steps));
                leg.steps = guidance::assignRelativeLocations(std::move(leg.steps),
                                                              leg_geometry,
                                                              phantoms.source_phantom,
                                                              phantoms.target_phantom);
                leg_geometry = guidance::resyncGeometry(std::move(leg_geometry), leg.steps);
            }

            leg_geometries.push_back(std::move(leg_geometry));
            legs.push_back(std::move(leg));
        }
        return result;
    }

    boost::optional<std::vector<Coordinate>> MakeOverview(const std::vector<guidance::LegGeometry>& leg_geometries) const {
        boost::optional<std::vector<Coordinate>> overview;
        if (parameters.overview != RouteParameters::OverviewType::False)
        {
            const auto use_simplification =
                    parameters.overview == RouteParameters::OverviewType::Simplified;
            BOOST_ASSERT(use_simplification ||
                         parameters.overview == RouteParameters::OverviewType::Full);

            overview = guidance::assembleOverview(leg_geometries, use_simplification);
        }
        return overview;
    }
};

} // ns api
} // ns engine
} // ns osrm

#endif

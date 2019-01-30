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

class RouteAPI : public BaseAPI
{
  public:
    RouteAPI(const datafacade::BaseDataFacade &facade_, const RouteParameters &parameters_)
        : BaseAPI(facade_, parameters_), parameters(parameters_)
    {
    }

    void
    MakeResponse(const InternalManyRoutesResult &raw_routes,
                 const std::vector<PhantomNodes>
                     &all_start_end_points, // all used coordinates, ignoring waypoints= parameter
                 util::json::Object &response) const
    {
        BOOST_ASSERT(!raw_routes.routes.empty());

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
    util::json::Value MakeGeometry(ForwardIter begin, ForwardIter end) const
    {
        if (parameters.geometries == RouteParameters::GeometriesType::Polyline)
        {
            return json::makePolyline<100000>(begin, end);
        }

        if (parameters.geometries == RouteParameters::GeometriesType::Polyline6)
        {
            return json::makePolyline<1000000>(begin, end);
        }

        BOOST_ASSERT(parameters.geometries == RouteParameters::GeometriesType::GeoJSON);
        return json::makeGeoJSONGeometry(begin, end);
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

    util::json::Object MakeRoute(const std::vector<PhantomNodes> &segment_end_coordinates,
                                 const std::vector<std::vector<PathData>> &unpacked_path_segments,
                                 const std::vector<bool> &source_traversed_in_reverse,
                                 const std::vector<bool> &target_traversed_in_reverse) const
    {
        std::vector<guidance::RouteLeg> legs;
        std::vector<guidance::LegGeometry> leg_geometries;
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

        auto route = guidance::assembleRoute(legs);
        boost::optional<util::json::Value> json_overview;
        if (parameters.overview != RouteParameters::OverviewType::False)
        {
            const auto use_simplification =
                parameters.overview == RouteParameters::OverviewType::Simplified;
            BOOST_ASSERT(use_simplification ||
                         parameters.overview == RouteParameters::OverviewType::Full);

            auto overview = guidance::assembleOverview(leg_geometries, use_simplification);
            json_overview = MakeGeometry(overview.begin(), overview.end());
        }

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
};

} // ns api
} // ns engine
} // ns osrm

#endif

#ifndef OSRM_ENGINE_API_ISOCHRONE_API_HPP
#define OSRM_ENGINE_API_ISOCHRONE_API_HPP

#include "engine/api/base_api.hpp"
#include "engine/api/isochrone_parameters.hpp"
#include "engine/api/json_factory.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "engine/isochrone/duration.hpp"
#include "engine/isochrone/weighted_polyline_grid.hpp"
#include "engine/phantom_node.hpp"

#include "util/json_container.hpp"

#include <boost/assert.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace osrm::engine::api
{

class IsochroneAPI final : public BaseAPI
{
  public:
    enum class ResponseStatus
    {
        Ok,
        TooBig
    };

    IsochroneAPI(const datafacade::BaseDataFacade &facade_, const IsochroneParameters &parameters_)
        : BaseAPI(facade_, parameters_), parameters(parameters_)
    {
    }

    ResponseStatus MakeResponse(const isochrone::WeightedGrid &grid,
                                const PhantomNodeCandidates &waypoint_candidates,
                                const std::size_t maximum_output_points,
                                util::json::Object &response) const
    {
        util::json::Array features;
        features.values.reserve(parameters.contours_seconds.size());
        std::size_t output_points = 0;
        std::size_t raw_output_points = 0;

        for (const auto contour_seconds : parameters.contours_seconds)
        {
            // Graph durations have decisecond precision. Keep contouring on
            // the exact limit used by the bounded search.
            const auto duration_cutoff = isochrone::durationCutoffFromSeconds(contour_seconds);
            BOOST_ASSERT(duration_cutoff);
            const auto effective_contour_seconds = isochrone::durationToSeconds(*duration_cutoff);
            std::size_t raw_contour_points = 0;
            auto polygons = grid.buildContours(effective_contour_seconds,
                                               maximum_output_points - raw_output_points,
                                               parameters.generalize.value_or(0.),
                                               parameters.denoise.value_or(0.),
                                               raw_contour_points);
            if (!polygons || !countCoordinates(*polygons, maximum_output_points, output_points))
                return ResponseStatus::TooBig;
            raw_output_points += raw_contour_points;

            auto feature =
                makeFeature(std::move(*polygons), contour_seconds, effective_contour_seconds);
            features.values.emplace_back(std::move(feature));
        }

        response.values["code"] = "Ok";
        response.values["type"] = "FeatureCollection";
        response.values["features"] = std::move(features);
        if (!parameters.skip_waypoints)
            response.values["waypoints"] = makeWaypoints(waypoint_candidates);

        const auto data_timestamp = facade.GetTimestamp();
        if (!data_timestamp.empty())
            response.values["data_version"] = data_timestamp;

        return ResponseStatus::Ok;
    }

  private:
    static bool countCoordinates(const std::vector<isochrone::CoordinatePolygon> &polygons,
                                 const std::size_t maximum,
                                 std::size_t &count)
    {
        const auto add = [maximum, &count](const std::size_t additional)
        {
            if (count > maximum || additional > maximum - count)
                return false;
            count += additional;
            return true;
        };

        for (const auto &polygon : polygons)
        {
            if (!add(polygon.outer.size()))
                return false;
            for (const auto &hole : polygon.holes)
            {
                if (!add(hole.size()))
                    return false;
            }
        }
        return true;
    }

    util::json::Array makeWaypoints(const PhantomNodeCandidates &candidates) const
    {
        util::json::Array waypoints;
        waypoints.values.emplace_back(MakeWaypoint(candidates));
        return waypoints;
    }

    static util::json::Array makeRing(const isochrone::CoordinateRing &ring)
    {
        util::json::Array coordinates;
        coordinates.values.reserve(ring.size());
        for (const auto coordinate : ring)
            coordinates.values.push_back(json::detail::coordinateToLonLat(coordinate));
        return coordinates;
    }

    static util::json::Array makePolygon(const isochrone::CoordinatePolygon &polygon)
    {
        util::json::Array rings;
        rings.values.reserve(polygon.holes.size() + 1);
        rings.values.emplace_back(makeRing(polygon.outer));
        for (const auto &hole : polygon.holes)
            rings.values.emplace_back(makeRing(hole));
        return rings;
    }

    util::json::Object makeGeometry(const std::vector<isochrone::CoordinatePolygon> &polygons) const
    {
        util::json::Object geometry;
        util::json::Array coordinates;

        if (parameters.polygons)
        {
            geometry.values["type"] = "MultiPolygon";
            coordinates.values.reserve(polygons.size());
            for (const auto &polygon : polygons)
                coordinates.values.emplace_back(makePolygon(polygon));
        }
        else
        {
            geometry.values["type"] = "MultiLineString";
            for (const auto &polygon : polygons)
            {
                coordinates.values.emplace_back(makeRing(polygon.outer));
                for (const auto &hole : polygon.holes)
                    coordinates.values.emplace_back(makeRing(hole));
            }
        }

        geometry.values["coordinates"] = std::move(coordinates);
        return geometry;
    }

    util::json::Object makeFeature(std::vector<isochrone::CoordinatePolygon> polygons,
                                   const double contour_seconds,
                                   const double effective_contour_seconds) const
    {
        util::json::Object properties;
        properties.values["contour_seconds"] = contour_seconds;
        properties.values["effective_contour_seconds"] = effective_contour_seconds;

        util::json::Object feature;
        feature.values["type"] = "Feature";
        feature.values["properties"] = std::move(properties);
        feature.values["geometry"] = makeGeometry(polygons);
        return feature;
    }

    const IsochroneParameters &parameters;
};

} // namespace osrm::engine::api

#endif // OSRM_ENGINE_API_ISOCHRONE_API_HPP

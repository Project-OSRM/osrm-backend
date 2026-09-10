#include "engine/plugins/isochrone.hpp"

#include "engine/api/isochrone_api.hpp"
#include "engine/api/isochrone_parameters.hpp"
#include "engine/isochrone/duration.hpp"
#include "engine/isochrone/search_result_materialization.hpp"
#include "engine/isochrone/weighted_polyline_grid.hpp"

#include <algorithm>
#include <cmath>
#include <variant>

namespace osrm::engine::plugins
{
Status IsochronePlugin::HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                                      const api::IsochroneParameters &parameters,
                                      osrm::engine::api::ResultT &result) const
{
    if (!parameters.IsValid())
        return Error("InvalidOptions", "Invalid isochrone parameters.", result);

    if (!CheckAllCoordinates(parameters.coordinates))
        return Error("InvalidOptions", "Coordinates are invalid", result);

    if (!CheckAlgorithms(parameters, algorithms, result))
        return Status::Error;

    if (!algorithms.HasIsochroneSearch())
    {
        return Error(
            "NotImplemented", "Isochrone search is not available for this dataset.", result);
    }

    if (parameters.format != api::BaseParameters::OutputFormatType::JSON)
    {
        return Error(
            "NotImplemented", "The isochrone service only supports JSON/GeoJSON output.", result);
    }

    if (!std::holds_alternative<util::json::Object>(result))
    {
        return Error(
            "NotImplemented", "The isochrone service only supports JSON/GeoJSON output.", result);
    }

    if (parameters.contours.size() > max_contours)
    {
        return Error("TooBig",
                     "Number of contours is higher than current maximum (" +
                         std::to_string(max_contours) + ")",
                     result);
    }

    const auto &facade = algorithms.GetFacade();
    EdgeDuration maximum_duration{0};
    for (const auto contour : parameters.contours)
    {
        const auto contour_duration = isochrone::durationCutoffFromSeconds(contour);
        if (!contour_duration || *contour_duration < EdgeDuration{1})
        {
            const auto truncated_contour =
                std::floor(contour * isochrone::INTERNAL_DURATION_UNITS_PER_SECOND);
            if (std::isfinite(truncated_contour) && truncated_contour < 1.)
            {
                return Error("InvalidValue",
                             "Contour is below the supported duration precision of 0.1 seconds.",
                             result);
            }
            return Error("InvalidValue",
                         "Contour exceeds the maximum supported duration in seconds.",
                         result);
        }
        maximum_duration = std::max(maximum_duration, *contour_duration);
    }

    const auto role = parameters.direction == api::IsochroneParameters::Direction::Outbound
                          ? area::ApproachRole::Departure
                          : area::ApproachRole::Arrival;
    auto phantom_node_pairs = GetPhantomNodesForRole(facade, parameters, role);
    if (phantom_node_pairs.size() != parameters.coordinates.size())
    {
        return Error("NoSegment",
                     MissingPhantomErrorMessage(phantom_node_pairs, parameters.coordinates),
                     result);
    }
    auto snapped_phantoms = SnapPhantomNodes(std::move(phantom_node_pairs));
    BOOST_ASSERT(snapped_phantoms.size() == 1);

    const auto search_result = algorithms.IsochroneSearch(
        snapped_phantoms.front(),
        maximum_duration,
        max_search_records,
        parameters.direction == api::IsochroneParameters::Direction::Inbound);
    switch (search_result.status)
    {
    case isochrone::SearchStatus::Complete:
        break;
    case isochrone::SearchStatus::SearchRecordLimitReached:
        return Error(
            "TooBig", "Isochrone search exceeded the maximum number of search records.", result);
    case isochrone::SearchStatus::ArithmeticOverflow:
        return Error("InternalError", "Isochrone search encountered arithmetic overflow.", result);
    }

    const isochrone::MaterializationLimits materialization_limits{max_materialized_points,
                                                                  max_materialized_points,
                                                                  max_materialized_points,
                                                                  max_materialized_points};
    const auto materialization_result = isochrone::materializeSearchResult(
        facade,
        search_result,
        maximum_duration,
        parameters.direction == api::IsochroneParameters::Direction::Inbound,
        materialization_limits);
    switch (materialization_result.error)
    {
    case isochrone::MaterializationError::None:
        break;
    case isochrone::MaterializationError::BudgetExceeded:
        return Error("TooBig", "Isochrone geometry exceeds the configured limit.", result);
    case isochrone::MaterializationError::RequiresLongitudeWrap:
        return Error("NotImplemented",
                     "Isochrone geometry crossing the antimeridian is not supported.",
                     result);
    case isochrone::MaterializationError::TouchesPole:
        return Error("NotImplemented",
                     "Isochrone geometry touching a geographic pole is not supported.",
                     result);
    default:
        return Error("InternalError", "Unable to materialize isochrone geometry.", result);
    }

    const auto rasterization =
        isochrone::rasterizeWeightedPolylines(materialization_result.polylines,
                                              {100., max_grid_cells, max_rasterization_steps},
                                              parameters.coordinates.front());
    switch (rasterization.error)
    {
    case isochrone::RasterizationError::None:
        break;
    case isochrone::RasterizationError::InvalidOptions:
        return Error("InternalError", "Isochrone rasterization configuration is invalid.", result);
    case isochrone::RasterizationError::TooBig:
        return Error("TooBig", "Isochrone rasterization exceeds the configured limit.", result);
    case isochrone::RasterizationError::TouchesLongitudeBoundary:
        return Error("NotImplemented",
                     "Isochrone geometry at the longitude world boundary is not supported.",
                     result);
    case isochrone::RasterizationError::TouchesPole:
        return Error("NotImplemented",
                     "Isochrone geometry touching a geographic pole is not supported.",
                     result);
    }
    BOOST_ASSERT(rasterization.grid);

    api::IsochroneAPI isochrone_api{facade, parameters};
    auto &json_result = std::get<util::json::Object>(result);
    if (isochrone_api.MakeResponse(
            *rasterization.grid, snapped_phantoms.front(), max_output_points, json_result) ==
        api::IsochroneAPI::ResponseStatus::TooBig)
    {
        return Error("TooBig", "Isochrone response exceeds the configured limit.", result);
    }

    return Status::Ok;
}

} // namespace osrm::engine::plugins

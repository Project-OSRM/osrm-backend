#include "server/service/isochrone_service.hpp"

#include <boost/assert.hpp>

#include "server/service/utils.hpp"

#include "server/api/parameters_parser.hpp"
#include "engine/api/isochrone_parameters.hpp"

#include "util/json_container.hpp"

#include <algorithm>
#include <cmath>
#include <variant>

namespace osrm::server::service
{

namespace
{

std::string getWrongOptionHelp(const engine::api::IsochroneParameters &parameters)
{
    std::string help;
    const auto coordinate_count = parameters.coordinates.size();

    const bool parameter_size_mismatch =
        constrainParamSize(
            PARAMETER_SIZE_MISMATCH_MSG, "hints", parameters.hints, coordinate_count, help) ||
        constrainParamSize(
            PARAMETER_SIZE_MISMATCH_MSG, "bearings", parameters.bearings, coordinate_count, help) ||
        constrainParamSize(
            PARAMETER_SIZE_MISMATCH_MSG, "radiuses", parameters.radiuses, coordinate_count, help) ||
        constrainParamSize(PARAMETER_SIZE_MISMATCH_MSG,
                           "approaches",
                           parameters.approaches,
                           coordinate_count,
                           help);

    if (!parameter_size_mismatch && parameters.coordinates.size() != 1)
        help = "Exactly one coordinate is required.";
    else if (!parameter_size_mismatch && parameters.contours.empty())
        help = "At least one contour is required.";
    else if (!parameter_size_mismatch &&
             std::any_of(parameters.contours.begin(),
                         parameters.contours.end(),
                         [](const double contour)
                         { return !std::isfinite(contour) || contour <= 0.; }))
    {
        help = "Contours must be finite durations in seconds and greater than zero.";
    }

    return help;
}

engine::Status runIsochrone(OSRM &routing_machine,
                            const engine::api::IsochroneParameters &parameters,
                            osrm::engine::api::ResultT &result)
{
    if (!parameters.IsValid())
    {
        auto &json_result = std::get<util::json::Object>(result);
        json_result.values["code"] = "InvalidOptions";
        json_result.values["message"] = getWrongOptionHelp(parameters);
        return engine::Status::Error;
    }
    BOOST_ASSERT(parameters.IsValid());

    if (parameters.format != engine::api::BaseParameters::OutputFormatType::JSON)
    {
        auto &json_result = std::get<util::json::Object>(result);
        json_result.values["code"] = "NotImplemented";
        json_result.values["message"] = "The isochrone service only supports JSON/GeoJSON output.";
        return engine::Status::Error;
    }

    return routing_machine.Isochrone(parameters, result);
}

} // namespace

engine::Status IsochroneService::RunQuery(std::size_t prefix_length,
                                          std::string &query,
                                          osrm::engine::api::ResultT &result)
{
    result = util::json::Object();
    auto &json_result = std::get<util::json::Object>(result);

    auto query_iterator = query.begin();
    auto parameters =
        api::parseParameters<engine::api::IsochroneParameters>(query_iterator, query.end());
    if (!parameters || query_iterator != query.end())
    {
        const auto position = std::distance(query.begin(), query_iterator);
        json_result.values["code"] = "InvalidQuery";
        json_result.values["message"] =
            "Query string malformed close to position " + std::to_string(prefix_length + position);
        return engine::Status::Error;
    }
    BOOST_ASSERT(parameters);

    return runIsochrone(BaseService::routing_machine, *parameters, result);
}

} // namespace osrm::server::service

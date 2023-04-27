#include "server/service/nearest_service.hpp"
#include "server/service/utils.hpp"

#include "server/api/parameters_parser.hpp"
#include "engine/api/nearest_parameters.hpp"

#include "util/json_container.hpp"

#include <boost/format.hpp>

namespace osrm::server::service
{

namespace
{
std::string getWrongOptionHelp(const engine::api::NearestParameters &parameters)
{
    std::string help;

    const auto coord_size = parameters.coordinates.size();

    constrainParamSize(PARAMETER_SIZE_MISMATCH_MSG, "hints", parameters.hints, coord_size, help);
    constrainParamSize(
        PARAMETER_SIZE_MISMATCH_MSG, "bearings", parameters.bearings, coord_size, help);
    constrainParamSize(
        PARAMETER_SIZE_MISMATCH_MSG, "radiuses", parameters.radiuses, coord_size, help);
    constrainParamSize(
        PARAMETER_SIZE_MISMATCH_MSG, "approaches", parameters.approaches, coord_size, help);

    return help;
}
} // namespace

engine::Status NearestService::RunQuery(std::size_t prefix_length,
                                        std::string &query,
                                        osrm::engine::api::ResultT &result)
{
    result = util::json::Object();
    auto &json_result = result.get<util::json::Object>();

    auto query_iterator = query.begin();
    auto parameters =
        api::parseParameters<engine::api::NearestParameters>(query_iterator, query.end());
    if (!parameters || query_iterator != query.end())
    {
        const auto position = std::distance(query.begin(), query_iterator);
        json_result.values["code"] = "InvalidQuery";
        json_result.values["message"] =
            "Query string malformed close to position " + std::to_string(prefix_length + position);
        return engine::Status::Error;
    }
    BOOST_ASSERT(parameters);

    if (!parameters->IsValid())
    {
        json_result.values["code"] = "InvalidOptions";
        json_result.values["message"] = getWrongOptionHelp(*parameters);
        return engine::Status::Error;
    }
    BOOST_ASSERT(parameters->IsValid());

    if (parameters->format)
    {
        if (parameters->format == engine::api::BaseParameters::OutputFormatType::FLATBUFFERS)
        {
            result = flatbuffers::FlatBufferBuilder();
        }
    }
    return BaseService::routing_machine.Nearest(*parameters, result);
}
} // namespace osrm::server::service

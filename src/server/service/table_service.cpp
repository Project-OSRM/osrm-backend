#include "server/service/table_service.hpp"

#include <boost/assert.hpp>

#include "server/api/json_parameters_parser.hpp"
#include "server/api/parameters_parser.hpp"
#include "engine/api/table_parameters.hpp"

#include "util/format.hpp"
#include "util/json_container.hpp"

namespace osrm::server::service
{

namespace
{

const constexpr char PARAMETER_SIZE_MISMATCH_MSG[] =
    "Number of elements in {} size {} does not match coordinate size {}";

template <typename ParamT>
bool constrainParamSize(const char *msg_template,
                        const char *name,
                        const ParamT &param,
                        const std::size_t target_size,
                        std::string &help)
{
    if (param.size() > 0 && param.size() != target_size)
    {
        help = osrm::util::compat::format(
            osrm::util::compat::runtime_format(msg_template), name, param.size(), target_size);
        return true;
    }
    return false;
}

std::string getWrongOptionHelp(const engine::api::TableParameters &parameters)
{
    std::string help;

    const auto coord_size = parameters.coordinates.size();

    const bool param_size_mismatch =
        constrainParamSize(
            PARAMETER_SIZE_MISMATCH_MSG, "hints", parameters.hints, coord_size, help) ||
        constrainParamSize(
            PARAMETER_SIZE_MISMATCH_MSG, "bearings", parameters.bearings, coord_size, help) ||
        constrainParamSize(
            PARAMETER_SIZE_MISMATCH_MSG, "radiuses", parameters.radiuses, coord_size, help) ||
        constrainParamSize(
            PARAMETER_SIZE_MISMATCH_MSG, "approaches", parameters.approaches, coord_size, help);

    if (!param_size_mismatch && parameters.coordinates.size() < 2)
    {
        help = "Number of coordinates needs to be at least two.";
    }

    if (parameters.fallback_speed <= 0)
    {
        help = "fallback_speed must be > 0";
    }

    if (parameters.scale_factor <= 0)
    {
        help = "scale_factor must be > 0";
    }

    return help;
}

// Shared tail for the GET and POST paths: validate the parsed parameters and run the query.
// `result` must hold a util::json::Object on entry (for error messages).
engine::Status runTable(OSRM &routing_machine,
                        const engine::api::TableParameters &parameters,
                        osrm::engine::api::ResultT &result)
{
    if (!parameters.IsValid())
    {
        auto &json_result = std::get<util::json::Object>(result);
        json_result.values["code"] = "InvalidOptions";
        json_result.values["message"] = getWrongOptionHelp(parameters);
        return engine::Status::Error;
    }

    if (parameters.format == engine::api::BaseParameters::OutputFormatType::FLATBUFFERS)
    {
        result = flatbuffers::FlatBufferBuilder();
    }
    return routing_machine.Table(parameters, result);
}
} // namespace

engine::Status TableService::RunQuery(std::size_t prefix_length,
                                      std::string &query,
                                      osrm::engine::api::ResultT &result)
{
    result = util::json::Object();
    auto &json_result = std::get<util::json::Object>(result);

    auto query_iterator = query.begin();
    auto parameters =
        api::parseParameters<engine::api::TableParameters>(query_iterator, query.end());
    if (!parameters || query_iterator != query.end())
    {
        const auto position = std::distance(query.begin(), query_iterator);
        json_result.values["code"] = "InvalidQuery";
        json_result.values["message"] =
            "Query string malformed close to position " + std::to_string(prefix_length + position);
        return engine::Status::Error;
    }
    BOOST_ASSERT(parameters);

    return runTable(BaseService::routing_machine, *parameters, result);
}

engine::Status TableService::RunJSONQuery(const std::string &json_body,
                                          osrm::engine::api::ResultT &result)
{
    result = util::json::Object();
    auto &json_result = std::get<util::json::Object>(result);

    std::string error;
    auto parameters = api::parseJSONParameters<engine::api::TableParameters>(json_body, error);
    if (!parameters)
    {
        json_result.values["code"] = "InvalidQuery";
        json_result.values["message"] = error;
        return engine::Status::Error;
    }

    return runTable(BaseService::routing_machine, *parameters, result);
}
} // namespace osrm::server::service

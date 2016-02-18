#include "server/service/table_service.hpp"

#include "engine/api/route_parameters.hpp"
#include "server/api/parameters_parser.hpp"

#include "util/json_container.hpp"

#include <boost/format.hpp>

namespace osrm
{
namespace server
{
namespace service
{

namespace
{

const constexpr char PARAMETER_SIZE_MISMATCH_MSG[] =
    "Number of elements in %1% size %2% does not match coordinate size %3%";

template <typename ParamT>
bool constrainParamSize(const char *msg_template,
                        const char *name,
                        const ParamT &param,
                        const std::size_t target_size,
                        std::string &help)
{
    if (param.size() > 0 && param.size() != target_size)
    {
        help = (boost::format(msg_template) % name % param.size() % target_size).str();
        return true;
    }
    return false;
}

std::string getWrongOptionHelp(const engine::api::TableParameters &parameters)
{
    std::string help;

    const auto coord_size = parameters.coordinates.size();

    const bool param_size_mismatch = constrainParamSize(PARAMETER_SIZE_MISMATCH_MSG, "hints",
                                                        parameters.hints, coord_size, help) ||
                                     constrainParamSize(PARAMETER_SIZE_MISMATCH_MSG, "bearings",
                                                        parameters.bearings, coord_size, help) ||
                                     constrainParamSize(PARAMETER_SIZE_MISMATCH_MSG, "radiuses",
                                                        parameters.radiuses, coord_size, help);

    if (!param_size_mismatch && parameters.coordinates.size() < 2)
    {
        help = "Number of coordinates needs to be at least two.";
    }

    return help;
}
} // anon. ns

engine::Status TableService::RunQuery(std::vector<util::FixedPointCoordinate> coordinates,
                                      std::string &options,
                                      util::json::Object &result)
{

    auto options_iterator = options.begin();
    auto parameters =
        api::parseParameters<engine::api::TableParameters>(options_iterator, options.end());
    if (!parameters || options_iterator != options.end())
    {
        const auto position = std::distance(options.begin(), options_iterator);
        result.values["code"] = "invalid-options";
        result.values["message"] =
            "Options string malformed close to position " + std::to_string(position);
        return engine::Status::Error;
    }

    BOOST_ASSERT(parameters);
    parameters->coordinates = std::move(coordinates);

    if (!parameters->IsValid())
    {
        result.values["code"] = "invalid-options";
        result.values["message"] = getWrongOptionHelp(*parameters);
        return engine::Status::Error;
    }
    BOOST_ASSERT(parameters->IsValid());

    return BaseService::routing_machine.Table(*parameters, result);
}
}
}
}

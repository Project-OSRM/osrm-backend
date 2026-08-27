#include "engine/plugins/nearest.hpp"
#include "engine/api/nearest_api.hpp"
#include "engine/api/nearest_parameters.hpp"

#include <string>

#include <boost/assert.hpp>
#include <boost/numeric/conversion/cast.hpp>

namespace osrm::engine::plugins
{

NearestPlugin::NearestPlugin(const int max_results_,
                             const int max_locations_nearest_,
                             const std::optional<double> default_radius_)
    : BasePlugin(default_radius_), max_results{max_results_},
      max_locations_nearest{max_locations_nearest_}
{
}

Status NearestPlugin::HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                                    const api::NearestParameters &params,
                                    osrm::engine::api::ResultT &result) const
{
    if (params.coordinates.empty())
        return Error("InvalidOptions", "Coordinates are invalid", result);
    BOOST_ASSERT(params.IsValid());

    if (!CheckAlgorithms(params, algorithms, result))
        return Status::Error;

    const auto &facade = algorithms.GetFacade();

    if (max_results > 0 &&
        (boost::numeric_cast<std::int64_t>(params.number_of_results) > max_results))
    {
        return Error("TooBig",
                     "Number of results " + std::to_string(params.number_of_results) +
                         " is higher than current maximum (" + std::to_string(max_results) + ")",
                     result);
    }

    if (!CheckAllCoordinates(params.coordinates))
        return Error("InvalidOptions", "Coordinates are invalid", result);

    if (max_locations_nearest > 0 &&
        (boost::numeric_cast<std::int64_t>(params.coordinates.size()) > max_locations_nearest))
    {
        return Error("TooBig",
                     "Number of coordinates " + std::to_string(params.coordinates.size()) +
                         " is higher than current maximum (" +
                         std::to_string(max_locations_nearest) + ")",
                     result);
    }

    auto phantom_nodes = GetPhantomNodes(facade, params, params.number_of_results);

    if (phantom_nodes.size() == 1 && phantom_nodes.front().empty())
    {
        return Error("NoSegment", "Could not find a matching segment for coordinate", result);
    }

    api::NearestAPI nearest_api(facade, params);
    nearest_api.MakeResponse(phantom_nodes, result);
    return Status::Ok;
}
} // namespace osrm::engine::plugins

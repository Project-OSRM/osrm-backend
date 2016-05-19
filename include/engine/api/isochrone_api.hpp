//
// Created by robin on 4/13/16.
//

#ifndef ENGINE_API_ISOCHRONE_HPP
#define ENGINE_API_ISOCHRONE_HPP

#include "engine/api/base_api.hpp"
#include "engine/api/isochrone_parameters.hpp"
#include "engine/plugins/isochrone.hpp"
#include "engine/plugins/plugin_base.hpp"

namespace osrm
{
namespace engine
{
namespace api
{

class IsochroneAPI final : public BaseAPI
{

  public:
    const IsochroneParameters &parameters;

    IsochroneAPI(const datafacade::BaseDataFacade &facade_, const IsochroneParameters &parameters_)
        : BaseAPI(facade_, parameters_), parameters(parameters_)
    {
    }

    void MakeResponse(const std::vector<engine::plugins::IsochroneNode> isochroneNodes,
                      const std::vector<engine::plugins::IsochroneNode> convexhull,
                      util::json::Object &response) const
    {
        util::json::Array data;
        for (auto isochrone : isochroneNodes)
        {
            util::json::Object object;

            util::json::Object source;
            source.values["lat"] = static_cast<double>(util::toFloating(isochrone.node.lat));
            source.values["lon"] = static_cast<double>(util::toFloating(isochrone.node.lon));
            object.values["p1"] = std::move(source);

            util::json::Object predecessor;
            predecessor.values["lat"] =
                    static_cast<double>(util::toFloating(isochrone.predecessor.lat));
            predecessor.values["lon"] =
                    static_cast<double>(util::toFloating(isochrone.predecessor.lon));
            object.values["p2"] = std::move(predecessor);

            util::json::Object distance;
            object.values["distance_from_start"] = isochrone.distance;

            data.values.push_back(object);
        }
        response.values["isochrone"] = std::move(data);
        if(!convexhull.empty()) {
            util::json::Array convexhullArray;
            for (engine::plugins::IsochroneNode n : convexhull)
            {
                util::json::Object point;
                point.values["lat"] = static_cast<double>(util::toFloating(n.node.lat));
                point.values["lon"] = static_cast<double>(util::toFloating(n.node.lon));
                convexhullArray.values.push_back(point);
            }
            response.values["convexhull"] = std::move(convexhullArray);
        }

    }
};
}
}
}
#endif // ENGINE_API_ISOCHRONE_HPP

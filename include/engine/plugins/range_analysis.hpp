//
// Created by robin on 3/10/16.
//

#ifndef OSRM_TEST_PLUGIN_HPP
#define OSRM_TEST_PLUGIN_HPP

#include "engine/plugins/plugin_base.hpp"
#include "engine/search_engine.hpp"
#include "engine/object_encoder.hpp"

#include "util/make_unique.hpp"
#include "util/string_util.hpp"
#include "util/simple_logger.hpp"
#include "osrm/json_container.hpp"

#include <cstdlib>

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <string>

namespace osrm
{
namespace engine
{
namespace plugins
{

template <class DataFacadeT> class RangeAnalysis final : public BasePlugin
{

    using PredecessorMap = std::unordered_map<NodeID, NodeID>;
    using DistanceMap = std::unordered_map<NodeID, EdgeWeight>;

  private:
    std::string temp_string;
    std::string descriptor_string;
    std::unique_ptr<SearchEngine<DataFacadeT>> search_engine_ptr;
    DataFacadeT *facade;
    PredecessorMap predecessorMap;
    DistanceMap distanceMap;

  public:
    explicit RangeAnalysis(DataFacadeT *facade) : descriptor_string("range"), facade(facade)
    {
        search_engine_ptr = util::make_unique<SearchEngine<DataFacadeT>>(facade);
    }

    virtual ~RangeAnalysis() {}

    const std::string GetDescriptor() const override final { return descriptor_string; }

    Status HandleRequest(const RouteParameters &routeParameters,
                         util::json::Object &json_result) override final
    {
        /* Check if valid
         *
         */
        if (routeParameters.coordinates.size() != 1)
        {
            json_result.values["status_message"] = "Number of Coordinates should be 1";
            return Status::Error;
        }
        if (!routeParameters.coordinates.front().IsValid())
        {
            json_result.values["status_message"] = "Coordinate is invalid";
            return Status::Error;
        }

        const auto &input_bearings = routeParameters.bearings;
        if (input_bearings.size() > 0 &&
            routeParameters.coordinates.size() != input_bearings.size())
        {
            json_result.values["status_message"] =
                "Number of bearings does not match number of coordinate";
            return Status::Error;
        }

        /** Interesting code starts here
         *
         */

        //        auto number_of_results = static_cast<std::size_t>(routeParameters.num_results);
        const int bearing = input_bearings.size() > 0 ? input_bearings.front().first : 0;
        const int range =
            input_bearings.size() > 0
                ? (input_bearings.front().second ? *input_bearings.front().second : 10)
                : 180;

        std::vector<PhantomNodePair> phantomNodeVector;
        auto phantomNodePair = facade->NearestPhantomNodeWithAlternativeFromBigComponent(
            routeParameters.coordinates.front(), bearing, range);

        phantomNodeVector.push_back(phantomNodePair);
        auto snapped_source_phantoms = snapPhantomNodes(phantomNodeVector);
        search_engine_ptr->oneToMany(snapped_source_phantoms.front(), 10000, predecessorMap,
                                     distanceMap);

        BOOST_ASSERT(predecessorMap.size() == distanceMap.size());

        std::string temp_string;
        json_result.values["title"] = "Range Analysis";

        util::json::Array data;
        for (auto it = predecessorMap.begin(); it != predecessorMap.end(); ++it)
        {
            util::json::Object object;

            util::json::Object source;
            FixedPointCoordinate sourceCoordinate = facade->GetCoordinateOfNode(it->first);
            source.values["lat"] = sourceCoordinate.lat / COORDINATE_PRECISION;
            source.values["lon"] = sourceCoordinate.lon / COORDINATE_PRECISION;
            object.values["Source"] = std::move(source);

            util::json::Object predecessor;
            FixedPointCoordinate destinationSource = facade->GetCoordinateOfNode(it->second);
            predecessor.values["lat"] = destinationSource.lat / COORDINATE_PRECISION;
            predecessor.values["lon"] = destinationSource.lon / COORDINATE_PRECISION;
            object.values["Predecessor"] = std::move(predecessor);

            util::json::Object distance;
            object.values["distance_from_start"] = distanceMap[it->first];

            data.values.push_back(object);
        }
        temp_string = std::to_string(distanceMap.size());
        json_result.values["Nodes Found"] = temp_string;;
        json_result.values["Range-Analysis"] = std::move(data);

        return Status::Ok;
    }
};
}
}
}
#endif // OSRM_TEST_PLUGIN_HPP

/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef MAP_MATCHING_PLUGIN_H
#define MAP_MATCHING_PLUGIN_H

#include "plugin_base.hpp"

#include "../algorithms/object_encoder.hpp"
#include "../util/integer_range.hpp"
#include "../data_structures/search_engine.hpp"
#include "../routing_algorithms/map_matching.hpp"
#include "../util/simple_logger.hpp"
#include "../util/string_util.hpp"

#include <cstdlib>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

template <class DataFacadeT> class MapMatchingPlugin : public BasePlugin
{
  private:
    std::shared_ptr<SearchEngine<DataFacadeT>> search_engine_ptr;

  public:
    MapMatchingPlugin(DataFacadeT *facade) : descriptor_string("match"), facade(facade)
    {
        search_engine_ptr = std::make_shared<SearchEngine<DataFacadeT>>(facade);
    }

    virtual ~MapMatchingPlugin() { search_engine_ptr.reset(); }

    const std::string GetDescriptor() const final { return descriptor_string; }

    int HandleRequest(const RouteParameters &route_parameters, JSON::Object &json_result) final
    {
        // check number of parameters

        SimpleLogger().Write() << "1";
        if (!check_all_coordinates(route_parameters.coordinates))
        {
            return 400;
        }

        SimpleLogger().Write() << "2";

        InternalRouteResult raw_route;
        Matching::CandidateLists candidate_lists;
        candidate_lists.resize(route_parameters.coordinates.size());

        SimpleLogger().Write() << "3";
        // fetch  10 candidates for each given coordinate
        for (const auto current_coordinate : osrm::irange<std::size_t>(0, candidate_lists.size()))
        {
            if (!facade->IncrementalFindPhantomNodeForCoordinateWithDistance(
                    route_parameters.coordinates[current_coordinate],
                    candidate_lists[current_coordinate],
                    10))
            {
                return 400;
            }

            while (candidate_lists[current_coordinate].size() < 10)
            {
                // TODO: add dummy candidates, if any are missing
                // TODO: add factory method to get an invalid PhantomNode/Distance pair
            }
        }
        SimpleLogger().Write() << "4";

        // call the actual map matching
        search_engine_ptr->map_matching(10, candidate_lists, route_parameters.coordinates, raw_route);

        if (INVALID_EDGE_WEIGHT == raw_route.shortest_path_length)
        {
            SimpleLogger().Write(logDEBUG) << "Error occurred, single path not found";
        }

        return 200;
    }

  private:
    std::string descriptor_string;
    DataFacadeT *facade;
};

#endif /* MAP_MATCHING_PLUGIN_H */

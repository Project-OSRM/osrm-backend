#include "engine/plugins/trip.hpp"

#include "extractor/tarjan_scc.hpp"

#include "engine/api/trip_api.hpp"
#include "engine/api/trip_parameters.hpp"
#include "engine/trip/trip_brute_force.hpp"
#include "engine/trip/trip_farthest_insertion.hpp"
#include "engine/trip/trip_nearest_neighbour.hpp"
#include "util/dist_table_wrapper.hpp" // to access the dist table more easily
#include "util/json_container.hpp"
#include "util/matrix_graph_wrapper.hpp" // wrapper to use tarjan scc on dist table

#include <boost/assert.hpp>

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace osrm
{
namespace engine
{
namespace plugins
{

// Object to hold all strongly connected components (scc) of a graph
// to access all graphs with component ID i, get the iterators by:
// auto start = std::begin(scc_component.component) + scc_component.range[i];
// auto end = std::begin(scc_component.component) + scc_component.range[i+1];
struct SCC_Component
{
    // in_component: all NodeIDs sorted by component ID
    // in_range: index where a new component starts
    //
    // example: NodeID 0, 1, 2, 4, 5 are in component 0
    //          NodeID 3, 6, 7, 8    are in component 1
    //          => in_component = [0, 1, 2, 4, 5, 3, 6, 7, 8]
    //          => in_range = [0, 5]
    SCC_Component(std::vector<NodeID> in_component_nodes, std::vector<size_t> in_range)
        : component(std::move(in_component_nodes)), range(std::move(in_range))
    {
        BOOST_ASSERT_MSG(component.size() > 0, "there's no scc component");
        BOOST_ASSERT_MSG(*std::max_element(range.begin(), range.end()) == component.size(),
                         "scc component ranges are out of bound");
        BOOST_ASSERT_MSG(*std::min_element(range.begin(), range.end()) == 0,
                         "invalid scc component range");
        BOOST_ASSERT_MSG(std::is_sorted(std::begin(range), std::end(range)),
                         "invalid component ranges");
    }

    std::size_t GetNumberOfComponents() const
    {
        BOOST_ASSERT_MSG(range.size() > 0, "there's no range");
        return range.size() - 1;
    }

    std::vector<NodeID> component;
    std::vector<std::size_t> range;
};

// takes the number of locations and its duration matrix,
// identifies and splits the graph in its strongly connected components (scc)
// and returns an SCC_Component
SCC_Component SplitUnaccessibleLocations(const std::size_t number_of_locations,
                                         const util::DistTableWrapper<EdgeWeight> &result_table)
{
    if (std::find(std::begin(result_table), std::end(result_table), INVALID_EDGE_WEIGHT) ==
        std::end(result_table))
    {
        // whole graph is one scc
        std::vector<NodeID> location_ids(number_of_locations);
        std::iota(std::begin(location_ids), std::end(location_ids), 0);
        std::vector<size_t> range = {0, location_ids.size()};
        return SCC_Component(std::move(location_ids), std::move(range));
    }

    // Run TarjanSCC
    auto wrapper = std::make_shared<util::MatrixGraphWrapper<EdgeWeight>>(result_table.GetTable(),
                                                                          number_of_locations);
    auto scc = extractor::TarjanSCC<util::MatrixGraphWrapper<EdgeWeight>>(wrapper);
    scc.Run();

    const auto number_of_components = scc.GetNumberOfComponents();

    std::vector<std::size_t> range_insertion;
    std::vector<std::size_t> range;
    range_insertion.reserve(number_of_components);
    range.reserve(number_of_components);

    std::vector<NodeID> components(number_of_locations, 0);

    std::size_t prefix = 0;
    for (std::size_t j = 0; j < number_of_components; ++j)
    {
        range_insertion.push_back(prefix);
        range.push_back(prefix);
        prefix += scc.GetComponentSize(j);
    }
    // senitel
    range.push_back(components.size());

    for (std::size_t i = 0; i < number_of_locations; ++i)
    {
        components[range_insertion[scc.GetComponentID(i)]] = i;
        ++range_insertion[scc.GetComponentID(i)];
    }

    return SCC_Component(std::move(components), std::move(range));
}

InternalRouteResult
TripPlugin::ComputeRoute(const std::shared_ptr<const datafacade::BaseDataFacade> facade,
                         const std::vector<PhantomNode> &snapped_phantoms,
                         const std::vector<NodeID> &trip,
                         const bool roundtrip) const
{
    InternalRouteResult min_route;
    // given the final trip, compute total duration and return the route and location permutation
    PhantomNodes viapoint;
    const auto start = std::begin(trip);
    const auto end = std::end(trip);
    // computes a roundtrip from the nodes in trip
    for (auto it = start; it != end; ++it)
    {
        const auto from_node = *it;

        // if from_node is the last node and it is a fixed start and end trip,
        // break out of this loop and return the route
        if (!roundtrip && std::next(it) == end)
            break;

        // if from_node is the last node, compute the route from the last to the first location
        const auto to_node = std::next(it) != end ? *std::next(it) : *start;

        viapoint = PhantomNodes{snapped_phantoms[from_node], snapped_phantoms[to_node]};
        min_route.segment_end_coordinates.emplace_back(viapoint);
    }

    if (roundtrip)
    {
        // trip comes out to be something like 0 1 4 3 2 0
        // BOOST_ASSERT(min_route.segment_end_coordinates.size() == trip.size());
    }
    else
    {
        // trip comes out to be something like 0 1 4 3 2, so the sizes don't match
        // BOOST_ASSERT(min_route.segment_end_coordinates.size() == trip.size() - 1);
    }

    shortest_path(facade, min_route.segment_end_coordinates, {false}, min_route);

    BOOST_ASSERT_MSG(min_route.shortest_path_length < INVALID_EDGE_WEIGHT, "unroutable route");
    return min_route;
}

Status TripPlugin::HandleRequest(const std::shared_ptr<const datafacade::BaseDataFacade> facade,
                                 const api::TripParameters &parameters,
                                 util::json::Object &json_result) const
{
    BOOST_ASSERT(parameters.IsValid());

    // enforce maximum number of locations for performance reasons
    if (max_locations_trip > 0 &&
        static_cast<int>(parameters.coordinates.size()) > max_locations_trip)
    {
        return Error("TooBig", "Too many trip coordinates", json_result);
    }

    if (!CheckAllCoordinates(parameters.coordinates))
    {
        return Error("InvalidValue", "Invalid coordinate value.", json_result);
    }

    auto phantom_node_pairs = GetPhantomNodes(*facade, parameters);
    if (phantom_node_pairs.size() != parameters.coordinates.size())
    {
        return Error("NoSegment",
                     std::string("Could not find a matching segment for coordinate ") +
                         std::to_string(phantom_node_pairs.size()),
                     json_result);
    }
    BOOST_ASSERT(phantom_node_pairs.size() == parameters.coordinates.size());

    if (parameters.source >= parameters.coordinates.size() ||
        parameters.destination >= parameters.coordinates.size())
    {
        return Error("InvalidValue", "Invalid source or destination value.", json_result);
    }

    bool fixed_start_and_end = false;
    if (parameters.source != parameters.destination)
    {
        fixed_start_and_end = true;
    }

    auto snapped_phantoms = SnapPhantomNodes(phantom_node_pairs);

    const auto number_of_locations = snapped_phantoms.size();

    // compute the duration table of all phantom nodes
    auto result_table = util::DistTableWrapper<EdgeWeight>(
        duration_table(facade, snapped_phantoms, {}, {}), number_of_locations);

    if (result_table.size() == 0)
    {
        return Status::Error;
    }

    const constexpr std::size_t BF_MAX_FEASABLE = 10;
    BOOST_ASSERT_MSG(result_table.size() == number_of_locations * number_of_locations,
                     "Distance Table has wrong size");

    // get scc components
    SCC_Component scc = SplitUnaccessibleLocations(result_table.GetNumberOfNodes(), result_table);

    if (fixed_start_and_end)
    {
        // ****************** Change Table *************************
        // The following code manipulates the table and produces the new table for
        // Trip with Fixed Start and End (TFSE). In the example the source is a
        // and destination is c. The new table forces the roundtrip to start at
        // source and end at destination by virtually squashing them together.
        // This way the brute force and the farthest insertion algorithms don't
        // have to be modified, and instead we can just pass a modified table to
        // return a non-roundtrip "optimal" route from a start node to an end node.

        // Original Table           // New Table
        //   a  b  c  d  e          //   a        b         c        d         e
        // a 0  15 36 34 30         // a 0        15        10000    34        30
        // b 15 0  25 30 34         // b 10000    0         25       30        34
        // c 36 25 0  18 32         // c 0        10000     0        10000     10000
        // d 34 30 18 0  15         // d 10000    30        18       0         15
        // e 30 34 32 15 0          // e 10000    34        32       15        0

        // change parameters.source column
        // set any node to source to impossibly high numbers so it will never
        // try to use any node->source in the middle of the "optimal path"
        for (NodeID i = 0; i < result_table.GetNumberOfNodes(); i++)
        {
            if (i == (NodeID)parameters.source)
                continue;
            result_table.InvalidateRoute(i, parameters.source);
        }

        // change parameters.destination row
        // set destination to anywhere else to impossibly high numbers so it will
        // never try to use destination->any node in the middle of the "optimal path"
        for (NodeID i = 0; i < result_table.GetNumberOfNodes(); i++)
        {
            if (i == (NodeID)parameters.destination)
                continue;
            result_table.ShortcutRoute(parameters.destination, i);
        }

        // set destination->source to zero so rountrip treats source and
        // destination as one location
        result_table.ShortcutRoute(parameters.destination, parameters.source);

        // set source->destination as very high number so algorithm is forced
        // to find another path to get to destination
        result_table.InvalidateRoute(parameters.source, parameters.destination);

        //*********  End of changes to table  *************************************

        //*********  SCC related errors in tfse ***********************************
        // if source and destination are in different sccs then return error
        if (scc.GetNumberOfComponents() > 1)
        {
            std::size_t source_component_id = std::numeric_limits<std::size_t>::max();
            std::size_t destination_component_id = std::numeric_limits<std::size_t>::max();

            for (std::size_t k = 0; k < scc.GetNumberOfComponents(); ++k)
            {
                auto route_begin = std::begin(scc.component) + scc.range[k];
                auto route_end = std::begin(scc.component) + scc.range[k + 1];

                std::for_each(route_begin, route_end, [&](const NodeID &id) {
                    if ((NodeID)parameters.source == id)
                        source_component_id = k;
                    if ((NodeID)parameters.destination == id)
                        destination_component_id = k;
                });
            }

            if (destination_component_id == std::numeric_limits<std::size_t>::max() ||
                source_component_id == std::numeric_limits<std::size_t>::max() ||
                source_component_id != destination_component_id)
            {
                return Error(
                    "NoTrips", "No route possible from source to destination", json_result);
            }
        }
        //*********  End of SCC related errors ************************************
    }

    std::vector<std::vector<NodeID>> trips;
    trips.reserve(scc.GetNumberOfComponents());
    // run Trip computation for every SCC
    for (std::size_t k = 0; k < scc.GetNumberOfComponents(); ++k)
    {
        const auto component_size = scc.range[k + 1] - scc.range[k];

        BOOST_ASSERT_MSG(component_size > 0, "invalid component size");

        std::vector<NodeID> scc_route;
        auto route_begin = std::begin(scc.component) + scc.range[k];
        auto route_end = std::begin(scc.component) + scc.range[k + 1];

        if (component_size > 1)
        {
            if (component_size < BF_MAX_FEASABLE)
            {
                scc_route =
                    trip::BruteForceTrip(route_begin, route_end, number_of_locations, result_table);
            }
            else
            {
                scc_route = trip::FarthestInsertionTrip(
                    route_begin, route_end, number_of_locations, result_table);
            }
        }
        else
        {
            scc_route = std::vector<NodeID>(route_begin, route_end);
        }
        // rotate result such that roundtrip starts at node with index 0
        if (parameters.roundtrip)
        {
            auto start_index = std::find(scc_route.begin(), scc_route.end(), 0);
            // @TODO: Find out whether there is always a 0 == Find out whether we return multiple
            //        trips if there are more than one SCC
            // BOOST_ASSERT(start_index != scc_route.end());
            std::rotate(scc_route.begin(), start_index, scc_route.end());
        }
        trips.push_back(std::move(scc_route));
    }
    if (trips.empty())
    {
        return Error("NoTrips", "Cannot find trips", json_result);
    }

    // compute all round trip routes
    std::vector<InternalRouteResult> routes;
    routes.reserve(trips.size());
    for (const auto &trip : trips)
    {
        routes.push_back(ComputeRoute(facade, snapped_phantoms, trip, parameters.roundtrip));
    }

    api::TripAPI trip_api{*facade, parameters};
    trip_api.MakeResponse(trips, routes, snapped_phantoms, json_result);

    return Status::Ok;
}
}
}
}

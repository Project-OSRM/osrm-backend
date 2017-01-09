#ifndef SHORTEST_PATH_HPP
#define SHORTEST_PATH_HPP

#include "engine/algorithm.hpp"
#include "engine/routing_algorithms/routing_base.hpp"
#include "engine/search_engine_data.hpp"
#include "util/integer_range.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include <memory>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

template <typename AlgorithmT> class ShortestPathRouting;

template <> class ShortestPathRouting<algorithm::CH> final : public BasicRouting<algorithm::CH>
{
    using super = BasicRouting<algorithm::CH>;
    using FacadeT = datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH>;
    using QueryHeap = SearchEngineData::QueryHeap;
    SearchEngineData &engine_working_data;
    const static constexpr bool DO_NOT_FORCE_LOOP = false;

  public:
    ShortestPathRouting(SearchEngineData &engine_working_data)
        : engine_working_data(engine_working_data)
    {
    }

    ~ShortestPathRouting() {}

    // allows a uturn at the target_phantom
    // searches source forward/reverse -> target forward/reverse
    void SearchWithUTurn(const FacadeT &facade,
                         QueryHeap &forward_heap,
                         QueryHeap &reverse_heap,
                         QueryHeap &forward_core_heap,
                         QueryHeap &reverse_core_heap,
                         const bool search_from_forward_node,
                         const bool search_from_reverse_node,
                         const bool search_to_forward_node,
                         const bool search_to_reverse_node,
                         const PhantomNode &source_phantom,
                         const PhantomNode &target_phantom,
                         const int total_weight_to_forward,
                         const int total_weight_to_reverse,
                         int &new_total_weight,
                         std::vector<NodeID> &leg_packed_path) const;

    // searches shortest path between:
    // source forward/reverse -> target forward
    // source forward/reverse -> target reverse
    void Search(const FacadeT &facade,
                QueryHeap &forward_heap,
                QueryHeap &reverse_heap,
                QueryHeap &forward_core_heap,
                QueryHeap &reverse_core_heap,
                const bool search_from_forward_node,
                const bool search_from_reverse_node,
                const bool search_to_forward_node,
                const bool search_to_reverse_node,
                const PhantomNode &source_phantom,
                const PhantomNode &target_phantom,
                const int total_weight_to_forward,
                const int total_weight_to_reverse,
                int &new_total_weight_to_forward,
                int &new_total_weight_to_reverse,
                std::vector<NodeID> &leg_packed_path_forward,
                std::vector<NodeID> &leg_packed_path_reverse) const;

    void UnpackLegs(const FacadeT &facade,
                    const std::vector<PhantomNodes> &phantom_nodes_vector,
                    const std::vector<NodeID> &total_packed_path,
                    const std::vector<std::size_t> &packed_leg_begin,
                    const int shortest_path_length,
                    InternalRouteResult &raw_route_data) const;

    void operator()(const FacadeT &facade,
                    const std::vector<PhantomNodes> &phantom_nodes_vector,
                    const boost::optional<bool> continue_straight_at_waypoint,
                    InternalRouteResult &raw_route_data) const;
};
} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif /* SHORTEST_PATH_HPP */

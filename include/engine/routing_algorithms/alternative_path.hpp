#ifndef ALTERNATIVE_PATH_ROUTING_HPP
#define ALTERNATIVE_PATH_ROUTING_HPP

#include "engine/datafacade/datafacade_base.hpp"
#include "engine/routing_algorithms/routing_base.hpp"

#include "engine/algorithm.hpp"
#include "engine/search_engine_data.hpp"
#include "util/integer_range.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <iterator>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <vector>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

const double constexpr VIAPATH_ALPHA = 0.10;
const double constexpr VIAPATH_EPSILON = 0.15; // alternative at most 15% longer
const double constexpr VIAPATH_GAMMA = 0.75;   // alternative shares at most 75% with the shortest.

template <typename AlgorithmT> class AlternativeRouting;

template <> class AlternativeRouting<algorithm::CH> final : private BasicRouting<algorithm::CH>
{
    using super = BasicRouting<algorithm::CH>;
    using FacadeT = datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH>;
    using QueryHeap = SearchEngineData::QueryHeap;
    using SearchSpaceEdge = std::pair<NodeID, NodeID>;

    struct RankedCandidateNode
    {
        RankedCandidateNode(const NodeID node, const int length, const int sharing)
            : node(node), length(length), sharing(sharing)
        {
        }

        NodeID node;
        int length;
        int sharing;

        bool operator<(const RankedCandidateNode &other) const
        {
            return (2 * length + sharing) < (2 * other.length + other.sharing);
        }
    };
    SearchEngineData &engine_working_data;

  public:
    AlternativeRouting(SearchEngineData &engine_working_data)
        : engine_working_data(engine_working_data)
    {
    }

    virtual ~AlternativeRouting() {}

    void operator()(const FacadeT &facade,
                    const PhantomNodes &phantom_node_pair,
                    InternalRouteResult &raw_route_data);

  private:
    // unpack alternate <s,..,v,..,t> by exploring search spaces from v
    void RetrievePackedAlternatePath(const QueryHeap &forward_heap1,
                                     const QueryHeap &reverse_heap1,
                                     const QueryHeap &forward_heap2,
                                     const QueryHeap &reverse_heap2,
                                     const NodeID s_v_middle,
                                     const NodeID v_t_middle,
                                     std::vector<NodeID> &packed_path) const;

    // TODO: reorder parameters
    // compute and unpack <s,..,v> and <v,..,t> by exploring search spaces
    // from v and intersecting against queues. only half-searches have to be
    // done at this stage
    void ComputeLengthAndSharingOfViaPath(const FacadeT &facade,
                                          const NodeID via_node,
                                          int *real_length_of_via_path,
                                          int *sharing_of_via_path,
                                          const std::vector<NodeID> &packed_shortest_path,
                                          const EdgeWeight min_edge_offset);

    // todo: reorder parameters
    template <bool is_forward_directed>
    void AlternativeRoutingStep(const FacadeT &facade,
                                QueryHeap &heap1,
                                QueryHeap &heap2,
                                NodeID *middle_node,
                                EdgeWeight *upper_bound_to_shortest_path_weight,
                                std::vector<NodeID> &search_space_intersection,
                                std::vector<SearchSpaceEdge> &search_space,
                                const EdgeWeight min_edge_offset) const
    {
        QueryHeap &forward_heap = (is_forward_directed ? heap1 : heap2);
        QueryHeap &reverse_heap = (is_forward_directed ? heap2 : heap1);

        const NodeID node = forward_heap.DeleteMin();
        const EdgeWeight weight = forward_heap.GetKey(node);
        // const NodeID parentnode = forward_heap.GetData(node).parent;
        // util::Log() << (is_forward_directed ? "[fwd] " : "[rev] ") << "settled
        // edge ("
        // << parentnode << "," << node << "), dist: " << weight;

        const auto scaled_weight =
            static_cast<EdgeWeight>((weight + min_edge_offset) / (1. + VIAPATH_EPSILON));
        if ((INVALID_EDGE_WEIGHT != *upper_bound_to_shortest_path_weight) &&
            (scaled_weight > *upper_bound_to_shortest_path_weight))
        {
            forward_heap.DeleteAll();
            return;
        }

        search_space.emplace_back(forward_heap.GetData(node).parent, node);

        if (reverse_heap.WasInserted(node))
        {
            search_space_intersection.emplace_back(node);
            const EdgeWeight new_weight = reverse_heap.GetKey(node) + weight;
            if (new_weight < *upper_bound_to_shortest_path_weight)
            {
                if (new_weight >= 0)
                {
                    *middle_node = node;
                    *upper_bound_to_shortest_path_weight = new_weight;
                    //     util::Log() << "accepted middle_node " << *middle_node
                    //     << " at
                    //     weight " << new_weight;
                    // } else {
                    //     util::Log() << "discarded middle_node " << *middle_node
                    //     << "
                    //     at weight " << new_weight;
                }
                else
                {
                    // check whether there is a loop present at the node
                    const auto loop_weight = super::GetLoopWeight<false>(facade, node);
                    const EdgeWeight new_weight_with_loop = new_weight + loop_weight;
                    if (loop_weight != INVALID_EDGE_WEIGHT &&
                        new_weight_with_loop <= *upper_bound_to_shortest_path_weight)
                    {
                        *middle_node = node;
                        *upper_bound_to_shortest_path_weight = loop_weight;
                    }
                }
            }
        }

        for (auto edge : facade.GetAdjacentEdgeRange(node))
        {
            const auto &data = facade.GetEdgeData(edge);
            const bool edge_is_forward_directed =
                (is_forward_directed ? data.forward : data.backward);
            if (edge_is_forward_directed)
            {
                const NodeID to = facade.GetTarget(edge);
                const EdgeWeight edge_weight = data.weight;

                BOOST_ASSERT(edge_weight > 0);
                const EdgeWeight to_weight = weight + edge_weight;

                // New Node discovered -> Add to Heap + Node Info Storage
                if (!forward_heap.WasInserted(to))
                {
                    forward_heap.Insert(to, to_weight, node);
                }
                // Found a shorter Path -> Update weight
                else if (to_weight < forward_heap.GetKey(to))
                {
                    // new parent
                    forward_heap.GetData(to).parent = node;
                    // decreased weight
                    forward_heap.DecreaseKey(to, to_weight);
                }
            }
        }
    }

    // conduct T-Test
    bool ViaNodeCandidatePassesTTest(const FacadeT &facade,
                                     QueryHeap &existing_forward_heap,
                                     QueryHeap &existing_reverse_heap,
                                     QueryHeap &new_forward_heap,
                                     QueryHeap &new_reverse_heap,
                                     const RankedCandidateNode &candidate,
                                     const int length_of_shortest_path,
                                     int *length_of_via_path,
                                     NodeID *s_v_middle,
                                     NodeID *v_t_middle,
                                     const EdgeWeight min_edge_offset) const;
};

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif /* ALTERNATIVE_PATH_ROUTING_HPP */

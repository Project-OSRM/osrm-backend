#ifndef OSRM_ENGINE_DATAFACADE_ALGORITHM_DATAFACADE_HPP
#define OSRM_ENGINE_DATAFACADE_ALGORITHM_DATAFACADE_HPP

#include "contractor/query_edge.hpp"
#include "customizer/edge_based_graph.hpp"
#include "extractor/edge_based_edge.hpp"
#include "engine/algorithm.hpp"

#include "partitioner/cell_storage.hpp"
#include "partitioner/multi_level_partition.hpp"

#include "util/filtered_graph.hpp"
#include "util/integer_range.hpp"

namespace osrm::engine::datafacade
{

// Namespace local aliases for algorithms
using CH = routing_algorithms::ch::Algorithm;
using MLD = routing_algorithms::mld::Algorithm;

template <typename AlgorithmT> class AlgorithmDataFacade;

template <> class AlgorithmDataFacade<CH>
{
  public:
    using EdgeData = contractor::QueryEdge::EdgeData;
    using EdgeRange = util::filtered_range<EdgeID, util::vector_view<bool>>;

    virtual ~AlgorithmDataFacade() = default;

    // search graph access
    virtual unsigned GetNumberOfNodes() const = 0;

    virtual unsigned GetNumberOfEdges() const = 0;

    virtual unsigned GetOutDegree(const NodeID edge_based_node_id) const = 0;

    virtual NodeID GetTarget(const EdgeID edge_based_edge_id) const = 0;

    virtual const EdgeData &GetEdgeData(const EdgeID edge_based_edge_id) const = 0;

    virtual EdgeRange GetAdjacentEdgeRange(const NodeID edge_based_node_id) const = 0;

    // searches for a specific edge
    virtual EdgeID FindEdge(const NodeID edge_based_node_from,
                            const NodeID edge_based_node_to) const = 0;

    virtual EdgeID FindEdgeInEitherDirection(const NodeID edge_based_node_from,
                                             const NodeID edge_based_node_to) const = 0;

    virtual EdgeID FindEdgeIndicateIfReverse(const NodeID edge_based_node_from,
                                             const NodeID edge_based_node_to,
                                             bool &result) const = 0;

    virtual EdgeID FindSmallestEdge(const NodeID edge_based_node_from,
                                    const NodeID edge_based_node_to,
                                    const std::function<bool(const EdgeData &)> &filter) const = 0;
};

template <> class AlgorithmDataFacade<MLD>
{
  public:
    using EdgeData = customizer::EdgeBasedGraphEdgeData;
    using EdgeRange = util::range<EdgeID>;

    virtual ~AlgorithmDataFacade() = default;

    // search graph access
    virtual unsigned GetNumberOfNodes() const = 0;

    virtual unsigned GetMaxBorderNodeID() const = 0;

    virtual unsigned GetNumberOfEdges() const = 0;

    virtual unsigned GetOutDegree(const NodeID edge_based_node_id) const = 0;

    virtual EdgeRange GetAdjacentEdgeRange(const NodeID edge_based_node_id) const = 0;

    virtual EdgeWeight GetNodeWeight(const NodeID edge_based_node_id) const = 0;

    virtual EdgeDuration
    GetNodeDuration(const NodeID edge_based_node_id) const = 0; // TODO: to be removed

    virtual EdgeDistance GetNodeDistance(const NodeID edge_based_node_id) const = 0;

    virtual bool IsForwardEdge(EdgeID edge_based_edge_id) const = 0;

    virtual bool IsBackwardEdge(EdgeID edge_based_edge_id) const = 0;

    virtual NodeID GetTarget(const EdgeID edge_based_edge_id) const = 0;

    virtual const EdgeData &GetEdgeData(const EdgeID edge_based_edge_id) const = 0;

    virtual const partitioner::MultiLevelPartitionView &GetMultiLevelPartition() const = 0;

    virtual const partitioner::CellStorageView &GetCellStorage() const = 0;

    virtual const customizer::CellMetricView &GetCellMetric() const = 0;

    virtual EdgeRange GetBorderEdgeRange(const LevelID level,
                                         const NodeID edge_based_node_id) const = 0;

    // searches for a specific edge
    virtual EdgeID FindEdge(const NodeID edge_based_node_from,
                            const NodeID edge_based_node_to) const = 0;
};
} // namespace osrm::engine::datafacade

#endif

#ifndef OSRM_ENGINE_DATAFACADE_ALGORITHM_DATAFACADE_HPP
#define OSRM_ENGINE_DATAFACADE_ALGORITHM_DATAFACADE_HPP

#include "contractor/query_edge.hpp"
#include "extractor/edge_based_edge.hpp"
#include "engine/algorithm.hpp"

#include "partition/cell_storage.hpp"
#include "partition/multi_level_partition.hpp"

#include "util/filtered_graph.hpp"
#include "util/integer_range.hpp"

namespace osrm
{
namespace engine
{
namespace datafacade
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

    // search graph access
    virtual unsigned GetNumberOfNodes() const = 0;

    virtual unsigned GetNumberOfEdges() const = 0;

    virtual unsigned GetOutDegree(const NodeID n) const = 0;

    virtual NodeID GetTarget(const EdgeID e) const = 0;

    virtual const EdgeData &GetEdgeData(const EdgeID e) const = 0;

    virtual EdgeRange GetAdjacentEdgeRange(const NodeID node) const = 0;

    // searches for a specific edge
    virtual EdgeID FindEdge(const NodeID from, const NodeID to) const = 0;

    virtual EdgeID FindEdgeInEitherDirection(const NodeID from, const NodeID to) const = 0;

    virtual EdgeID
    FindEdgeIndicateIfReverse(const NodeID from, const NodeID to, bool &result) const = 0;

    virtual EdgeID FindSmallestEdge(const NodeID from,
                                    const NodeID to,
                                    const std::function<bool(EdgeData)> filter) const = 0;
};

template <> class AlgorithmDataFacade<MLD>
{
  public:
    using EdgeData = extractor::EdgeBasedEdge::EdgeData;
    using EdgeRange = util::range<EdgeID>;

    // search graph access
    virtual unsigned GetNumberOfNodes() const = 0;

    virtual unsigned GetNumberOfEdges() const = 0;

    virtual unsigned GetOutDegree(const NodeID n) const = 0;

    virtual NodeID GetTarget(const EdgeID e) const = 0;

    virtual const EdgeData &GetEdgeData(const EdgeID e) const = 0;

    virtual EdgeRange GetAdjacentEdgeRange(const NodeID node) const = 0;

    virtual const partition::MultiLevelPartitionView &GetMultiLevelPartition() const = 0;

    virtual const partition::CellStorageView &GetCellStorage() const = 0;

    virtual const customizer::CellMetricView &GetCellMetric() const = 0;

    virtual EdgeRange GetBorderEdgeRange(const LevelID level, const NodeID node) const = 0;

    // searches for a specific edge
    virtual EdgeID FindEdge(const NodeID from, const NodeID to) const = 0;
};
}
}
}

#endif

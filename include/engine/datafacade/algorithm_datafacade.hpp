#ifndef OSRM_ENGINE_DATAFACADE_ALGORITHM_DATAFACADE_HPP
#define OSRM_ENGINE_DATAFACADE_ALGORITHM_DATAFACADE_HPP

#include "contractor/query_edge.hpp"
#include "engine/algorithm.hpp"
#include "util/integer_range.hpp"

namespace osrm
{
namespace engine
{
namespace datafacade
{

using EdgeRange = util::range<EdgeID>;

template <typename AlgorithmT> class AlgorithmDataFacade;

template <> class AlgorithmDataFacade<algorithm::CH>
{
  public:
    using EdgeData = contractor::QueryEdge::EdgeData;

    // search graph access
    virtual unsigned GetNumberOfNodes() const = 0;

    virtual unsigned GetNumberOfEdges() const = 0;

    virtual unsigned GetOutDegree(const NodeID n) const = 0;

    virtual NodeID GetTarget(const EdgeID e) const = 0;

    virtual const EdgeData &GetEdgeData(const EdgeID e) const = 0;

    virtual EdgeID BeginEdges(const NodeID n) const = 0;

    virtual EdgeID EndEdges(const NodeID n) const = 0;

    virtual EdgeRange GetAdjacentEdgeRange(const NodeID node) const = 0;

    // searches for a specific edge
    virtual EdgeID FindEdge(const NodeID from, const NodeID to) const = 0;

    virtual EdgeID FindEdgeInEitherDirection(const NodeID from, const NodeID to) const = 0;

    virtual EdgeID
    FindEdgeIndicateIfReverse(const NodeID from, const NodeID to, bool &result) const = 0;

    virtual EdgeID FindSmallestEdge(const NodeID from,
                                    const NodeID to,
                                    const std::function<bool(EdgeData)> filter) const = 0;

    virtual bool IsCoreNode(const NodeID id) const = 0;

    virtual std::size_t GetCoreSize() const = 0;
};
}
}
}

#endif

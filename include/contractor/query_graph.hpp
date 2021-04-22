#ifndef OSRM_CONTRACTOR_QUERY_GRAPH_HPP
#define OSRM_CONTRACTOR_QUERY_GRAPH_HPP

#include "contractor/query_edge.hpp"

#include "util/static_graph.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace contractor
{

namespace detail
{
template <storage::Ownership Ownership>
using QueryGraph = util::StaticGraph<typename QueryEdge::EdgeData, Ownership>;
}

using QueryGraph = detail::QueryGraph<storage::Ownership::Container>;
using QueryGraphView = detail::QueryGraph<storage::Ownership::View>;
} // namespace contractor
} // namespace osrm

#endif // QUERYEDGE_HPP

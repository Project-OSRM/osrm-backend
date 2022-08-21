#ifndef OSMR_CONTRACTOR_CONTRACTED_METRIC_HPP
#define OSMR_CONTRACTOR_CONTRACTED_METRIC_HPP

#include "contractor/query_graph.hpp"

namespace osrm
{
namespace contractor
{

namespace detail
{
template <storage::Ownership Ownership> struct ContractedMetric
{
    detail::QueryGraph<Ownership> graph;
    std::vector<util::ViewOrVector<bool, Ownership>> edge_filter;
};
} // namespace detail

using ContractedMetric = detail::ContractedMetric<storage::Ownership::Container>;
using ContractedMetricView = detail::ContractedMetric<storage::Ownership::View>;
} // namespace contractor
} // namespace osrm

#endif

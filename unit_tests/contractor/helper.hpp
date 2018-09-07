#ifndef OSRM_UNIT_TESTS_CONTRACTOR_HELPER_HPP_
#define OSRM_UNIT_TESTS_CONTRACTOR_HELPER_HPP_

#include "contractor/contractor_graph.hpp"

namespace osrm
{
namespace unit_test
{

using TestEdge = std::tuple<unsigned, unsigned, int>;
inline contractor::ContractorGraph makeGraph(const std::vector<TestEdge> &edges)
{
    std::vector<contractor::ContractorEdge> input_edges;
    auto id = 0u;
    auto max_id = 0u;
    for (const auto &edge : edges)
    {
        unsigned start;
        unsigned target;
        int weight;
        std::tie(start, target, weight) = edge;
        int duration = weight * 2;
        float distance = 1.0;
        max_id = std::max(std::max(start, target), max_id);
        input_edges.push_back(contractor::ContractorEdge{
            start,
            target,
            contractor::ContractorEdgeData{
                weight, duration, distance, id++, 0, false, true, false}});
        input_edges.push_back(contractor::ContractorEdge{
            target,
            start,
            contractor::ContractorEdgeData{
                weight, duration, distance, id++, 0, false, false, true}});
    }
    std::sort(input_edges.begin(), input_edges.end());

    return contractor::ContractorGraph{max_id + 1, std::move(input_edges)};
}
}
}

#endif

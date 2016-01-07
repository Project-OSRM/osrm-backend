#ifndef CONTRACTOR_CONTRACTOR_HPP
#define CONTRACTOR_CONTRACTOR_HPP

#include "extractor/edge_based_edge.hpp"
#include "extractor/node_based_edge.hpp"
#include "contractor/contractor.hpp"
#include "contractor/contractor_config.hpp"
#include "contractor/query_edge.hpp"
#include "extractor/edge_based_edge.hpp"
#include "util/static_graph.hpp"
#include "util/deallocating_vector.hpp"
#include "util/node_based_graph.hpp"

#include <boost/filesystem.hpp>

#include <vector>

struct lua_State;

namespace osrm
{
namespace extractor
{
struct SpeedProfileProperties;
struct EdgeBasedNode;
struct EdgeBasedEdge;
}
namespace contractor
{

/// Base class of osrm-contract
class Contractor
{
  public:
    using EdgeData = QueryEdge::EdgeData;

    explicit Contractor(ContractorConfig contractor_config) : config(std::move(contractor_config))
    {
    }
    Contractor(const Contractor &) = delete;
    Contractor& operator=(const Contractor &) = delete;

    int Run();

  protected:
    void ContractGraph(const unsigned max_edge_id,
                       util::DeallocatingVector<extractor::EdgeBasedEdge> &edge_based_edge_list,
                       util::DeallocatingVector<QueryEdge> &contracted_edge_list,
                       std::vector<EdgeWeight> &&node_weights,
                       std::vector<bool> &is_core_node,
                       std::vector<float> &inout_node_levels) const;
    void WriteCoreNodeMarker(std::vector<bool> &&is_core_node) const;
    void WriteNodeLevels(std::vector<float> &&node_levels) const;
    void ReadNodeLevels(std::vector<float> &contraction_order) const;
    std::size_t
    WriteContractedGraph(unsigned number_of_edge_based_nodes,
                         const util::DeallocatingVector<QueryEdge> &contracted_edge_list);
    void FindComponents(unsigned max_edge_id,
                        const util::DeallocatingVector<extractor::EdgeBasedEdge> &edges,
                        std::vector<extractor::EdgeBasedNode> &nodes) const;

  private:
    ContractorConfig config;
    std::size_t
    LoadEdgeExpandedGraph(const std::string &edge_based_graph_path,
                          util::DeallocatingVector<extractor::EdgeBasedEdge> &edge_based_edge_list,
                          const std::string &edge_segment_lookup_path,
                          const std::string &edge_penalty_path,
                          const std::string &segment_speed_path);
};
}
}

#endif // PROCESSING_CHAIN_HPP

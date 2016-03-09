#ifndef CONTRACTOR_CONTRACTOR_HPP
#define CONTRACTOR_CONTRACTOR_HPP

#include "contractor/contractor_config.hpp"
#include "contractor/query_edge.hpp"
#include "extractor/edge_based_edge.hpp"
#include "extractor/edge_based_node.hpp"
#include "util/typedefs.hpp"
#include "util/deallocating_vector.hpp"

#include <vector>
#include <string>

#include <cstddef>

namespace osrm
{
namespace contractor
{

/// Base class of osrm-contract
class Contractor
{
  public:
    using EdgeData = QueryEdge::EdgeData;

    explicit Contractor(const ContractorConfig &config_) : config{config_} {}

    Contractor(const Contractor &) = delete;
    Contractor &operator=(const Contractor &) = delete;

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
                          const std::string &segment_speed_path,
                          const std::string &nodes_filename,
                          const std::string &geometry_filename,
                          const std::string &rtree_leaf_filename);
};
}
}

#endif // PROCESSING_CHAIN_HPP

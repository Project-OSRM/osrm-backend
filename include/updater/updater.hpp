#ifndef OSRM_UPDATER_UPDATER_HPP
#define OSRM_UPDATER_UPDATER_HPP

#include "updater/updater_config.hpp"

#include "extractor/edge_based_edge.hpp"
#include "extractor/isochrone_transition.hpp"

#include <vector>

namespace osrm::updater
{
struct TurnPenaltyMetrics
{
    std::vector<TurnPenalty> weight_penalties;
    std::vector<TurnPenalty> duration_penalties;
};

// The normal edge-expanded graph can lose transitions while coalescing parallel edges during
// partitioning.  Isochrone preprocessing needs the original transition sources as well as the
// final, in-memory turn penalties used for this update.
struct EdgeExpandedGraphUpdateOptions
{
    const std::vector<extractor::IsochroneTransition> *isochrone_transitions = nullptr;
    TurnPenaltyMetrics *turn_penalties = nullptr;
    // Generate raw transitions from the edge-based graph after it is loaded but before updates
    // can remove or invalidate its entries. This is needed for an unpartitioned CH dataset,
    // where there is no sidecar carrying the original transitions.
    std::vector<extractor::IsochroneTransition> *generated_isochrone_transitions = nullptr;
    // Traffic and turn updates enforce a lower bound on the total duration of each updated
    // transition.  Keep that build-only information separate from node_durations: storing the
    // lower bound as a node duration would incorrectly add it to positive turn penalties.
    std::vector<EdgeDuration> *node_duration_lower_bounds = nullptr;
};

class Updater
{
  public:
    Updater(UpdaterConfig config_) : config(std::move(config_)) {}

    EdgeID
    LoadAndUpdateEdgeExpandedGraph(std::vector<extractor::EdgeBasedEdge> &edge_based_edge_list,
                                   std::vector<EdgeWeight> &node_weights,
                                   std::uint32_t &connectivity_checksum) const;

    EdgeID LoadAndUpdateEdgeExpandedGraph(
        std::vector<extractor::EdgeBasedEdge> &edge_based_edge_list,
        std::vector<EdgeWeight> &node_weights,
        std::vector<EdgeDuration> &node_durations, // TODO: remove when optional
        std::uint32_t &connectivity_checksum,
        const EdgeExpandedGraphUpdateOptions &options = {}) const;
    EdgeID LoadAndUpdateEdgeExpandedGraph(
        std::vector<extractor::EdgeBasedEdge> &edge_based_edge_list,
        std::vector<EdgeWeight> &node_weights,
        std::vector<EdgeDuration> &node_durations, // TODO: remove when optional
        std::vector<EdgeDistance> &node_distances, // TODO: remove when optional
        std::uint32_t &connectivity_checksum) const;

  private:
    UpdaterConfig config;
};
} // namespace osrm::updater

#endif

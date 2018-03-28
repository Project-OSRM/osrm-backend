#ifndef OSRM_GUIDANCE_STATISTICS_HANDLER_HPP_
#define OSRM_GUIDANCE_STATISTICS_HANDLER_HPP_

#include "guidance/intersection.hpp"
#include "guidance/intersection_handler.hpp"
#include "guidance/turn_instruction.hpp"

#include "util/log.hpp"

#include <algorithm>
#include <iomanip>
#include <iterator>
#include <map>
#include <mutex>

#include <cstdint>

namespace osrm
{
namespace guidance
{

// Unconditionally runs over all intersections and gathers statistics for
// instruction turn types and direction modifiers (see turn_instruction.hpp).
class StatisticsHandler final : public IntersectionHandler
{
  public:
    StatisticsHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                      const extractor::EdgeBasedNodeDataContainer &node_data_container,
                      const std::vector<util::Coordinate> &coordinates,
                      const extractor::CompressedEdgeContainer &compressed_geometries,
                      const extractor::RestrictionMap &node_restriction_map,
                      const std::unordered_set<NodeID> &barrier_nodes,
                      const extractor::TurnLanesIndexedArray &turn_lanes_data,
                      const extractor::NameTable &name_table,
                      const extractor::SuffixTable &street_name_suffix_table)
        : IntersectionHandler(node_based_graph,
                              node_data_container,
                              coordinates,
                              compressed_geometries,
                              node_restriction_map,
                              barrier_nodes,
                              turn_lanes_data,
                              name_table,
                              street_name_suffix_table)
    {
    }

    ~StatisticsHandler() override final
    {
        const auto add_second = [](const auto acc, const auto &kv) { return acc + kv.second; };

        const auto num_types =
            std::accumulate(begin(type_hist), end(type_hist), std::uint64_t{0}, add_second);
        const auto num_modifiers =
            std::accumulate(begin(modifier_hist), end(modifier_hist), std::uint64_t{0}, add_second);

        util::Log() << "Assigned " << num_types << " turn instruction types:";

        for (const auto &kv : type_hist)
            if (kv.second > 0)
                util::Log() << "  " << std::fixed << std::setprecision(2)
                            << internalInstructionTypeToString(kv.first) << ": " << kv.second
                            << " (" << (kv.second / static_cast<float>(num_types) * 100.) << "%)";

        util::Log() << "Assigned " << num_modifiers << " turn instruction modifiers:";

        for (const auto &kv : modifier_hist)
            if (kv.second > 0)
                util::Log() << "  " << std::fixed << std::setprecision(2)
                            << instructionModifierToString(kv.first) << ": " << kv.second << " ("
                            << (kv.second / static_cast<float>(num_modifiers) * 100.) << "%)";
    }

    bool canProcess(const NodeID, const EdgeID, const Intersection &) const override final
    {
        return true;
    }

    Intersection
    operator()(const NodeID, const EdgeID, Intersection intersection) const override final
    {
        // Lock histograms updates on a per-intersection basis.
        std::lock_guard<std::mutex> defer{lock};

        // Generate histograms for all roads; this way we will get duplicates
        // which we would not get doing it after EBF generation. But we want
        // numbers closer to the handlers and see how often handlers ran.
        for (const auto &road : intersection)
        {
            if (road.entry_allowed)
            {
                const auto type = road.instruction.type;
                const auto modifier = road.instruction.direction_modifier;

                type_hist[type] += 1;
                modifier_hist[modifier] += 1;
            }
        }

        return intersection;
    }

  private:
    mutable std::mutex lock;
    mutable std::map<TurnType::Enum, std::uint64_t> type_hist;
    mutable std::map<DirectionModifier::Enum, std::uint64_t> modifier_hist;
};

} // namespace guidance
} // namespace osrm

#endif // OSRM_GUIDANCE_VALIDATION_HANDLER_HPP_

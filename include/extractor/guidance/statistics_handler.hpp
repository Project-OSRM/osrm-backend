#ifndef OSRM_EXTRACTOR_GUIDANCE_STATISTICS_HANDLER_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_STATISTICS_HANDLER_HPP_

#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/intersection_handler.hpp"
#include "extractor/guidance/turn_instruction.hpp"

#include "util/log.hpp"

#include <mutex>
#include <unordered_map>

#include <cstdint>

namespace osrm
{
namespace extractor
{
namespace guidance
{

// Unconditionally runs over all intersections and gathers statistics for
// instruction turn types and direction modifiers (see turn_instruction.hpp).
class StatisticsHandler final : public IntersectionHandler
{
  public:
    StatisticsHandler(const IntersectionGenerator &intersection_generator,
                      const util::NodeBasedDynamicGraph &node_based_graph,
                      const EdgeBasedNodeDataContainer &node_data_container,
                      const std::vector<util::Coordinate> &coordinates,
                      const util::NameTable &name_table,
                      const SuffixTable &street_name_suffix_table)
        : IntersectionHandler(node_based_graph,
                              node_data_container,
                              coordinates,
                              name_table,
                              street_name_suffix_table,
                              intersection_generator)
    {
    }

    ~StatisticsHandler() override final
    {
        // Todo: type and modifier to string

        util::Log() << "Assigned turn instruction types";

        for (const auto &kv : type_hist)
            util::Log() << internalInstructionTypeToString(kv.first) << ": " << kv.second;

        util::Log() << "Assigned turn instruction modifiers";

        for (const auto &kv : modifier_hist)
            util::Log() << instructionModifierToString(kv.first) << ": " << kv.second;
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

            const auto type = road.instruction.type;
            const auto modifier = road.instruction.direction_modifier;

            type_hist[type] += 1;
            modifier_hist[modifier] += 1;
        }

        return intersection;
    }

  private:
    mutable std::mutex lock;
    mutable std::unordered_map<TurnType::Enum, std::uint64_t> type_hist;
    mutable std::unordered_map<DirectionModifier::Enum, std::uint64_t> modifier_hist;
};

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_GUIDANCE_VALIDATION_HANDLER_HPP_

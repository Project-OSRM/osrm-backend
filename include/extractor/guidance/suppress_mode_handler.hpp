#ifndef OSRM_EXTRACTOR_GUIDANCE_SUPPRESS_MODE_HANDLER_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_SUPPRESS_MODE_HANDLER_HPP_

#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/intersection_generator.hpp"
#include "extractor/guidance/intersection_handler.hpp"
#include "extractor/travel_mode.hpp"
#include "util/node_based_graph.hpp"

namespace osrm
{
namespace extractor
{
namespace guidance
{

class SuppressModeHandler final : public IntersectionHandler
{
  public:
    SuppressModeHandler(const IntersectionGenerator &intersection_generator,
                        const util::NodeBasedDynamicGraph &node_based_graph,
                        const std::vector<QueryNode> &node_info_list,
                        const util::NameTable &name_table,
                        const SuffixTable &street_name_suffix_table);

    ~SuppressModeHandler() override final = default;

    bool canProcess(const NodeID /*nid*/,
                    const EdgeID via_eid,
                    const Intersection &intersection) const override final;

    Intersection operator()(const NodeID nid,
                            const EdgeID via_eid,
                            Intersection intersection) const override final;

    using SuppressModeListT = std::array<osrm::extractor::TravelMode, 2>;
};

} // namespace osrm
} // namespace extractor
} // namespace guidance

#endif /* OSRM_EXTRACTOR_GUIDANCE_SUPPRESS_MODE_HANDLER_HPP_ */

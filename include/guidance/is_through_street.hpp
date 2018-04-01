#ifndef OSRM_GUIDANCE_IS_THROUGH_STREET_HPP_
#define OSRM_GUIDANCE_IS_THROUGH_STREET_HPP_

#include "guidance/constants.hpp"

#include "extractor/intersection/have_identical_names.hpp"
#include "extractor/node_data_container.hpp"
#include "extractor/suffix_table.hpp"

#include "util/bearing.hpp"
#include "util/node_based_graph.hpp"

#include "util/guidance/name_announcements.hpp"

namespace osrm
{
namespace guidance
{

template <typename IntersectionType>
inline bool isThroughStreet(const std::size_t index,
                            const IntersectionType &intersection,
                            const util::NodeBasedDynamicGraph &node_based_graph,
                            const extractor::EdgeBasedNodeDataContainer &node_data_container,
                            const extractor::NameTable &name_table,
                            const extractor::SuffixTable &street_name_suffix_table)
{
    using osrm::util::angularDeviation;

    const auto &data_at_index = node_data_container.GetAnnotation(
        node_based_graph.GetEdgeData(intersection[index].eid).annotation_data);

    if (data_at_index.name_id == EMPTY_NAMEID)
        return false;

    // a through street cannot start at our own position -> index 1
    for (std::size_t road_index = 1; road_index < intersection.size(); ++road_index)
    {
        if (road_index == index)
            continue;

        const auto &road = intersection[road_index];
        const auto &road_data = node_data_container.GetAnnotation(
            node_based_graph.GetEdgeData(road.eid).annotation_data);

        // roads have a near straight angle (180 degree)
        const bool is_nearly_straight = angularDeviation(road.angle, intersection[index].angle) >
                                        (STRAIGHT_ANGLE - FUZZY_ANGLE_DIFFERENCE);

        const bool have_same_name = extractor::intersection::HaveIdenticalNames(
            data_at_index.name_id, road_data.name_id, name_table, street_name_suffix_table);

        const bool have_same_category =
            node_based_graph.GetEdgeData(intersection[index].eid).flags.road_classification ==
            node_based_graph.GetEdgeData(road.eid).flags.road_classification;

        if (is_nearly_straight && have_same_name && have_same_category)
            return true;
    }
    return false;
}

} // namespace guidance
} // namespace osrm

#endif /*OSRM_GUIDANCE_IS_THROUGH_STREET_HPP_*/

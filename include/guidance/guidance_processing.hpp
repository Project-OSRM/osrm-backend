#ifndef OSRM_GUIDANCE_GUIDANCE_RUNNER_HPP
#define OSRM_GUIDANCE_GUIDANCE_RUNNER_HPP

#include "guidance/turn_data_container.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/name_table.hpp"
#include "extractor/node_data_container.hpp"
#include "extractor/node_restriction_map.hpp"
#include "extractor/suffix_table.hpp"
#include "extractor/turn_lane_types.hpp"
#include "extractor/way_restriction_map.hpp"

#include "util/coordinate.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/guidance/turn_lanes.hpp"
#include "util/node_based_graph.hpp"

#include <unordered_set>

namespace osrm::guidance
{
using BearingClassesVector = std::vector<BearingClassID>;
using BearingClassesMap = util::ConcurrentIDMap<util::guidance::BearingClass, BearingClassID>;
using EntryClassesMap = util::ConcurrentIDMap<util::guidance::EntryClass, EntryClassID>;

void annotateTurns(const util::NodeBasedDynamicGraph &node_based_graph,
                   const extractor::EdgeBasedNodeDataContainer &edge_based_node_container,
                   const std::vector<util::Coordinate> &node_coordinates,
                   const extractor::CompressedEdgeContainer &compressed_edge_container,
                   const std::unordered_set<NodeID> &barrier_nodes,
                   const extractor::RestrictionMap &node_restriction_map,
                   const extractor::WayRestrictionMap &way_restriction_map,
                   const extractor::NameTable &name_table,
                   const extractor::SuffixTable &suffix_table,
                   const extractor::TurnLanesIndexedArray &turn_lanes_data,
                   extractor::LaneDescriptionMap &lane_description_map,
                   util::guidance::LaneDataIdMap &lane_data_map,
                   guidance::TurnDataExternalContainer &turn_data_container,
                   BearingClassesVector &bearing_class_by_node_based_node,
                   BearingClassesMap &bearing_class_hash,
                   EntryClassesMap &entry_class_hash,
                   std::uint32_t &connectivity_checksum);

} // namespace osrm::guidance

#endif

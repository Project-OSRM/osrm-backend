#ifndef OSRM_EXTRACTION_SEGMENT_HPP
#define OSRM_EXTRACTION_SEGMENT_HPP

#include <extractor/node_based_edge.hpp>
#include <util/coordinate.hpp>

namespace osrm::extractor
{

struct ExtractionSegment
{
    ExtractionSegment(const osrm::util::Coordinate source_,
                      const osrm::util::Coordinate target_,
                      double distance_,
                      double weight_,
                      double duration_,
                      const NodeBasedEdgeClassification flags_,
                      const OSMNodeID osm_source_id_,
                      const OSMNodeID osm_target_id_)
        : source(source_), target(target_), distance(distance_), weight(weight_),
          duration(duration_), flags(flags_), osm_source_id(osm_source_id_),
          osm_target_id(osm_target_id_)
    {
    }

    const osrm::util::Coordinate source;
    const osrm::util::Coordinate target;
    const double distance;
    double weight;
    double duration;
    const NodeBasedEdgeClassification flags;
    const OSMNodeID osm_source_id;
    const OSMNodeID osm_target_id;
};
} // namespace osrm::extractor

#endif

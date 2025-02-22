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
                      const NodeBasedEdgeClassification flags_)
        : source(source_), target(target_), distance(distance_), weight(weight_),
          duration(duration_), flags(flags_)
    {
    }

    const osrm::util::Coordinate source;
    const osrm::util::Coordinate target;
    const double distance;
    double weight;
    double duration;
    const NodeBasedEdgeClassification flags;
};
} // namespace osrm::extractor

#endif

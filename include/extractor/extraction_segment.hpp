#ifndef OSRM_EXTRACTION_SEGMENT_HPP
#define OSRM_EXTRACTION_SEGMENT_HPP

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
                      const osrm::extractor::NodeBasedEdgeClassification flags_)
        : source(source_), target(target_), distance(distance_), weight(weight_),
          duration(duration_), flags(flags_)
    {
    }

    const osrm::util::Coordinate source;
    const osrm::util::Coordinate target;
    const double distance;
    double weight;
    double duration;
    const osrm::extractor::NodeBasedEdgeClassification flags;
};
} // namespace osrm::extractor

#endif

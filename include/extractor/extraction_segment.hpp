#ifndef OSRM_EXTRACTION_SEGMENT_HPP
#define OSRM_EXTRACTION_SEGMENT_HPP

namespace osrm
{
namespace extractor
{

struct ExtractionSegment
{
  ExtractionSegment(const osrm::util::Coordinate source_, const osrm::util::Coordinate target_, double distance_, double duration_, double weight_)
    : source(source_), target(target_), distance(distance_), duration(duration_), weight(weight_)
  {
  }

  const osrm::util::Coordinate source;
  const osrm::util::Coordinate target;
  const double distance;
  double duration;
  double weight;
};
}
}

#endif

#ifndef INTERNAL_EXTRACTOR_EDGE_HPP
#define INTERNAL_EXTRACTOR_EDGE_HPP

#include "extractor/node_based_edge.hpp"
#include "extractor/travel_mode.hpp"
#include "osrm/coordinate.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>
#include <mapbox/variant.hpp>
#include <utility>

namespace osrm
{
namespace extractor
{

namespace detail
{
// Use a single float to pack two positive values:
//  * by_edge: + sign, value >= 0, value used as the edge weight
//  * by_meter: - sign, value > 0, value used as a denominator in distance / value
struct ByEdgeOrByMeterValue
{
    struct ValueByEdge
    {
    } static const by_edge;
    struct ValueByMeter
    {
    } static const by_meter;

    ByEdgeOrByMeterValue() : value(0.f) {}

    ByEdgeOrByMeterValue(ValueByEdge, double input)
    {
        BOOST_ASSERT(input >= 0.f);
        value = static_cast<value_type>(input);
    }

    ByEdgeOrByMeterValue(ValueByMeter, double input)
    {
        BOOST_ASSERT(input > 0.f);
        value = -static_cast<value_type>(input);
    }

    double operator()(double distance) { return value >= 0 ? value : -distance / value; }

  private:
    using value_type = float;
    value_type value;
};
} // namespace detail

struct InternalExtractorEdge
{
    using WeightData = detail::ByEdgeOrByMeterValue;
    using DurationData = detail::ByEdgeOrByMeterValue;

    explicit InternalExtractorEdge() : weight_data(), duration_data() {}

    explicit InternalExtractorEdge(OSMNodeID source,
                                   OSMNodeID target,
                                   WeightData weight_data,
                                   DurationData duration_data,
                                   util::Coordinate source_coordinate)
        : result(source, target, 0, 0, 0, {}, -1, {}), weight_data(std::move(weight_data)),
          duration_data(std::move(duration_data)), source_coordinate(std::move(source_coordinate))
    {
    }

    explicit InternalExtractorEdge(NodeBasedEdgeWithOSM edge,
                                   WeightData weight_data,
                                   DurationData duration_data,
                                   util::Coordinate source_coordinate)
        : result(std::move(edge)), weight_data(weight_data), duration_data(duration_data),
          source_coordinate(source_coordinate)
    {
    }

    // data that will be written to disk
    NodeBasedEdgeWithOSM result;
    // intermediate edge weight
    WeightData weight_data;
    // intermediate edge duration
    DurationData duration_data;
    // coordinate of the source node
    util::Coordinate source_coordinate;
};
} // namespace extractor
} // namespace osrm

#endif // INTERNAL_EXTRACTOR_EDGE_HPP

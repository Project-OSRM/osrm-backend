#ifndef DESCRIPTION_FACTORY_HPP
#define DESCRIPTION_FACTORY_HPP

#include "engine/douglas_peucker.hpp"
#include "engine/phantom_node.hpp"
#include "engine/segment_information.hpp"
#include "extractor/turn_instructions.hpp"

#include <boost/assert.hpp>

#include "osrm/coordinate.hpp"
#include "osrm/json_container.hpp"

#include <cmath>

#include <limits>
#include <vector>

struct PathData;
/* This class is fed with all way segments in consecutive order
 *  and produces the description plus the encoded polyline */

class DescriptionFactory
{
    DouglasPeucker polyline_generalizer;
    PhantomNode start_phantom, target_phantom;

    double DegreeToRadian(const double degree) const;
    double RadianToDegree(const double degree) const;

    std::vector<unsigned> via_indices;

    double entire_length;

  public:
    struct RouteSummary
    {
        unsigned distance;
        EdgeWeight duration;
        unsigned source_name_id;
        unsigned target_name_id;
        RouteSummary() : distance(0), duration(0), source_name_id(0), target_name_id(0) {}

        void BuildDurationAndLengthStrings(const double raw_distance, const unsigned raw_duration)
        {
            // compute distance/duration for route summary
            distance = static_cast<unsigned>(std::round(raw_distance));
            duration = static_cast<EdgeWeight>(std::round(raw_duration / 10.));
        }
    } summary;

    // I know, declaring this public is considered bad. I'm lazy
    std::vector<SegmentInformation> path_description;
    DescriptionFactory();
    void AppendSegment(const FixedPointCoordinate &coordinate, const PathData &data);
    void BuildRouteSummary(const double distance, const unsigned time);
    void SetStartSegment(const PhantomNode &start_phantom, const bool traversed_in_reverse);
    void SetEndSegment(const PhantomNode &start_phantom,
                       const bool traversed_in_reverse,
                       const bool is_via_location = false);
    osrm::json::Value AppendGeometryString(const bool return_encoded);
    std::vector<unsigned> const &GetViaIndices() const;

    double get_entire_length() const { return entire_length; }

    void Run(const unsigned zoom_level);
};

#endif /* DESCRIPTION_FACTORY_HPP */

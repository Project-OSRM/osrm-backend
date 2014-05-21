/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef EXTRACT_ROUTE_NAMES_H
#define EXTRACT_ROUTE_NAMES_H

#include <boost/assert.hpp>

#include <algorithm>
#include <string>
#include <vector>

struct RouteNames
{
    std::string shortest_path_name_1;
    std::string shortest_path_name_2;
    std::string alternative_path_name_1;
    std::string alternative_path_name_2;
};

// construct routes names
template <class DataFacadeT, class SegmentT> struct ExtractRouteNames
{
    RouteNames operator()(std::vector<SegmentT> &shortest_path_segments,
                          std::vector<SegmentT> &alternative_path_segments,
                          const DataFacadeT *facade)
    {
        RouteNames route_names;

        SegmentT shortest_segment_1, shortest_segment_2;
        SegmentT alternative_segment_1, alternative_segment_2;

        auto length_comperator = [](SegmentT a, SegmentT b)
        { return a.length > b.length; };
        auto name_id_comperator = [](SegmentT a, SegmentT b)
        { return a.name_id < b.name_id; };

        if (shortest_path_segments.empty())
        {
            return route_names;
        }

        std::sort(shortest_path_segments.begin(), shortest_path_segments.end(), length_comperator);
        shortest_segment_1 = shortest_path_segments[0];
        if (!alternative_path_segments.empty())
        {
            std::sort(alternative_path_segments.begin(),
                      alternative_path_segments.end(),
                      length_comperator);
            alternative_segment_1 = alternative_path_segments[0];
        }
        std::vector<SegmentT> shortest_path_set_difference(shortest_path_segments.size());
        std::vector<SegmentT> alternative_path_set_difference(alternative_path_segments.size());
        std::set_difference(shortest_path_segments.begin(),
                            shortest_path_segments.end(),
                            alternative_path_segments.begin(),
                            alternative_path_segments.end(),
                            shortest_path_set_difference.begin(),
                            name_id_comperator);
        int size_of_difference = shortest_path_set_difference.size();
        if (size_of_difference)
        {
            int i = 0;
            while (i < size_of_difference &&
                   shortest_path_set_difference[i].name_id == shortest_path_segments[0].name_id)
            {
                ++i;
            }
            if (i < size_of_difference)
            {
                shortest_segment_2 = shortest_path_set_difference[i];
            }
        }

        std::set_difference(alternative_path_segments.begin(),
                            alternative_path_segments.end(),
                            shortest_path_segments.begin(),
                            shortest_path_segments.end(),
                            alternative_path_set_difference.begin(),
                            name_id_comperator);
        size_of_difference = alternative_path_set_difference.size();
        if (size_of_difference)
        {
            int i = 0;
            while (i < size_of_difference &&
                   alternative_path_set_difference[i].name_id ==
                       alternative_path_segments[0].name_id)
            {
                ++i;
            }
            if (i < size_of_difference)
            {
                alternative_segment_2 = alternative_path_set_difference[i];
            }
        }
        if (shortest_segment_1.position > shortest_segment_2.position)
        {
            std::swap(shortest_segment_1, shortest_segment_2);
        }
        if (alternative_segment_1.position > alternative_segment_2.position)
        {
            std::swap(alternative_segment_1, alternative_segment_2);
        }
        route_names.shortest_path_name_1 =
            facade->GetEscapedNameForNameID(shortest_segment_1.name_id);
        route_names.shortest_path_name_2 =
            facade->GetEscapedNameForNameID(shortest_segment_2.name_id);

        route_names.alternative_path_name_1 =
            facade->GetEscapedNameForNameID(alternative_segment_1.name_id);
        route_names.alternative_path_name_2 =
            facade->GetEscapedNameForNameID(alternative_segment_2.name_id);

        return route_names;
    }
};

#endif // EXTRACT_ROUTE_NAMES_H

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

#ifndef SUGGESTELEVATION_H_
#define SUGGESTELEVATION_H_

#include "../DataStructures/PhantomNodes.h"
#include "../DataStructures/RawRouteData.h"
#include "osrm/Coordinate.h"

/*
 * node - phantom node for which we need to suggest an elevation
 * path_data_vector - vector of vector of PathData - the computed route segments
 * facade - data facade used for coordinate and elevation queries
 * forward - boolean flag which indicates direction (true for the start phantom node, false for the target phantom
 */
template <typename DataFacadeT>
int SuggestElevation(const PhantomNode& node,
                     const std::vector<std::vector<PathData> >& path_data_vector,
                     const DataFacadeT* facade,
                     const bool forward
) {
    const double search_radius = 100.0; // search radius in metres
    const int max_points = 5; // maximum number of points used for the weighted average

    std::vector<std::vector<PathData> >::const_iterator outer_iterator;
    std::vector<std::vector<PathData> >::const_iterator outer_iterator_end;
    std::vector<PathData>::const_iterator inner_iterator;
    std::vector<PathData>::const_iterator inner_iterator_end;
    if (forward) {
        // We are dealing with the start phantom node
        outer_iterator = path_data_vector.begin();
        outer_iterator_end = path_data_vector.end();
        inner_iterator = outer_iterator->begin();
        inner_iterator_end = outer_iterator->end();
    } else {
        // We are dealing with the target phantom node
        outer_iterator = path_data_vector.end() - 1;
        outer_iterator_end = path_data_vector.begin() - 1;
        inner_iterator = outer_iterator->end() - 1;
        inner_iterator_end = outer_iterator->begin() - 1;
    }

    const FixedPointCoordinate initialCoordinates = node.location;

    std::vector<double> distance_vector;
    distance_vector.reserve(max_points);
    std::vector<int> elevation_vector;
    elevation_vector.reserve(max_points);

    int num_points = 0;
    while (num_points < max_points) {
        const FixedPointCoordinate current_coordinates = facade->GetCoordinateOfNode(inner_iterator->node);
        const double current_distance = FixedPointCoordinate::ApproximateDistance(initialCoordinates,
                                                                                 current_coordinates);
        int current_elevation = facade->GetElevationOfNode(inner_iterator->node);
        if (current_distance < 1) {
            return current_elevation;
        } else {
            distance_vector.push_back(current_distance);
            elevation_vector.push_back(current_elevation);
            num_points++;
            if (current_distance > search_radius) {
                break;
            }
        }

        if (forward) {
            ++inner_iterator;
            if (inner_iterator == inner_iterator_end) {
                ++outer_iterator;
                if (outer_iterator != outer_iterator_end) {
                    inner_iterator = outer_iterator->begin();
                    inner_iterator_end = outer_iterator->end();
                } else {
                    break;
                }
            }
        } else {
            --inner_iterator;
            if (inner_iterator == inner_iterator_end) {
                --outer_iterator;
                if (outer_iterator != outer_iterator_end) {
                    inner_iterator = outer_iterator->end() - 1;
                    inner_iterator_end = outer_iterator->begin() - 1;
                } else {
                    break;
                }
            }
        }
    }

    if (distance_vector.size() > 1) {
        double denominator = 0;
        for (std::vector<double>::const_iterator it = distance_vector.begin(); it != distance_vector.end(); ++it) {
            denominator += 1 / (*it);
        }

        int elevation = 0;
        for (int i = 0; i < elevation_vector.size(); ++i) {
            elevation += static_cast<int>(elevation_vector[i] * 1 / distance_vector[i] / denominator);
        }
        return elevation;
    } else {
        return elevation_vector[0];
    }
}

#endif // SUGGESTELEVATION_H_

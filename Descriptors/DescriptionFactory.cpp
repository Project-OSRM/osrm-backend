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

#include "DescriptionFactory.h"

DescriptionFactory::DescriptionFactory() : entireLength(0) {}

DescriptionFactory::~DescriptionFactory() {}

inline double DescriptionFactory::DegreeToRadian(const double degree) const
{
    return degree * (M_PI / 180.);
}

inline double DescriptionFactory::RadianToDegree(const double radian) const
{
    return radian * (180. / M_PI);
}

double DescriptionFactory::GetBearing(const FixedPointCoordinate &A, const FixedPointCoordinate &B)
    const
{
    double delta_long = DegreeToRadian(B.lon / COORDINATE_PRECISION - A.lon / COORDINATE_PRECISION);

    const double lat1 = DegreeToRadian(A.lat / COORDINATE_PRECISION);
    const double lat2 = DegreeToRadian(B.lat / COORDINATE_PRECISION);

    const double y = sin(delta_long) * cos(lat2);
    const double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(delta_long);
    double result = RadianToDegree(atan2(y, x));
    while (result < 0.)
    {
        result += 360.;
    }

    while (result >= 360.)
    {
        result -= 360.;
    }
    return result;
}

void DescriptionFactory::SetStartSegment(const PhantomNode &source)
{
    start_phantom = source;
    AppendSegment(source.location, PathData(0, source.name_id, TurnInstruction::HeadOn, source.forward_weight));
}

void DescriptionFactory::SetEndSegment(const PhantomNode &target)
{
    target_phantom = target;
    path_description.emplace_back(
        target.location, target.name_id, 0, target.reverse_weight, TurnInstruction::NoTurn, true);
}

void DescriptionFactory::AppendSegment(const FixedPointCoordinate &coordinate,
                                       const PathData &path_point)
{
    if ((1 == path_description.size()) && (path_description.back().location == coordinate))
    {
        path_description.back().name_id = path_point.name_id;
    }
    else
    {
        path_description.emplace_back(coordinate,
                                      path_point.name_id,
                                      path_point.segment_duration,
                                      0,
                                      path_point.turn_instruction);
    }
}

void DescriptionFactory::AppendEncodedPolylineString(const bool return_encoded,
                                                     std::vector<std::string> &output)
{
    std::string temp;
    if (return_encoded)
    {
        polyline_compressor.printEncodedString(path_description, temp);
    }
    else
    {
        polyline_compressor.printUnencodedString(path_description, temp);
    }
    output.emplace_back(temp);
}

void DescriptionFactory::AppendEncodedPolylineString(std::vector<std::string> &output) const
{
    std::string temp;
    polyline_compressor.printEncodedString(path_description, temp);
    output.emplace_back(temp);
}

void DescriptionFactory::AppendUnencodedPolylineString(std::vector<std::string> &output) const
{
    std::string temp;
    polyline_compressor.printUnencodedString(path_description, temp);
    output.emplace_back(temp);
}

void DescriptionFactory::BuildRouteSummary(const double distance, const unsigned time)
{
    summary.startName = start_phantom.name_id;
    summary.destName = target_phantom.name_id;
    summary.BuildDurationAndLengthStrings(distance, time);
}

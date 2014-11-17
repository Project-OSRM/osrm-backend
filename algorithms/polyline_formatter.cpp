/*

Copyright (c) 2015, Project OSRM contributors
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

#include "polyline_formatter.hpp"

#include "polyline_compressor.hpp"
#include "../data_structures/segment_information.hpp"

#include <osrm/coordinate.hpp>

std::string PolylineFormatter::printEncodedStr(const std::vector<SegmentInformation> &polyline) const
{
    return PolylineCompressor().get_encoded_string(polyline);
}

std::vector<std::string> PolylineFormatter::printUnencodedStr(const std::vector<SegmentInformation> &polyline) const
{
    std::vector<std::string> output;
    for (const auto &segment : polyline)
    {
        if (segment.necessary)
        {
            std::string tmp, res;
            FixedPointCoordinate::convertInternalLatLonToString(segment.location.lat, tmp);
            res += (tmp + ",");
            FixedPointCoordinate::convertInternalLatLonToString(segment.location.lon, tmp);
            res += tmp;
            output.push_back(res);
        }
    }
    return output;
}

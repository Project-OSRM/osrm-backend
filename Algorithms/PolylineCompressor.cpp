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

#include "PolylineCompressor.h"

void PolylineCompressor::encodeVectorSignedNumber(
    std::vector<int> & numbers,
    std::string & output
) const {
    for(unsigned i = 0; i < numbers.size(); ++i) {
        numbers[i] <<= 1;
        if (numbers[i] < 0) {
            numbers[i] = ~(numbers[i]);
        }
    }
    for(unsigned i = 0; i < numbers.size(); ++i) {
        encodeNumber(numbers[i], output);
    }
}

void PolylineCompressor::encodeNumber(int number_to_encode, std::string & output) const {
    while (number_to_encode >= 0x20) {
        int nextValue = (0x20 | (number_to_encode & 0x1f)) + 63;
        output += static_cast<char>(nextValue);
        if(92 == nextValue) {
            output += static_cast<char>(nextValue);
        }
        number_to_encode >>= 5;
    }

    number_to_encode += 63;
    output += static_cast<char>(number_to_encode);
    if(92 == number_to_encode) {
        output += static_cast<char>(number_to_encode);
    }
}

void PolylineCompressor::printEncodedString(
    const std::vector<SegmentInformation> & polyline,
    std::string & output
) const {
    std::vector<int> deltaNumbers;
    output += "\"";
    if(!polyline.empty()) {
        FixedPointCoordinate lastCoordinate = polyline[0].location;
        deltaNumbers.push_back( lastCoordinate.lat );
        deltaNumbers.push_back( lastCoordinate.lon );
        for(unsigned i = 1; i < polyline.size(); ++i) {
            if(!polyline[i].necessary) {
                continue;
            }
            deltaNumbers.push_back(polyline[i].location.lat - lastCoordinate.lat);
            deltaNumbers.push_back(polyline[i].location.lon - lastCoordinate.lon);
            lastCoordinate = polyline[i].location;
        }
        encodeVectorSignedNumber(deltaNumbers, output);
    }
    output += "\"";

}

void PolylineCompressor::printEncodedString(
    const std::vector<FixedPointCoordinate>& polyline,
    std::string &output
) const {
    std::vector<int> deltaNumbers(2*polyline.size());
    output += "\"";
    if(!polyline.empty()) {
        deltaNumbers[0] = polyline[0].lat;
        deltaNumbers[1] = polyline[0].lon;
        for(unsigned i = 1; i < polyline.size(); ++i) {
            deltaNumbers[(2*i)]   = (polyline[i].lat - polyline[i-1].lat);
            deltaNumbers[(2*i)+1] = (polyline[i].lon - polyline[i-1].lon);
        }
        encodeVectorSignedNumber(deltaNumbers, output);
    }
    output += "\"";
}

void PolylineCompressor::printUnencodedString(
    const std::vector<FixedPointCoordinate> & polyline,
    std::string & output
) const {
    output += "[";
    std::string tmp;
    for(unsigned i = 0; i < polyline.size(); i++) {
        convertInternalLatLonToString(polyline[i].lat, tmp);
        output += "[";
        output += tmp;
        convertInternalLatLonToString(polyline[i].lon, tmp);
        output += ", ";
        output += tmp;
        output += "]";
        if( i < polyline.size()-1 ) {
            output += ",";
        }
    }
    output += "]";
}

void PolylineCompressor::printUnencodedString(
    const std::vector<SegmentInformation> & polyline,
    std::string & output
) const {
    output += "[";
    std::string tmp;
    for(unsigned i = 0; i < polyline.size(); i++) {
        if(!polyline[i].necessary) {
            continue;
        }
        convertInternalLatLonToString(polyline[i].location.lat, tmp);
        output += "[";
        output += tmp;
        convertInternalLatLonToString(polyline[i].location.lon, tmp);
        output += ", ";
        output += tmp;
        output += "]";
        if( i < polyline.size()-1 ) {
            output += ",";
        }
    }
    output += "]";
}

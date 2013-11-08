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

#ifndef ROUTE_PARAMETERS_H
#define ROUTE_PARAMETERS_H

#include "../../DataStructures/Coordinate.h"
#include "../../DataStructures/HashTable.h"

#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/sequence/intrinsic.hpp>
#include <boost/fusion/include/at_c.hpp>

#include <string>
#include <vector>

struct RouteParameters {
    RouteParameters() :
        zoomLevel(18),
        printInstructions(false),
        alternateRoute(true),
        geometry(true),
        compression(true),
        deprecatedAPI(false),
        checkSum(-1) {}
    short zoomLevel;
    bool printInstructions;
    bool alternateRoute;
    bool geometry;
    bool compression;
    bool deprecatedAPI;
    unsigned checkSum;
    std::string service;
    std::string outputFormat;
    std::string jsonpParameter;
    std::string language;
    std::vector<std::string> hints;
    std::vector<FixedPointCoordinate> coordinates;
    typedef HashTable<std::string, std::string>::const_iterator OptionsIterator;

    void setZoomLevel(const short i) {
        if (18 > i && 0 < i) {
            zoomLevel = i;
        }
    }

    void setAlternateRouteFlag(const bool b) {
        alternateRoute = b;
    }

    void setDeprecatedAPIFlag(const std::string &) {
        deprecatedAPI = true;
    }

    void setChecksum(const unsigned c) {
        checkSum = c;
    }

    void setInstructionFlag(const bool b) {
        printInstructions = b;
    }

    void setService( const std::string & s) {
        service = s;
    }

    void setOutputFormat(const std::string & s) {
        outputFormat = s;
    }

    void setJSONpParameter(const std::string & s) {
        jsonpParameter = s;
    }

    void addHint(const std::string & s) {
        hints.resize(coordinates.size());
        hints.back() = s;
    }

    void setLanguage(const std::string & s) {
        language = s;
    }

    void setGeometryFlag(const bool b) {
        geometry = b;
    }

    void setCompressionFlag(const bool b) {
        compression = b;
    }

    void addCoordinate(const boost::fusion::vector < double, double > & arg_) {
        int lat = COORDINATE_PRECISION*boost::fusion::at_c < 0 > (arg_);
        int lon = COORDINATE_PRECISION*boost::fusion::at_c < 1 > (arg_);
        coordinates.push_back(FixedPointCoordinate(lat, lon));
    }
};

#endif /*ROUTE_PARAMETERS_H*/

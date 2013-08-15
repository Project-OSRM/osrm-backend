/*
    open source routing machine
    Copyright (C) Dennis Luxen, 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
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

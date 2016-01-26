/*

Copyright (c) 2016, Project OSRM contributors
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

#ifndef ROUTE_PARAMETERS_HPP
#define ROUTE_PARAMETERS_HPP

#include "osrm/coordinate.hpp"

#include <boost/optional/optional.hpp>

#include <string>
#include <vector>

namespace osrm
{
namespace engine
{

struct RouteParameters
{
    RouteParameters();

    void SetZoomLevel(const short level);

    void SetNumberOfResults(const short number);

    void SetAlternateRouteFlag(const bool flag);

    void SetUTurn(const bool flag);

    void SetAllUTurns(const bool flag);

    void SetClassify(const bool classify);

    void SetMatchingBeta(const double beta);

    void SetGPSPrecision(const double precision);

    void SetDeprecatedAPIFlag(const std::string &);

    void SetChecksum(const unsigned check_sum);

    void SetInstructionFlag(const bool flag);

    void SetService(const std::string &service);

    void SetOutputFormat(const std::string &format);

    void SetJSONpParameter(const std::string &parameter);

    void AddHint(const std::string &hint);

    void AddTimestamp(const unsigned timestamp);

    bool AddBearing(int bearing, boost::optional<int> range);

    void SetLanguage(const std::string &language);

    void SetGeometryFlag(const bool flag);

    void SetCompressionFlag(const bool flag);

    void AddCoordinate(const double latitude, const double longitude);

    void AddDestination(const double latitude, const double longitude);

    void AddSource(const double latitude, const double longitude);

    void SetCoordinatesFromGeometry(const std::string &geometry_string);

    short zoom_level;
    bool print_instructions;
    bool alternate_route;
    bool geometry;
    bool compression;
    bool deprecatedAPI;
    bool uturn_default;
    bool classify;
    double matching_beta;
    double gps_precision;
    unsigned check_sum;
    short num_results;
    std::string service;
    std::string output_format;
    std::string jsonp_parameter;
    std::string language;
    std::vector<std::string> hints;
    std::vector<unsigned> timestamps;
    std::vector<std::pair<const int, const boost::optional<int>>> bearings;
    std::vector<bool> uturns;
    std::vector<FixedPointCoordinate> coordinates;
    std::vector<bool> is_destination;
    std::vector<bool> is_source;
};
}
}

#endif // ROUTE_PARAMETERS_HPP

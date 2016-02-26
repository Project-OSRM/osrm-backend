#include "engine/route_parameters.hpp"
#include "util/coordinate.hpp"

#include "engine/polyline_compressor.hpp"

#include <string>
#include <utility>

namespace osrm
{
namespace engine
{

RouteParameters::RouteParameters()
    : zoom_level(18), print_instructions(false), alternate_route(true), geometry(true),
      compression(true), deprecatedAPI(false), uturn_default(false), classify(false),
      matching_beta(5), gps_precision(5), check_sum(-1), num_results(1)
{
}

void RouteParameters::SetZoomLevel(const short level)
{
    if (18 >= level && 0 <= level)
    {
        zoom_level = level;
    }
}

void RouteParameters::SetNumberOfResults(const short number)
{
    if (number > 0 && number <= 100)
    {
        num_results = number;
    }
}

void RouteParameters::SetAlternateRouteFlag(const bool flag) { alternate_route = flag; }

void RouteParameters::SetUTurn(const bool flag)
{
    // the API grammar should make sure this never happens
    BOOST_ASSERT(!uturns.empty());
    uturns.back() = flag;
}

void RouteParameters::SetAllUTurns(const bool flag)
{
    // if the flag flips the default, then we erase everything.
    if (flag)
    {
        uturn_default = flag;
        uturns.clear();
        uturns.resize(coordinates.size(), uturn_default);
    }
}

void RouteParameters::SetDeprecatedAPIFlag(const std::string & /*unused*/) { deprecatedAPI = true; }

void RouteParameters::SetChecksum(const unsigned sum) { check_sum = sum; }

void RouteParameters::SetInstructionFlag(const bool flag) { print_instructions = flag; }

void RouteParameters::SetService(const std::string &service_string) { service = service_string; }

void RouteParameters::SetClassify(const bool flag) { classify = flag; }

void RouteParameters::SetMatchingBeta(const double beta) { matching_beta = beta; }

void RouteParameters::SetGPSPrecision(const double precision) { gps_precision = precision; }

void RouteParameters::SetOutputFormat(const std::string &format) { output_format = format; }

void RouteParameters::SetJSONpParameter(const std::string &parameter)
{
    jsonp_parameter = parameter;
}

void RouteParameters::AddHint(const std::string &hint)
{
    hints.resize(coordinates.size());
    if (!hints.empty())
    {
        hints.back() = hint;
    }
}

void RouteParameters::AddTimestamp(const unsigned timestamp)
{
    timestamps.resize(coordinates.size());
    if (!timestamps.empty())
    {
        timestamps.back() = timestamp;
    }
}

bool RouteParameters::AddBearing(int bearing, boost::optional<int> range)
{
    if (bearing < 0 || bearing > 359)
        return false;
    if (range && (*range < 0 || *range > 180))
        return false;
    bearings.emplace_back(std::make_pair(bearing, range));
    return true;
}

void RouteParameters::SetLanguage(const std::string &language_string)
{
    language = language_string;
}

void RouteParameters::SetGeometryFlag(const bool flag) { geometry = flag; }

void RouteParameters::SetCompressionFlag(const bool flag) { compression = flag; }

void RouteParameters::AddCoordinate(const double latitude, const double longitude)
{
    coordinates.emplace_back(
        static_cast<int>(COORDINATE_PRECISION * latitude),
        static_cast<int>(COORDINATE_PRECISION * longitude));
    is_source.push_back(true);
    is_destination.push_back(true);
    uturns.push_back(uturn_default);
}

void RouteParameters::AddDestination(const double latitude, const double longitude)
{
    coordinates.emplace_back(
        static_cast<int>(COORDINATE_PRECISION * latitude),
        static_cast<int>(COORDINATE_PRECISION * longitude));
    is_source.push_back(false);
    is_destination.push_back(true);
    uturns.push_back(uturn_default);
}

void RouteParameters::AddSource(const double latitude, const double longitude)
{
    coordinates.emplace_back(
        static_cast<int>(COORDINATE_PRECISION * latitude),
        static_cast<int>(COORDINATE_PRECISION * longitude));
    is_source.push_back(true);
    is_destination.push_back(false);
    uturns.push_back(uturn_default);
}

void RouteParameters::SetCoordinatesFromGeometry(const std::string &geometry_string)
{
    coordinates = polylineDecode(geometry_string);
}

void RouteParameters::SetX(const int &x_) { x = x_; }
void RouteParameters::SetZ(const int &z_) { z = z_; }
void RouteParameters::SetY(const int &y_) { y = y_; }

}
}

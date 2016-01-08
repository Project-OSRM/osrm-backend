#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/sequence/intrinsic.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/spirit/include/qi.hpp>

#include "osrm/route_parameters.hpp"

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

void RouteParameters::AddBearing(
    const boost::fusion::vector<int, boost::optional<int>> &received_bearing,
    boost::spirit::qi::unused_type /* unused */,
    bool &pass)
{
    pass = false;
    const int bearing = boost::fusion::at_c<0>(received_bearing);
    const boost::optional<int> range = boost::fusion::at_c<1>(received_bearing);
    if (bearing < 0 || bearing > 359)
        return;
    if (range && (*range < 0 || *range > 180))
        return;
    bearings.emplace_back(std::make_pair(bearing, range));
    pass = true;
}

void RouteParameters::SetLanguage(const std::string &language_string)
{
    language = language_string;
}

void RouteParameters::SetGeometryFlag(const bool flag) { geometry = flag; }

void RouteParameters::SetCompressionFlag(const bool flag) { compression = flag; }

void RouteParameters::AddCoordinate(
    const boost::fusion::vector<double, double> &received_coordinates)
{
    coordinates.emplace_back(
        static_cast<int>(COORDINATE_PRECISION * boost::fusion::at_c<0>(received_coordinates)),
        static_cast<int>(COORDINATE_PRECISION * boost::fusion::at_c<1>(received_coordinates)));
    is_source.push_back(true);
    is_destination.push_back(true);
    uturns.push_back(uturn_default);
}

void RouteParameters::AddDestination(
    const boost::fusion::vector<double, double> &received_coordinates)
{
    coordinates.emplace_back(
        static_cast<int>(COORDINATE_PRECISION * boost::fusion::at_c<0>(received_coordinates)),
        static_cast<int>(COORDINATE_PRECISION * boost::fusion::at_c<1>(received_coordinates)));
    is_source.push_back(false);
    is_destination.push_back(true);
    uturns.push_back(uturn_default);
}

void RouteParameters::AddSource(const boost::fusion::vector<double, double> &received_coordinates)
{
    coordinates.emplace_back(
        static_cast<int>(COORDINATE_PRECISION * boost::fusion::at_c<0>(received_coordinates)),
        static_cast<int>(COORDINATE_PRECISION * boost::fusion::at_c<1>(received_coordinates)));
    is_source.push_back(true);
    is_destination.push_back(false);
    uturns.push_back(uturn_default);
}

void RouteParameters::SetCoordinatesFromGeometry(const std::string &geometry_string)
{
    coordinates = polylineDecode(geometry_string);
}
}
}

#ifndef API_GRAMMAR_HPP
#define API_GRAMMAR_HPP

#include <boost/bind.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_action.hpp>

namespace osrm
{
namespace server
{

namespace qi = boost::spirit::qi;

template <typename Iterator, class HandlerT> struct APIGrammar : qi::grammar<Iterator>
{
    explicit APIGrammar(HandlerT *h) : APIGrammar::base_type(api_call), handler(h)
    {
        const auto set_x_wrapper = [this](const int x, bool &pass)
        {
            pass = handler->SetX(x);
        };
        const auto set_y_wrapper = [this](const int y, bool &pass)
        {
            pass = handler->SetY(y);
        };
        const auto set_z_wrapper = [this](const int z, bool &pass)
        {
            pass = handler->SetZ(z);
        };
        const auto add_bearing_wrapper = [this](
            const boost::fusion::vector<int, boost::optional<int>> &received_bearing, bool &pass)
        {
            const int bearing = boost::fusion::at_c<0>(received_bearing);
            const boost::optional<int> range = boost::fusion::at_c<1>(received_bearing);
            pass = handler->AddBearing(bearing, range);
        };
        const auto add_coordinate_wrapper =
            [this](const boost::fusion::vector<double, double> &received_coordinate)
        {
            handler->AddCoordinate(boost::fusion::at_c<0>(received_coordinate),
                                   boost::fusion::at_c<1>(received_coordinate));
        };
        const auto add_source_wrapper =
            [this](const boost::fusion::vector<double, double> &received_coordinate)
        {
            handler->AddSource(boost::fusion::at_c<0>(received_coordinate),
                               boost::fusion::at_c<1>(received_coordinate));
        };
        const auto add_destination_wrapper =
            [this](const boost::fusion::vector<double, double> &received_coordinate)
        {
            handler->AddDestination(boost::fusion::at_c<0>(received_coordinate),
                                    boost::fusion::at_c<1>(received_coordinate));
        };

        api_call =
            qi::lit('/') >> string[boost::bind(&HandlerT::SetService, handler, ::_1)] >> -query;
        query = ('?') >> +(zoom | output | jsonp | checksum | uturns | location_with_options |
                           destination_with_options | source_with_options | cmp | language |
                           instruction | geometry | alt_route | old_API | num_results |
                           matching_beta | gps_precision | classify | locs | x | y | z);
        // all combinations of timestamp, uturn, hint and bearing without duplicates
        t_u = (u >> -timestamp) | (timestamp >> -u);
        t_h = (hint >> -timestamp) | (timestamp >> -hint);
        u_h = (u >> -hint) | (hint >> -u);
        t_u_h = (hint >> -t_u) | (u >> -t_h) | (timestamp >> -u_h);
        location_options =
            (bearing >> -t_u_h) | (t_u_h >> -bearing) |                                          //
            (u >> bearing >> -t_h) | (timestamp >> bearing >> -u_h) | (hint >> bearing >> t_u) | //
            (t_h >> bearing >> -u) | (u_h >> bearing >> -timestamp) | (t_u >> bearing >> -hint);
        location_with_options = location >> -location_options;
        source_with_options = source >> -location_options;
        destination_with_options = destination >> -location_options;
        zoom = (-qi::lit('&')) >> qi::lit('z') >> '=' >>
               qi::short_[boost::bind(&HandlerT::SetZoomLevel, handler, ::_1)];
        output = (-qi::lit('&')) >> qi::lit("output=json");
        jsonp = (-qi::lit('&')) >> qi::lit("jsonp") >> '=' >>
                stringwithPercent[boost::bind(&HandlerT::SetJSONpParameter, handler, ::_1)];
        checksum = (-qi::lit('&')) >> qi::lit("checksum") >> '=' >>
                   qi::uint_[boost::bind(&HandlerT::SetChecksum, handler, ::_1)];
        instruction = (-qi::lit('&')) >> qi::lit("instructions") >> '=' >>
                      qi::bool_[boost::bind(&HandlerT::SetInstructionFlag, handler, ::_1)];
        geometry = (-qi::lit('&')) >> qi::lit("geometry") >> '=' >>
                   qi::bool_[boost::bind(&HandlerT::SetGeometryFlag, handler, ::_1)];
        cmp = (-qi::lit('&')) >> qi::lit("compression") >> '=' >>
              qi::bool_[boost::bind(&HandlerT::SetCompressionFlag, handler, ::_1)];
        location = (-qi::lit('&')) >> qi::lit("loc") >> '=' >>
                   (qi::double_ >> qi::lit(',') >>
                    qi::double_)[boost::bind<void>(add_coordinate_wrapper, ::_1)];
        destination = (-qi::lit('&')) >> qi::lit("dst") >> '=' >>
                      (qi::double_ >> qi::lit(',') >>
                       qi::double_)[boost::bind<void>(add_destination_wrapper, ::_1)];
        source = (-qi::lit('&')) >> qi::lit("src") >> '=' >>
                 (qi::double_ >> qi::lit(',') >>
                  qi::double_)[boost::bind<void>(add_source_wrapper, ::_1)];
        hint = (-qi::lit('&')) >> qi::lit("hint") >> '=' >>
               stringwithDot[boost::bind(&HandlerT::AddHint, handler, ::_1)];
        timestamp = (-qi::lit('&')) >> qi::lit("t") >> '=' >>
                    qi::uint_[boost::bind(&HandlerT::AddTimestamp, handler, ::_1)];
        bearing = (-qi::lit('&')) >> qi::lit("b") >> '=' >>
                  (qi::int_ >> -(qi::lit(',') >> qi::int_ |
                                 qi::attr(10)))[boost::bind<void>(add_bearing_wrapper, ::_1, ::_3)];
        u = (-qi::lit('&')) >> qi::lit("u") >> '=' >>
            qi::bool_[boost::bind(&HandlerT::SetUTurn, handler, ::_1)];
        uturns = (-qi::lit('&')) >> qi::lit("uturns") >> '=' >>
                 qi::bool_[boost::bind(&HandlerT::SetAllUTurns, handler, ::_1)];
        language = (-qi::lit('&')) >> qi::lit("hl") >> '=' >>
                   string[boost::bind(&HandlerT::SetLanguage, handler, ::_1)];
        alt_route = (-qi::lit('&')) >> qi::lit("alt") >> '=' >>
                    qi::bool_[boost::bind(&HandlerT::SetAlternateRouteFlag, handler, ::_1)];
        old_API = (-qi::lit('&')) >> qi::lit("geomformat") >> '=' >>
                  string[boost::bind(&HandlerT::SetDeprecatedAPIFlag, handler, ::_1)];
        num_results = (-qi::lit('&')) >> qi::lit("num_results") >> '=' >>
                      qi::short_[boost::bind(&HandlerT::SetNumberOfResults, handler, ::_1)];
        matching_beta = (-qi::lit('&')) >> qi::lit("matching_beta") >> '=' >>
                        qi::float_[boost::bind(&HandlerT::SetMatchingBeta, handler, ::_1)];
        gps_precision = (-qi::lit('&')) >> qi::lit("gps_precision") >> '=' >>
                        qi::float_[boost::bind(&HandlerT::SetGPSPrecision, handler, ::_1)];
        classify = (-qi::lit('&')) >> qi::lit("classify") >> '=' >>
                   qi::bool_[boost::bind(&HandlerT::SetClassify, handler, ::_1)];
        locs = (-qi::lit('&')) >> qi::lit("locs") >> '=' >>
               stringforPolyline[boost::bind(&HandlerT::SetCoordinatesFromGeometry, handler, ::_1)];

        x = (-qi::lit('&')) >> qi::lit("tx") >> '=' >>
            qi::int_[boost::bind<void>(set_x_wrapper, ::_1, ::_3)];
        y = (-qi::lit('&')) >> qi::lit("ty") >> '=' >>
            qi::int_[boost::bind<void>(set_y_wrapper, ::_1, ::_3)];
        z = (-qi::lit('&')) >> qi::lit("tz") >> '=' >>
            qi::int_[boost::bind<void>(set_z_wrapper, ::_1, ::_3)];

        string = +(qi::char_("a-zA-Z"));
        stringwithDot = +(qi::char_("a-zA-Z0-9_.-"));
        stringwithPercent = +(qi::char_("a-zA-Z0-9_.-") | qi::char_('[') | qi::char_(']') |
                              (qi::char_('%') >> qi::char_("0-9A-Z") >> qi::char_("0-9A-Z")));
        stringforPolyline = +(qi::char_("a-zA-Z0-9_.-[]{}@?|\\%~`^"));
    }

    qi::rule<Iterator> api_call, query, location_options, location_with_options,
        destination_with_options, source_with_options, t_u, t_h, u_h, t_u_h;
    qi::rule<Iterator, std::string()> service, zoom, output, string, jsonp, checksum, location,
        destination, source, hint, timestamp, bearing, stringwithDot, stringwithPercent, language,
        geometry, cmp, alt_route, u, uturns, old_API, num_results, matching_beta, gps_precision,
        classify, locs, instruction, stringforPolyline, x, y, z;

    HandlerT *handler;
};
}
}

#endif /* API_GRAMMAR_HPP */

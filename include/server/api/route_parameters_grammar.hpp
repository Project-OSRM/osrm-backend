#ifndef ROUTE_PARAMETERS_GRAMMAR_HPP
#define ROUTE_PARAMETERS_GRAMMAR_HPP

#include "server/api/base_parameters_grammar.hpp"
#include "engine/api/route_parameters.hpp"

#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

namespace osrm
{
namespace server
{
namespace api
{

namespace
{
namespace ph = boost::phoenix;
namespace qi = boost::spirit::qi;
}

template <typename Iterator = std::string::iterator,
          typename Signature = void(engine::api::RouteParameters &)>
struct RouteParametersGrammar : public BaseParametersGrammar<Iterator, Signature>
{
    using BaseGrammar = BaseParametersGrammar<Iterator, Signature>;

    RouteParametersGrammar() : RouteParametersGrammar(root_rule)
    {
        route_rule =
            (qi::lit("alternatives=") >
             (qi::uint_[ph::bind(&engine::api::RouteParameters::number_of_alternatives, qi::_r1) =
                            qi::_1,
                        ph::bind(&engine::api::RouteParameters::alternatives, qi::_r1) =
                            qi::_1 > 0] |
              qi::bool_[ph::bind(&engine::api::RouteParameters::number_of_alternatives, qi::_r1) =
                            qi::_1,
                        ph::bind(&engine::api::RouteParameters::alternatives, qi::_r1) = qi::_1])) |
            (qi::lit("continue_straight=") >
             (qi::lit("default") |
              qi::bool_[ph::bind(&engine::api::RouteParameters::continue_straight, qi::_r1) =
                            qi::_1]));

        root_rule = query_rule(qi::_r1) > BaseGrammar::format_rule(qi::_r1) >
                    -('?' > (route_rule(qi::_r1) | base_rule(qi::_r1)) % '&');
    }

    RouteParametersGrammar(qi::rule<Iterator, Signature> &root_rule_) : BaseGrammar(root_rule_)
    {
#ifdef BOOST_HAS_LONG_LONG
        if (std::is_same<std::size_t, unsigned long long>::value)
            size_t_ = qi::ulong_long;
        else
            size_t_ = qi::ulong_;
#else
        size_t_ = qi::ulong_;
#endif
        using AnnotationsType = engine::api::RouteParameters::AnnotationsType;

        const auto add_annotation = [](engine::api::RouteParameters &route_parameters,
                                       AnnotationsType route_param) {
            route_parameters.annotations_type = route_parameters.annotations_type | route_param;
            route_parameters.annotations =
                route_parameters.annotations_type != AnnotationsType::None;
        };

        geometries_type.add("geojson", engine::api::RouteParameters::GeometriesType::GeoJSON)(
            "polyline", engine::api::RouteParameters::GeometriesType::Polyline)(
            "polyline6", engine::api::RouteParameters::GeometriesType::Polyline6);

        overview_type.add("simplified", engine::api::RouteParameters::OverviewType::Simplified)(
            "full", engine::api::RouteParameters::OverviewType::Full)(
            "false", engine::api::RouteParameters::OverviewType::False);

        annotations_type.add("duration", AnnotationsType::Duration)("nodes",
                                                                    AnnotationsType::Nodes)(
            "distance", AnnotationsType::Distance)("weight", AnnotationsType::Weight)(
            "datasources", AnnotationsType::Datasources)("speed", AnnotationsType::Speed);

        waypoints_rule =
            qi::lit("waypoints=") >
            (size_t_ % ';')[ph::bind(&engine::api::RouteParameters::waypoints, qi::_r1) = qi::_1];

        base_rule =
            BaseGrammar::base_rule(qi::_r1) | waypoints_rule(qi::_r1) |
            (qi::lit("steps=") >
             qi::bool_[ph::bind(&engine::api::RouteParameters::steps, qi::_r1) = qi::_1]) |
            (qi::lit("geometries=") >
             geometries_type[ph::bind(&engine::api::RouteParameters::geometries, qi::_r1) =
                                 qi::_1]) |
            (qi::lit("overview=") >
             overview_type[ph::bind(&engine::api::RouteParameters::overview, qi::_r1) = qi::_1]) |
            (qi::lit("annotations=") >
             (qi::lit("true")[ph::bind(add_annotation, qi::_r1, AnnotationsType::All)] |
              qi::lit("false")[ph::bind(add_annotation, qi::_r1, AnnotationsType::None)] |
              (annotations_type[ph::bind(add_annotation, qi::_r1, qi::_1)] % ',')));

        query_rule = BaseGrammar::query_rule(qi::_r1);
    }

  protected:
    qi::rule<Iterator, Signature> base_rule;
    qi::rule<Iterator, Signature> query_rule;

  private:
    qi::rule<Iterator, Signature> root_rule;
    qi::rule<Iterator, Signature> route_rule;
    qi::rule<Iterator, Signature> waypoints_rule;
    qi::rule<Iterator, std::size_t()> size_t_;

    qi::symbols<char, engine::api::RouteParameters::GeometriesType> geometries_type;
    qi::symbols<char, engine::api::RouteParameters::OverviewType> overview_type;
    qi::symbols<char, engine::api::RouteParameters::AnnotationsType> annotations_type;
};
}
}
}

#endif

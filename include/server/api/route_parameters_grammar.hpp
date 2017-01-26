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

    const auto add_annotation = [](engine::api::RouteParameters &route_parameters,
                                   engine::api::RouteParameters::AnnotationsType &route_param) {
        // bitflag each parameter
        if (route_param)
        {
            route_parameters.annotations_ = route_parameters.annotations_ | route_param;
        }
    };

    RouteParametersGrammar() : RouteParametersGrammar(root_rule)
    {
        route_rule =
            (qi::lit("alternatives=") >
             qi::bool_[ph::bind(&engine::api::RouteParameters::alternatives, qi::_r1) = qi::_1]) |
            (qi::lit("continue_straight=") >
             (qi::lit("default") |
              qi::bool_[ph::bind(&engine::api::RouteParameters::continue_straight, qi::_r1) =
                            qi::_1]));

        root_rule = query_rule(qi::_r1) > -qi::lit(".json") >
                    -('?' > (route_rule(qi::_r1) | base_rule(qi::_r1)) % '&');
    }

    RouteParametersGrammar(qi::rule<Iterator, Signature> &root_rule_) : BaseGrammar(root_rule_)
    {
        geometries_type.add("geojson", engine::api::RouteParameters::GeometriesType::GeoJSON)(
            "polyline", engine::api::RouteParameters::GeometriesType::Polyline)(
            "polyline6", engine::api::RouteParameters::GeometriesType::Polyline6);

        overview_type.add("simplified", engine::api::RouteParameters::OverviewType::Simplified)(
            "full", engine::api::RouteParameters::OverviewType::Full)(
            "false", engine::api::RouteParameters::OverviewType::False);

        annotations_type.add("all", engine::api::RouteParameters::AnnotationsType::All)(
            "none", engine::api::RouteParameters::AnnotationsType::None)(
            "duration", engine::api::RouteParameters::AnnotationsType::Duration)(
            "nodes", engine::api::RouteParameters::AnnotationsType::Nodes)(
            "distance", engine::api::RouteParameters::AnnotationsType::Distance);

        base_rule =
            BaseGrammar::base_rule(qi::_r1) |
            (qi::lit("steps=") >
             qi::bool_[ph::bind(&engine::api::RouteParameters::steps, qi::_r1) = qi::_1]) |
            (qi::lit("geometries=") >
             geometries_type[ph::bind(&engine::api::RouteParameters::geometries, qi::_r1) =
                                 qi::_1]) |
            (qi::lit("overview=") >
             overview_type[ph::bind(&engine::api::RouteParameters::overview, qi::_r1) = qi::_1]) |
            (qi::lit("annotations=") >
             annotations_type[ph::bind(add_annotation, qi::_r1, qi::_1)] % ',');

        query_rule = BaseGrammar::query_rule(qi::_r1);
    }

  protected:
    qi::rule<Iterator, Signature> base_rule;
    qi::rule<Iterator, Signature> query_rule;

  private:
    qi::rule<Iterator, Signature> root_rule;
    qi::rule<Iterator, Signature> route_rule;

    qi::symbols<char, engine::api::RouteParameters::GeometriesType> geometries_type;
    qi::symbols<char, engine::api::RouteParameters::OverviewType> overview_type;
    qi::symbols<char, engine::api::RouteParameters::AnnotationsType> annotations_type;
};
}
}
}

#endif

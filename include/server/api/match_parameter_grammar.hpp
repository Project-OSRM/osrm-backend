#ifndef MATCH_PARAMETERS_GRAMMAR_HPP
#define MATCH_PARAMETERS_GRAMMAR_HPP

#include "engine/api/match_parameters.hpp"
#include "server/api/base_parameters_grammar.hpp"

//#define BOOST_SPIRIT_DEBUG
#include <boost/spirit/include/qi.hpp>

namespace osrm
{
namespace server
{
namespace api
{

namespace qi = boost::spirit::qi;

struct MatchParametersGrammar final : public BaseParametersGrammar
{
    using Iterator = std::string::iterator;
    using StepsT = bool;
    using TimestampsT = std::vector<unsigned>;
    using GeometriesT = engine::api::RouteParameters::GeometriesType;
    using OverviewT = engine::api::RouteParameters::OverviewType;

    MatchParametersGrammar() : BaseParametersGrammar(root_rule, parameters)
    {
        const auto set_geojson_type = [this] {
            parameters.geometries = engine::api::RouteParameters::GeometriesType::GeoJSON;
        };
        const auto set_polyline_type = [this] {
            parameters.geometries = engine::api::RouteParameters::GeometriesType::Polyline;
        };

        const auto set_simplified_type = [this] {
            parameters.overview = engine::api::RouteParameters::OverviewType::Simplified;
        };
        const auto set_full_type = [this] {
            parameters.overview = engine::api::RouteParameters::OverviewType::Full;
        };
        const auto set_false_type = [this] {
            parameters.overview = engine::api::RouteParameters::OverviewType::False;
        };
        const auto set_steps = [this](const StepsT steps) { parameters.steps = steps; };
        const auto set_timestamps = [this](TimestampsT timestamps) {
            parameters.timestamps = std::move(timestamps);
        };

        steps_rule = qi::lit("steps=") > qi::bool_;
        geometries_rule = qi::lit("geometries=geojson")[set_geojson_type] |
                          qi::lit("geometries=polyline")[set_polyline_type];
        overview_rule = qi::lit("overview=simplified")[set_simplified_type] |
                        qi::lit("overview=full")[set_full_type] |
                        qi::lit("overview=false")[set_false_type];
        timestamps_rule = qi::lit("timestamps=") > qi::uint_ % ";";
        match_rule = steps_rule[set_steps] | geometries_rule | overview_rule |
                     timestamps_rule[set_timestamps];
        root_rule =
            query_rule > -qi::lit(".json") > -(qi::lit("?") > (match_rule | base_rule) % '&');
    }

    engine::api::MatchParameters parameters;

  private:
    qi::rule<Iterator> root_rule, match_rule, geometries_rule, overview_rule;
    qi::rule<Iterator, TimestampsT()> timestamps_rule;
    qi::rule<Iterator, StepsT()> steps_rule;
};
}
}
}

#endif

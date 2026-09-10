#ifndef ISOCHRONE_PARAMETERS_GRAMMAR_HPP
#define ISOCHRONE_PARAMETERS_GRAMMAR_HPP

#include "server/api/base_parameters_grammar.hpp"

#include "engine/api/isochrone_parameters.hpp"

#include <boost/spirit/home/x3.hpp>

namespace osrm::server::api::isochrone_grammar
{

namespace x3 = boost::spirit::x3;

inline const auto direction_type = []()
{
    x3::symbols<engine::api::IsochroneParameters::Direction> sym;
    sym.add("outbound", engine::api::IsochroneParameters::Direction::Outbound)(
        "inbound", engine::api::IsochroneParameters::Direction::Inbound);
    return sym;
}();

inline const auto contours_seconds_rule =
    x3::lit("contours_seconds=") >
    (base_grammar::json_double %
     ',')[([](auto &ctx) { x3::get<params_tag>(ctx).get().contours_seconds = x3::_attr(ctx); })];

inline const auto direction_rule =
    x3::lit("direction=") >
    direction_type[([](auto &ctx) { x3::get<params_tag>(ctx).get().direction = x3::_attr(ctx); })];

inline const auto polygons_rule =
    x3::lit("polygons=") >
    x3::bool_[([](auto &ctx) { x3::get<params_tag>(ctx).get().polygons = x3::_attr(ctx); })];

inline const auto generalize_rule =
    x3::lit("generalize=") >
    base_grammar::json_double[([](auto &ctx)
                               { x3::get<params_tag>(ctx).get().generalize = x3::_attr(ctx); })];

inline const auto denoise_rule =
    x3::lit("denoise=") >
    base_grammar::json_double[([](auto &ctx)
                               { x3::get<params_tag>(ctx).get().denoise = x3::_attr(ctx); })];

inline const auto root_rule = x3::rule<struct isochrone_root_tag>{"isochrone_root"} =
    base_grammar::query_rule > base_grammar::format_rule >
    -('?' > (contours_seconds_rule | direction_rule | polygons_rule | generalize_rule |
             denoise_rule | base_grammar::base_options) %
                '&');

} // namespace osrm::server::api::isochrone_grammar

#endif // ISOCHRONE_PARAMETERS_GRAMMAR_HPP

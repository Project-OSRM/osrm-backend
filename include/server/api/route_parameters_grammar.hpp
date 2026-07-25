#ifndef ROUTE_PARAMETERS_GRAMMAR_HPP
#define ROUTE_PARAMETERS_GRAMMAR_HPP

#include "server/api/base_parameters_grammar.hpp"
#include "engine/api/route_parameters.hpp"

#include <boost/spirit/home/x3.hpp>

namespace osrm::server::api::route_grammar
{

namespace x3 = boost::spirit::x3;
inline const x3::uint_parser<std::size_t> size_t_{};

using AnnotationsType = engine::api::RouteParameters::AnnotationsType;

// --- Symbol tables ---

inline const auto geometries_type = []()
{
    x3::symbols<engine::api::RouteParameters::GeometriesType> sym;
    sym.add("geojson", engine::api::RouteParameters::GeometriesType::GeoJSON)(
        "polyline", engine::api::RouteParameters::GeometriesType::Polyline)(
        "polyline6", engine::api::RouteParameters::GeometriesType::Polyline6);
    return sym;
}();

inline const auto overview_type = []()
{
    x3::symbols<engine::api::RouteParameters::OverviewType> sym;
    sym.add("simplified", engine::api::RouteParameters::OverviewType::Simplified)(
        "full", engine::api::RouteParameters::OverviewType::Full)(
        "false", engine::api::RouteParameters::OverviewType::False)(
        "by_legs", engine::api::RouteParameters::OverviewType::ByLegs);
    return sym;
}();

inline const auto annotations_type = []()
{
    x3::symbols<AnnotationsType> sym;
    sym.add("duration", AnnotationsType::Duration)("nodes", AnnotationsType::Nodes)(
        "distance", AnnotationsType::Distance)("weight", AnnotationsType::Weight)(
        "datasources", AnnotationsType::Datasources)("speed", AnnotationsType::Speed);
    return sym;
}();

// --- Route-specific parameter rules ---

inline const auto alternatives_rule = x3::lit("alternatives=") >
                                      (x3::uint_[(
                                           [](auto &ctx)
                                           {
                                               auto &p = x3::get<params_tag>(ctx).get();
                                               p.number_of_alternatives = x3::_attr(ctx);
                                               p.alternatives = x3::_attr(ctx) > 0;
                                           })] |
                                       x3::bool_[(
                                           [](auto &ctx)
                                           {
                                               auto &p = x3::get<params_tag>(ctx).get();
                                               p.number_of_alternatives = x3::_attr(ctx);
                                               p.alternatives = x3::_attr(ctx);
                                           })]);

inline const auto continue_straight_rule =
    x3::lit("continue_straight=") >
    (x3::lit("default") |
     x3::bool_[([](auto &ctx)
                { x3::get<params_tag>(ctx).get().continue_straight = x3::_attr(ctx); })]);

inline const auto route_rule = alternatives_rule | continue_straight_rule;

inline const auto steps_rule =
    x3::lit("steps=") >
    x3::bool_[([](auto &ctx) { x3::get<params_tag>(ctx).get().steps = x3::_attr(ctx); })];

inline const auto geometries_rule =
    x3::lit("geometries=") >
    geometries_type[([](auto &ctx)
                     { x3::get<params_tag>(ctx).get().geometries = x3::_attr(ctx); })];

inline const auto overview_rule =
    x3::lit("overview=") >
    overview_type[([](auto &ctx) { x3::get<params_tag>(ctx).get().overview = x3::_attr(ctx); })];

inline const auto add_annotation = [](auto &ctx)
{
    auto &p = x3::get<params_tag>(ctx).get();
    p.annotations_type = p.annotations_type | x3::_attr(ctx);
    p.annotations = p.annotations_type != AnnotationsType::None;
};

inline const auto
    annotations_rule = x3::lit("annotations=") >
                       ((x3::lit("true") >> x3::attr(AnnotationsType::All))[add_annotation] |
                        (x3::lit("false") >> x3::attr(AnnotationsType::None))[add_annotation] |
                        annotations_type[add_annotation] % ',');

inline const auto waypoints_rule =
    x3::lit("waypoints=") >
    (size_t_ % ';')[([](auto &ctx) { x3::get<params_tag>(ctx).get().waypoints = x3::_attr(ctx); })];

// Route options = base options + route extensions
inline const auto route_options = x3::rule<struct route_options_tag>{"route_options"} =
    base_grammar::base_options | waypoints_rule | steps_rule | geometries_rule | overview_rule |
    annotations_rule;

// Route root rule
inline const auto root_rule = x3::rule<struct route_root_tag>{"route_root"} =
    base_grammar::query_rule > base_grammar::format_rule >
    -('?' > (route_rule | route_options) % '&');

} // namespace osrm::server::api::route_grammar

#endif

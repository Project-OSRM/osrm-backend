#ifndef TABLE_PARAMETERS_GRAMMAR_HPP
#define TABLE_PARAMETERS_GRAMMAR_HPP

#include "server/api/base_parameters_grammar.hpp"
#include "engine/api/table_parameters.hpp"

#include <boost/spirit/home/x3.hpp>

namespace osrm::server::api::table_grammar
{

namespace x3 = boost::spirit::x3;
using json_policy = no_trailing_dot_policy<double, 'j', 's', 'o', 'n'>;
inline const x3::real_parser<double, json_policy> json_double{};
inline const x3::uint_parser<std::size_t> size_t_{};

using AnnotationsType = engine::api::TableParameters::AnnotationsType;

// --- Symbol tables ---

inline const auto annotations_sym = []()
{
    x3::symbols<AnnotationsType> sym;
    sym.add("duration", AnnotationsType::Duration)("distance", AnnotationsType::Distance);
    return sym;
}();

inline const auto fallback_coordinate_type = []()
{
    x3::symbols<engine::api::TableParameters::FallbackCoordinateType> sym;
    sym.add("input", engine::api::TableParameters::FallbackCoordinateType::Input)(
        "snapped", engine::api::TableParameters::FallbackCoordinateType::Snapped);
    return sym;
}();

// --- Table-specific rules ---

// Accumulate annotations with bitwise OR
inline const auto annotations_list =
    x3::rule<struct table_annotations_list_tag, AnnotationsType>{"table_annotations_list"} =
        annotations_sym[([](auto &ctx) { x3::_val(ctx) = x3::_val(ctx) | x3::_attr(ctx); })] % ',';

inline const auto table_annotations_rule =
    x3::lit("annotations=") >
    annotations_list[([](auto &ctx)
                      { x3::get<params_tag>(ctx).get().annotations = x3::_attr(ctx); })];

// Table base options = base + table annotations
inline const auto table_base_options =
    x3::rule<struct table_base_options_tag>{"table_base_options"} =
        base_grammar::base_options | table_annotations_rule;

inline const auto sources_rule =
    x3::lit("sources=") >
    (x3::lit("all") |
     (size_t_ % ';')[([](auto &ctx) { x3::get<params_tag>(ctx).get().sources = x3::_attr(ctx); })]);

inline const auto destinations_rule =
    x3::lit("destinations=") >
    (x3::lit("all") |
     (size_t_ %
      ';')[([](auto &ctx) { x3::get<params_tag>(ctx).get().destinations = x3::_attr(ctx); })]);

inline const auto fallback_speed_rule =
    x3::lit("fallback_speed=") >
    json_double[([](auto &ctx)
                 { x3::get<params_tag>(ctx).get().fallback_speed = x3::_attr(ctx); })];

inline const auto scale_factor_rule =
    x3::lit("scale_factor=") >
    json_double[([](auto &ctx) { x3::get<params_tag>(ctx).get().scale_factor = x3::_attr(ctx); })];

inline const auto fallback_coordinate_rule =
    x3::lit("fallback_coordinate=") >
    fallback_coordinate_type[(
        [](auto &ctx)
        { x3::get<params_tag>(ctx).get().fallback_coordinate_type = x3::_attr(ctx); })];

inline const auto table_rule = destinations_rule | sources_rule;

// Table root rule
inline const auto root_rule = x3::rule<struct table_root_tag>{"table_root"} =
    base_grammar::query_rule > base_grammar::format_rule >
    -('?' > (table_rule | table_base_options | scale_factor_rule | fallback_speed_rule |
             fallback_coordinate_rule) %
                '&');

} // namespace osrm::server::api::table_grammar

#endif

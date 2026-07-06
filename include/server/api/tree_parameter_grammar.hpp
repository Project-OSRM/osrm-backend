#ifndef TREE_PARAMETERS_GRAMMAR_HPP
#define TREE_PARAMETERS_GRAMMAR_HPP

#include "server/api/base_parameters_grammar.hpp"
#include "engine/api/tree_parameters.hpp"

#include <boost/spirit/home/x3.hpp>

namespace osrm::server::api::tree_grammar
{

namespace x3 = boost::spirit::x3;

inline const auto hard_cap_rule =
    x3::lit("hard_cap_m=") >
    x3::uint_[([](auto &ctx) { x3::get<params_tag>(ctx).get().hard_cap_m = x3::_attr(ctx); })];

inline const auto debug_rule =
    x3::lit("debug=") >
    x3::bool_[([](auto &ctx) { x3::get<params_tag>(ctx).get().debug = x3::_attr(ctx); })];

// Tree root rule. Reuses the base options (coordinates, bearings, radiuses, ...) plus the
// tree-specific hard_cap_m and debug. The bearing is mandatory in practice (it disambiguates the
// carriageway) but that is enforced in the plugin, not the grammar, to keep the surface identical
// to the other services.
inline const auto root_rule = x3::rule<struct tree_root_tag>{"tree_root"} =
    base_grammar::query_rule > base_grammar::format_rule >
    -('?' > (hard_cap_rule | debug_rule | base_grammar::base_options) % '&');

} // namespace osrm::server::api::tree_grammar

#endif

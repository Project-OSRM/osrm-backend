#ifndef SERVER_API_ISOCHRONE_PARAMETERS_GRAMMAR_HPP
#define SERVER_API_ISOCHRONE_PARAMETERS_GRAMMAR_HPP

#include "server/api/base_parameters_grammar.hpp"
#include "engine/api/isochrone_parameters.hpp"

#include <boost/spirit/home/x3.hpp>

namespace osrm::server::api::isochrone_grammar
{
namespace x3 = boost::spirit::x3;

inline const auto range_rule = x3::rule<struct range_rule_tag>{"range_rule"} =
    x3::lit("range=") > x3::uint_[([](auto &ctx) { x3::get<params_tag>(ctx).get().range = x3::_attr(ctx); })];

// Root rule: coords and optional format, then optional query options starting with '?'
inline const auto root_rule = x3::rule<struct isochrone_root_tag>{"isochrone_root"} =
    base_grammar::query_rule > base_grammar::format_rule >
    -('?' > (range_rule | base_grammar::base_options) % '&');


} // namespace osrm::server::api::isochrone_grammar

#endif

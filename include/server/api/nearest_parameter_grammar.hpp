#ifndef NEAREST_PARAMETERS_GRAMMAR_HPP
#define NEAREST_PARAMETERS_GRAMMAR_HPP

#include "server/api/base_parameters_grammar.hpp"
#include "engine/api/nearest_parameters.hpp"

#include <boost/spirit/home/x3.hpp>

namespace osrm::server::api::nearest_grammar
{

namespace x3 = boost::spirit::x3;

inline const auto number_rule =
    x3::lit("number=") >
    x3::uint_[([](auto &ctx)
               { x3::get<params_tag>(ctx).get().number_of_results = x3::_attr(ctx); })];

// Nearest root rule
inline const auto root_rule = x3::rule<struct nearest_root_tag>{"nearest_root"} =
    base_grammar::query_rule > base_grammar::format_rule >
    -('?' > (number_rule | base_grammar::base_options) % '&');

} // namespace osrm::server::api::nearest_grammar

#endif

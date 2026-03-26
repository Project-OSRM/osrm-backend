#ifndef MATCH_PARAMETERS_GRAMMAR_HPP
#define MATCH_PARAMETERS_GRAMMAR_HPP

#include "server/api/route_parameters_grammar.hpp"
#include "engine/api/match_parameters.hpp"

#include <boost/spirit/home/x3.hpp>

namespace osrm::server::api::match_grammar
{

namespace x3 = boost::spirit::x3;

inline const auto gaps_type = []()
{
    x3::symbols<engine::api::MatchParameters::GapsType> sym;
    sym.add("split", engine::api::MatchParameters::GapsType::Split)(
        "ignore", engine::api::MatchParameters::GapsType::Ignore);
    return sym;
}();

inline const auto timestamps_rule =
    x3::lit("timestamps=") >
    (x3::uint_ %
     ';')[([](auto &ctx) { x3::get<params_tag>(ctx).get().timestamps = x3::_attr(ctx); })];

inline const auto gaps_rule =
    x3::lit("gaps=") >
    gaps_type[([](auto &ctx) { x3::get<params_tag>(ctx).get().gaps = x3::_attr(ctx); })];

inline const auto tidy_rule =
    x3::lit("tidy=") >
    x3::bool_[([](auto &ctx) { x3::get<params_tag>(ctx).get().tidy = x3::_attr(ctx); })];

// Match root rule
inline const auto root_rule = x3::rule<struct match_root_tag>{"match_root"} =
    base_grammar::query_rule > base_grammar::format_rule >
    -('?' > (timestamps_rule | route_grammar::route_options | gaps_rule | tidy_rule) % '&');

} // namespace osrm::server::api::match_grammar

#endif

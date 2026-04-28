#ifndef TRIP_PARAMETERS_GRAMMAR_HPP
#define TRIP_PARAMETERS_GRAMMAR_HPP

#include "server/api/route_parameters_grammar.hpp"
#include "engine/api/trip_parameters.hpp"

#include <boost/spirit/home/x3.hpp>

namespace osrm::server::api::trip_grammar
{

namespace x3 = boost::spirit::x3;

inline const auto source_type = []()
{
    x3::symbols<engine::api::TripParameters::SourceType> sym;
    sym.add("any", engine::api::TripParameters::SourceType::Any)(
        "first", engine::api::TripParameters::SourceType::First);
    return sym;
}();

inline const auto destination_type = []()
{
    x3::symbols<engine::api::TripParameters::DestinationType> sym;
    sym.add("any", engine::api::TripParameters::DestinationType::Any)(
        "last", engine::api::TripParameters::DestinationType::Last);
    return sym;
}();

inline const auto roundtrip_rule =
    x3::lit("roundtrip=") >
    x3::bool_[([](auto &ctx) { x3::get<params_tag>(ctx).get().roundtrip = x3::_attr(ctx); })];

inline const auto source_rule =
    x3::lit("source=") >
    source_type[([](auto &ctx) { x3::get<params_tag>(ctx).get().source = x3::_attr(ctx); })];

inline const auto destination_rule =
    x3::lit("destination=") >
    destination_type[([](auto &ctx)
                      { x3::get<params_tag>(ctx).get().destination = x3::_attr(ctx); })];

// Trip root rule
inline const auto root_rule = x3::rule<struct trip_root_tag>{"trip_root"} =
    base_grammar::query_rule > base_grammar::format_rule >
    -('?' > (roundtrip_rule | source_rule | destination_rule | route_grammar::route_options) % '&');

} // namespace osrm::server::api::trip_grammar

#endif

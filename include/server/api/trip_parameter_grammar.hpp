#ifndef TRIP_PARAMETERS_GRAMMAR_HPP
#define TRIP_PARAMETERS_GRAMMAR_HPP

#include "server/api/route_parameters_grammar.hpp"
#include "engine/api/trip_parameters.hpp"

#include <boost/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

namespace osrm::server::api
{

namespace
{
namespace ph = boost::phoenix;
namespace qi = boost::spirit::qi;
} // namespace

template <typename Iterator = std::string::iterator,
          typename Signature = void(engine::api::TripParameters &)>
struct TripParametersGrammar final : public RouteParametersGrammar<Iterator, Signature>
{
    using BaseGrammar = RouteParametersGrammar<Iterator, Signature>;

    TripParametersGrammar() : BaseGrammar(root_rule)
    {
        roundtrip_rule =
            qi::lit("roundtrip=") >
            qi::bool_[ph::bind(&engine::api::TripParameters::roundtrip, qi::_r1) = qi::_1];

        source_type.add("any", engine::api::TripParameters::SourceType::Any)(
            "first", engine::api::TripParameters::SourceType::First);

        destination_type.add("any", engine::api::TripParameters::DestinationType::Any)(
            "last", engine::api::TripParameters::DestinationType::Last);

        source_rule = qi::lit("source=") >
                      source_type[ph::bind(&engine::api::TripParameters::source, qi::_r1) = qi::_1];

        destination_rule =
            qi::lit("destination=") >
            destination_type[ph::bind(&engine::api::TripParameters::destination, qi::_r1) = qi::_1];

        root_rule = BaseGrammar::query_rule(qi::_r1) > BaseGrammar::format_rule(qi::_r1) >
                    -('?' > (roundtrip_rule(qi::_r1) | source_rule(qi::_r1) |
                             destination_rule(qi::_r1) | BaseGrammar::base_rule(qi::_r1)) %
                                '&');
    }

  private:
    qi::rule<Iterator, Signature> source_rule;
    qi::rule<Iterator, Signature> destination_rule;
    qi::rule<Iterator, Signature> roundtrip_rule;
    qi::rule<Iterator, Signature> root_rule;

    qi::symbols<char, engine::api::TripParameters::SourceType> source_type;
    qi::symbols<char, engine::api::TripParameters::DestinationType> destination_type;
};
} // namespace osrm::server::api

#endif

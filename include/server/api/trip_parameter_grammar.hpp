#ifndef TRIP_PARAMETERS_GRAMMAR_HPP
#define TRIP_PARAMETERS_GRAMMAR_HPP

#include "server/api/route_parameters_grammar.hpp"
#include "engine/api/trip_parameters.hpp"

#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

namespace osrm
{
namespace server
{
namespace api
{

namespace
{
namespace ph = boost::phoenix;
namespace qi = boost::spirit::qi;
}

template <typename Iterator = std::string::iterator,
          typename Signature = void(engine::api::TripParameters &)>
struct TripParametersGrammar final : public RouteParametersGrammar<Iterator, Signature>
{
    using BaseGrammar = RouteParametersGrammar<Iterator, Signature>;

    TripParametersGrammar() : BaseGrammar(root_rule)
    {
        source_rule = (qi::lit("source=") >
                       qi::uint_)[ph::bind(&engine::api::TripParameters::source, qi::_r1) = qi::_1];

        destination_rule =
            (qi::lit("destination=") >
             qi::uint_)[ph::bind(&engine::api::TripParameters::destination, qi::_r1) = qi::_1];

        root_rule = BaseGrammar::query_rule(qi::_r1) > -qi::lit(".json") >
                    -('?' > ((source_rule(qi::_r1) | destination_rule(qi::_r1) |
                              BaseGrammar::base_rule(qi::_r1))) %
                                '&');
    }

  private:
    qi::rule<Iterator, Signature> source_rule;
    qi::rule<Iterator, Signature> destination_rule;
    qi::rule<Iterator, Signature> root_rule;
};
}
}
}

#endif

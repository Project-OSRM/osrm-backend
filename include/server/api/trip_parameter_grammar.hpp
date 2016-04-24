#ifndef TRIP_PARAMETERS_GRAMMAR_HPP
#define TRIP_PARAMETERS_GRAMMAR_HPP

#include "engine/api/trip_parameters.hpp"
#include "server/api/route_parameters_grammar.hpp"

#include <boost/spirit/include/qi.hpp>

namespace osrm
{
namespace server
{
namespace api
{

namespace
{
namespace qi = boost::spirit::qi;
}

template <typename Iterator = std::string::iterator,
          typename Signature = void(engine::api::TripParameters &)>
struct TripParametersGrammar final : public RouteParametersGrammar<Iterator, Signature>
{
    using BaseGrammar = RouteParametersGrammar<Iterator, Signature>;

    TripParametersGrammar() : BaseGrammar(root_rule)
    {
        root_rule
            = BaseGrammar::query_rule(qi::_r1) > -qi::lit(".json")
            > -('?' > (BaseGrammar::base_rule(qi::_r1)) % '&')
            ;
    }

  private:
    qi::rule<Iterator, Signature> root_rule;
};
}
}
}

#endif

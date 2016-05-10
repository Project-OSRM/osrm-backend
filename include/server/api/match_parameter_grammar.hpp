#ifndef MATCH_PARAMETERS_GRAMMAR_HPP
#define MATCH_PARAMETERS_GRAMMAR_HPP

#include "engine/api/match_parameters.hpp"
#include "server/api/route_parameters_grammar.hpp"

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
          typename Signature = void(engine::api::MatchParameters &)>
struct MatchParametersGrammar final : public RouteParametersGrammar<Iterator, Signature>
{
    using BaseGrammar = RouteParametersGrammar<Iterator, Signature>;

    MatchParametersGrammar() : BaseGrammar(root_rule)
    {
        timestamps_rule
            = qi::lit("timestamps=")
            > (qi::uint_ % ';')[ph::bind(&engine::api::MatchParameters::timestamps, qi::_r1) = qi::_1]
            ;

        root_rule
            = BaseGrammar::query_rule(qi::_r1) > -qi::lit(".json")
            > -('?' > (timestamps_rule(qi::_r1) | BaseGrammar::base_rule(qi::_r1)) % '&')
            ;
    }

  private:
    qi::rule<Iterator, Signature> root_rule;
    qi::rule<Iterator, Signature> timestamps_rule;
};
}
}
}

#endif

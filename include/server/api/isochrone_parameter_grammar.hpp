#ifndef SERVER_API_ISOCHRONE_PARAMETERS_GRAMMAR_HPP
#define SERVER_API_ISOCHRONE_PARAMETERS_GRAMMAR_HPP

#include "server/api/base_parameters_grammar.hpp"
#include "engine/api/isochrone_parameters.hpp"

#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

#include <string>

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
          typename Signature = void(engine::api::IsochroneParameters &)>
struct IsochroneParametersGrammar final : public BaseParametersGrammar<Iterator, Signature>
{
    using BaseGrammar = BaseParametersGrammar<Iterator, Signature>;

    IsochroneParametersGrammar() : BaseGrammar(root_rule)
    {
        range_rule =
            (qi::lit("range=") >
             qi::uint_)[ph::bind(&engine::api::IsochroneParameters::range, qi::_r1) = qi::_1];

        root_rule = BaseGrammar::query_rule(qi::_r1) > -qi::lit(".json") >
                    -('?' > (range_rule(qi::_r1) | BaseGrammar::base_rule(qi::_r1)) % '&');
    }

  private:
    qi::rule<Iterator, Signature> root_rule;
    qi::rule<Iterator, Signature> range_rule;
};
}
}
}

#endif

//
// Created by robin on 4/13/16.
//

#ifndef OSRM_ISOCHRONE_PARAMETER_GRAMMAR_HPP
#define OSRM_ISOCHRONE_PARAMETER_GRAMMAR_HPP

#include "engine/api/isochrone_parameters.hpp"
#include "server/api/base_parameters_grammar.hpp"

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
          typename Signature = void(engine::api::IsochroneParameters &)>
struct IsochroneParametersGrammar final : public BaseParametersGrammar<Iterator, Signature>
{
    using BaseGrammar = BaseParametersGrammar<Iterator, Signature>;

    IsochroneParametersGrammar() : BaseGrammar(root_rule)
    {

        isochrone_rule = (qi::lit("distance=") >
                          qi::uint_)[ph::bind(&engine::api::IsochroneParameters::distance,
                                              qi::_r1) = qi::_1];

        root_rule = BaseGrammar::query_rule(qi::_r1) > -qi::lit(".json") >
                    -('?' > (isochrone_rule(qi::_r1) | BaseGrammar::base_rule(qi::_r1)) % '&');
    }

  private:
    qi::rule<Iterator, Signature> root_rule;
    qi::rule<Iterator, Signature> isochrone_rule;
};
}
}
}

#endif // OSRM_ISOCHRONE_PARAMETER_GRAMMAR_HPP

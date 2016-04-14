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

namespace qi = boost::spirit::qi;

struct IsochroneParametersGrammar final : public BaseParametersGrammar
{
    using Iterator = std::string::iterator;

    IsochroneParametersGrammar() : BaseParametersGrammar(root_rule, parameters)
    {
        const auto set_number = [this](const unsigned number)
        {
            parameters.number_of_results = number;
        };
        isochrone_rule = (qi::lit("number=") > qi::uint_)[set_number];
        root_rule =
            query_rule > -qi::lit(".json") > -(qi::lit("?") > (isochrone_rule | base_rule) % '&');
    }

    engine::api::IsochroneParameters parameters;

  private:
    qi::rule<Iterator> root_rule;
    qi::rule<Iterator> isochrone_rule;
};
}
}
}

#endif // OSRM_ISOCHRONE_PARAMETER_GRAMMAR_HPP

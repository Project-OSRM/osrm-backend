#ifndef MATCH_PARAMETERS_GRAMMAR_HPP
#define MATCH_PARAMETERS_GRAMMAR_HPP

#include "engine/api/match_parameters.hpp"

#include "server/api/base_parameters_grammar.hpp"

#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_uint.hpp>
#include <boost/spirit/include/qi_grammar.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/qi_optional.hpp>

namespace osrm
{
namespace server
{
namespace api
{

namespace qi = boost::spirit::qi;

struct MatchParametersGrammar final : public BaseParametersGrammar
{
    using Iterator = std::string::iterator;

    MatchParametersGrammar() : BaseParametersGrammar(root_rule, parameters)
    {
        root_rule = "TODO(daniel-j-h)";
    }

    engine::api::MatchParameters parameters;

  private:
    qi::rule<Iterator> root_rule, match_rule;
};
}
}
}

#endif

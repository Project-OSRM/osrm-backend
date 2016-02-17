#ifndef NEAREST_PARAMETERS_GRAMMAR_HPP
#define NEAREST_PARAMETERS_GRAMMAR_HPP

#include "engine/api/nearest_parameters.hpp"

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

struct NearestParametersGrammar final : public BaseParametersGrammar
{
    using Iterator = std::string::iterator;

    NearestParametersGrammar() : BaseParametersGrammar(root_rule, parameters)
    {
        root_rule = "TODO(daniel-j-h)";
    }

    engine::api::NearestParameters parameters;

  private:
    qi::rule<Iterator> root_rule, nearest_rule;
};
}
}
}

#endif

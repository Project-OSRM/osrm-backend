#ifndef TRIP_PARAMETERS_GRAMMAR_HPP
#define TRIP_PARAMETERS_GRAMMAR_HPP

#include "engine/api/trip_parameters.hpp"

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

struct TripParametersGrammar final : public BaseParametersGrammar
{
    using Iterator = std::string::iterator;

    TripParametersGrammar() : BaseParametersGrammar(root_rule, parameters)
    {
        root_rule = "TODO(daniel-j-h)";
    }

    engine::api::TripParameters parameters;

  private:
    qi::rule<Iterator> root_rule, trip_rule;
};
}
}
}

#endif

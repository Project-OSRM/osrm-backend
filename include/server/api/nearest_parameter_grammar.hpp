#ifndef NEAREST_PARAMETERS_GRAMMAR_HPP
#define NEAREST_PARAMETERS_GRAMMAR_HPP

#include "engine/api/nearest_parameters.hpp"

//#define BOOST_SPIRIT_DEBUG
#include "server/api/base_parameters_grammar.hpp"

#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/qi_grammar.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_optional.hpp>
#include <boost/spirit/include/qi_uint.hpp>

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
        const auto set_number = [this](const unsigned number) {
            parameters.number_of_results = number;
        };
        nearest_rule = (qi::lit("number=") >> qi::uint_)[set_number];
        root_rule = query_rule >> -qi::lit(".") >> -qi::lit("json") >>
                    -(qi::lit("?") >> (nearest_rule | base_rule) % '&');
    }

    engine::api::NearestParameters parameters;

  private:
    qi::rule<Iterator> root_rule;
    qi::rule<Iterator> nearest_rule;
};
}
}
}

#endif

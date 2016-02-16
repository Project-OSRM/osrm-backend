#ifndef SERVER_API_BASE_PARAMETERS_GRAMMAR_HPP
#define SERVER_API_BASE_PARAMETERS_GRAMMAR_HPP

#include "engine/api/base_parameters.hpp"

#include "engine/polyline_compressor.hpp"
#include "engine/hint.hpp"

#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_char_.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/qi_real.hpp>
#include <boost/spirit/include/qi_grammar.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/qi_optional.hpp>
#include <boost/spirit/include/qi_attr_cast.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_repeat.hpp>
#include <boost/spirit/include/qi_as_string.hpp>

#include <string>

namespace osrm
{
namespace server
{
namespace api
{

namespace qi = boost::spirit::qi;
struct BaseParametersGrammar : boost::spirit::qi::grammar<std::string::iterator>
{
    using Iterator = std::string::iterator;
    using RadiusesT = std::vector<boost::optional<double>>;

    BaseParametersGrammar(qi::rule<Iterator> &child_rule,
                          engine::api::BaseParameters &parameters_)
        : BaseParametersGrammar::base_type(child_rule), base_parameters(parameters_)
    {
        const auto add_bearing = [this](const boost::optional<boost::fusion::vector<short, short>> &bearing_range) {
            boost::optional<engine::api::BaseParameters::Bearing> bearing;
            if (bearing_range)
            {
                bearing = engine::api::BaseParameters::Bearing {boost::fusion::at_c<0>(*bearing_range), boost::fusion::at_c<1>(*bearing_range)};
            }
            base_parameters.bearings.push_back(std::move(bearing));
        };
        const auto set_radiuses = [this](RadiusesT& radiuses) {
            base_parameters.radiuses = std::move(radiuses);
        };
        const auto add_hint = [this](const std::string& hint_string) {
            if (hint_string.size() > 0)
            {
                base_parameters.hints.push_back(engine::Hint::FromBase64(hint_string));
            }
        };

        alpha_numeral = +qi::char_("a-zA-Z0-9");
        base64_char = qi::char_("a-zA-Z0-9--_");

        radiuses_rule = qi::lit("radiuses=") >> -qi::double_ % ";";
        hints_rule = qi::lit("hints=") >> qi::as_string[qi::repeat(engine::ENCODED_HINT_SIZE)[base64_char]][add_hint] % ";";
        bearings_rule =
            qi::lit("bearings=") >> (-(qi::short_ >> ',' >> qi::short_))[add_bearing] % ";";
        base_rule = bearings_rule | radiuses_rule[set_radiuses] | hints_rule;
    }

    qi::rule<Iterator> base_rule;

private:
    engine::api::BaseParameters &base_parameters;
    qi::rule<Iterator> bearings_rule;
    qi::rule<Iterator> hints_rule;
    qi::rule<Iterator, RadiusesT()> radiuses_rule;
    qi::rule<Iterator, unsigned char()> base64_char;
    qi::rule<Iterator, std::string()> alpha_numeral;
};
}
}
}

#endif

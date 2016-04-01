#include "server/api/url_parser.hpp"

#include "engine/polyline_compressor.hpp"

#include <boost/bind.hpp>
#include <boost/spirit/include/qi_char_.hpp>
#include <boost/spirit/include/qi_grammar.hpp>
#include <boost/spirit/include/qi_uint.hpp>
#include <boost/spirit/include/qi_real.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/qi_as_string.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_plus.hpp>

namespace osrm
{
namespace server
{
namespace api
{

namespace
{

namespace qi = boost::spirit::qi;
using Iterator = std::string::iterator;
struct URLGrammar : boost::spirit::qi::grammar<Iterator>
{
    URLGrammar() : URLGrammar::base_type(url_rule)
    {
        const auto set_service = [this](std::string &service)
        {
            parsed_url.service = std::move(service);
        };
        const auto set_version = [this](const unsigned version)
        {
            parsed_url.version = version;
        };
        const auto set_profile = [this](std::string &profile)
        {
            parsed_url.profile = std::move(profile);
        };
        const auto set_query = [this](std::string &query)
        {
            parsed_url.query = std::move(query);
        };

        alpha_numeral = qi::char_("a-zA-Z0-9");
        polyline_chars = qi::char_("a-zA-Z0-9_.--[]{}@?|\\%~`^");
        all_chars = polyline_chars | qi::char_("=,;:&().");

        service_rule = +alpha_numeral;
        version_rule = qi::uint_;
        profile_rule = +alpha_numeral;
        query_rule = +all_chars;

        url_rule = qi::lit('/') >> service_rule[set_service]                    //
                   >> qi::lit('/') >> qi::lit('v') >> version_rule[set_version] //
                   >> qi::lit('/') >> profile_rule[set_profile]                 //
                   >> qi::lit('/') >> query_rule[set_query];
    }

    ParsedURL parsed_url;

    qi::rule<Iterator> url_rule;
    qi::rule<Iterator, std::string()> service_rule, profile_rule, query_rule;
    qi::rule<Iterator, unsigned()> version_rule;
    qi::rule<Iterator, char()> alpha_numeral, all_chars, polyline_chars;
};
}

boost::optional<ParsedURL> parseURL(std::string::iterator &iter, const std::string::iterator end)
{
    boost::optional<ParsedURL> parsed_url;

    URLGrammar grammar;
    const auto result = boost::spirit::qi::parse(iter, end, grammar);

    if (result && iter == end)
    {
        parsed_url = std::move(grammar.parsed_url);
    }

    return parsed_url;
}
}
}
}

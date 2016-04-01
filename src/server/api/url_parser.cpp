#include "server/api/url_parser.hpp"
#include "engine/polyline_compressor.hpp"

//#define BOOST_SPIRIT_DEBUG
#include <boost/spirit/include/qi.hpp>

#include <string>
#include <type_traits>

// Keep impl. TU local
namespace
{
namespace qi = boost::spirit::qi;

template <typename Iterator, typename Into> //
struct URLParser final : qi::grammar<Iterator, Into>
{

    URLParser() : URLParser::base_type(start)
    {
        alpha_numeral = qi::char_("a-zA-Z0-9");
        polyline_chars = qi::char_("a-zA-Z0-9_.--[]{}@?|\\%~`^");
        all_chars = polyline_chars | qi::char_("=,;:&().");

        service = +alpha_numeral;
        version = qi::uint_;
        profile = +alpha_numeral;
        query = +all_chars;

        // Example input: /route/v1/driving/7.416351,43.731205;7.420363,43.736189

        start = qi::lit('/') >> service                    //
                >> qi::lit('/') >> qi::lit('v') >> version //
                >> qi::lit('/') >> profile                 //
                >> qi::lit('/') >> query;                  //

        BOOST_SPIRIT_DEBUG_NODES((start)(service)(version)(profile)(query))
    }

    qi::rule<Iterator, Into> start;

    qi::rule<Iterator, std::string()> service;
    qi::rule<Iterator, unsigned()> version;
    qi::rule<Iterator, std::string()> profile;
    qi::rule<Iterator, std::string()> query;

    qi::rule<Iterator, char()> alpha_numeral;
    qi::rule<Iterator, char()> all_chars;
    qi::rule<Iterator, char()> polyline_chars;
};

} // anon.

namespace osrm
{
namespace server
{
namespace api
{

boost::optional<ParsedURL> parseURL(std::string::iterator &iter, const std::string::iterator end)
{
    using It = std::decay<decltype(iter)>::type;

    static URLParser<It, ParsedURL> const parser;
    ParsedURL out;

    const auto ok = boost::spirit::qi::parse(iter, end, parser, out);

    if (ok && iter == end)
        return boost::make_optional(out);

    return boost::none;
}

} // api
} // server
} // osrm

#include "server/api/url_parser.hpp"
#include "engine/polyline_compressor.hpp"

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/repository/include/qi_iter_pos.hpp>

#include <string>
#include <type_traits>

BOOST_FUSION_ADAPT_STRUCT(osrm::server::api::ParsedURL,
                          (std::string, service)(unsigned, version)(std::string,
                                                                    profile)(std::string, query))

// Keep impl. TU local
namespace
{
namespace ph = boost::phoenix;
namespace qi = boost::spirit::qi;

template <typename Iterator, typename Into> //
struct URLParser final : qi::grammar<Iterator, Into>
{
    URLParser() : URLParser::base_type(start)
    {
        using boost::spirit::repository::qi::iter_pos;

        alpha_numeral = qi::char_("a-zA-Z0-9");
        percent_encoding =
            qi::char_('%') > qi::uint_parser<unsigned char, 16, 2, 2>()[qi::_val = qi::_1];
        polyline_chars = qi::char_("a-zA-Z0-9_.--[]{}@?|\\~`^") | percent_encoding;
        all_chars = polyline_chars | qi::char_("=,;:&().");

        service = +alpha_numeral;
        version = qi::uint_;
        profile = +alpha_numeral;
        query = +all_chars;

        // Example input: /route/v1/driving/7.416351,43.731205;7.420363,43.736189

        start = qi::lit('/') > service > qi::lit('/') > qi::lit('v') > version > qi::lit('/') >
                profile > qi::lit('/') >
                qi::omit[iter_pos[ph::bind(&osrm::server::api::ParsedURL::prefix_length, qi::_val) =
                                      qi::_1 - qi::_r1]] > query;

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
    qi::rule<Iterator, char()> percent_encoding;
};

} // namespace

namespace osrm
{
namespace server
{
namespace api
{

boost::optional<ParsedURL> parseURL(std::string::iterator &iter, const std::string::iterator end)
{
    using It = std::decay<decltype(iter)>::type;

    static URLParser<It, ParsedURL(It)> const parser;
    ParsedURL out;

    try
    {
        const auto ok = boost::spirit::qi::parse(iter, end, parser(boost::phoenix::val(iter)), out);

        if (ok && iter == end)
            return boost::make_optional(out);
    }
    catch (const qi::expectation_failure<It> &failure)
    {
        // The grammar above using expectation parsers ">" does not automatically increment the
        // iterator to the failing position. Extract the position from the exception ourselves.
        iter = failure.first;
    }

    return boost::none;
}

} // namespace api
} // namespace server
} // namespace osrm

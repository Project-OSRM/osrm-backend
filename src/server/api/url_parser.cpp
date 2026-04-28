#include "server/api/url_parser.hpp"

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>

#include <string>

BOOST_FUSION_ADAPT_STRUCT(osrm::server::api::ParsedURL, service, version, profile, query)

// Keep impl. TU local
namespace
{
namespace x3 = boost::spirit::x3;

const x3::uint_parser<unsigned char, 16, 2, 2> hex2{};

const auto percent_encoding = x3::rule<struct percent_encoding_tag, char>{"percent_encoding"} =
    x3::lit('%') > hex2;

const auto identifier = x3::char_("a-zA-Z0-9_.~:-");
const auto polyline_chars = x3::char_("a-zA-Z0-9_[]{}@?|\\~`^") | percent_encoding;
const auto all_chars = polyline_chars | x3::char_("=,;:&().-");

const auto service = x3::rule<struct service_tag, std::string>{"service"} = +identifier;
const auto version = x3::uint_;
const auto profile = x3::rule<struct profile_tag, std::string>{"profile"} = +identifier;
const auto query = x3::rule<struct query_tag, std::string>{"query"} = +all_chars;

// Example input: /route/v1/driving/7.416351,43.731205;7.420363,43.736189
const auto url_parser = x3::rule<struct url_parser_tag, osrm::server::api::ParsedURL>{"url"} =
    x3::lit('/') > service > x3::lit('/') > x3::lit('v') > version > x3::lit('/') > profile
    > x3::lit('/') > query;

std::size_t countDigits(unsigned v)
{
    std::size_t count = 1;
    while (v >= 10)
    {
        v /= 10;
        ++count;
    }
    return count;
}

} // namespace

namespace osrm::server::api
{

std::optional<ParsedURL> parseURL(std::string::iterator &iter, const std::string::iterator end)
{
    ParsedURL out;

    try
    {
        const auto ok = x3::parse(iter, end, url_parser, out);

        if (ok && iter == end)
        {
            // prefix = /{service}/v{version}/{profile}/
            out.prefix_length =
                1 + out.service.size() + 2 + countDigits(out.version) + 1 + out.profile.size() + 1;
            return std::make_optional(out);
        }
    }
    catch (const x3::expectation_failure<std::string::iterator> &failure)
    {
        // The grammar above using expectation parsers ">" does not automatically increment the
        // iterator to the failing position. Extract the position from the exception ourselves.
        iter = failure.where();
    }

    return std::nullopt;
}

} // namespace osrm::server::api

#include "server/api/url_parser.hpp"
#include "engine/polyline_compressor.hpp"

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/optional.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>

#include <string>

BOOST_FUSION_ADAPT_STRUCT(osrm::server::api::ParsedURL,
                          (std::string, service)(unsigned, version)(std::string,
                                                                    profile)(std::string, query))

namespace osrm::server::api
{
namespace x3 = boost::spirit::x3;

struct ParsedURLClass : x3::annotate_on_success
{
};
const x3::rule<struct Service, std::string> service = "service";
const x3::rule<struct Version, unsigned> version = "version";
const x3::rule<struct Profile, std::string> profile = "profile";
const x3::rule<struct Query, std::string> query = "query";
const x3::rule<struct ParsedURL, ParsedURL> start = "start";

const auto identifier = x3::char_("a-zA-Z0-9_.~:-");
const auto percent_encoding = x3::char_('%') >> x3::uint_parser<unsigned char, 16, 2, 2>();
const auto polyline_chars = x3::char_("a-zA-Z0-9_[]{}@?|\\~`^") | percent_encoding;
const auto all_chars = polyline_chars | x3::char_("=,;:&().-");

const auto service_def = +identifier;
const auto version_def = x3::uint_;
const auto profile_def = +identifier;
const auto query_def = +all_chars;

const auto start_def = x3::lit('/') > service > x3::lit('/') > x3::lit('v') > version
                       > x3::lit('/') > profile > x3::lit('/') > query;

BOOST_SPIRIT_DEFINE(service, version, profile, query, start)

boost::optional<ParsedURL> parseURL(std::string::iterator &iter, const std::string::iterator end)
{
    ParsedURL out;

    try
    {
        auto iter_copy = iter;
        bool r = x3::phrase_parse(iter_copy, end, start, x3::space, out);

        if (r && iter_copy == end)
        {
            iter = iter_copy;
            // TODO: find a way to do it more effective
            std::string parsed_part = "/" + out.service + "/v" + std::to_string(out.version) + "/" + out.profile + "/";
            out.prefix_length = parsed_part.length();
            return boost::make_optional(out);
        }
    }
    catch (const x3::expectation_failure<std::string::iterator> &failure)
    {
        // The grammar above using expectation parsers ">" does not automatically increment the
        // iterator to the failing position. Extract the position from the exception ourselves.
        iter = failure.where();
    }

    return boost::none;
}

} // namespace osrm::server::api

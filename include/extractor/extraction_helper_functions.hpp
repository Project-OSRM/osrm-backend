#ifndef EXTRACTION_HELPER_FUNCTIONS_HPP
#define EXTRACTION_HELPER_FUNCTIONS_HPP

#include <boost/fusion/include/at_c.hpp>
#include <boost/spirit/home/x3.hpp>

#include <boost/algorithm/string/replace.hpp>

#include <algorithm>
#include <cctype>
#include <iterator>
#include <limits>
#include <string>

#include "guidance/parsing_toolkit.hpp"

namespace osrm::extractor
{

namespace detail
{

namespace x3 = boost::spirit::x3;

inline x3::uint_parser<unsigned, 10, 1, 2> const uint_p{};
inline x3::uint_parser<unsigned, 10, 2, 2> const uint2_p{};

inline const auto hh = x3::rule<struct hh_tag, unsigned>{"hh"} = uint2_p[(
    [](auto &ctx)
    {
        if (x3::_attr(ctx) >= 24)
        {
            x3::_pass(ctx) = false;
            return;
        }
        x3::_val(ctx) = x3::_attr(ctx);
    })];

inline const auto mm = x3::rule<struct mm_tag, unsigned>{"mm"} = uint2_p[(
    [](auto &ctx)
    {
        if (x3::_attr(ctx) >= 60)
        {
            x3::_pass(ctx) = false;
            return;
        }
        x3::_val(ctx) = x3::_attr(ctx);
    })];

inline const auto ss = x3::rule<struct ss_tag, unsigned>{"ss"} = uint2_p[(
    [](auto &ctx)
    {
        if (x3::_attr(ctx) >= 60)
        {
            x3::_pass(ctx) = false;
            return;
        }
        x3::_val(ctx) = x3::_attr(ctx);
    })];

// "H:M:S" or "H:M" or "M" (bare number = minutes)
inline const auto osm_time = x3::rule<struct osm_time_tag, unsigned>{"osm_time"} =
    (uint_p >> ':' >> uint_p >> ':' >> uint_p >> x3::eoi)[(
        [](auto &ctx)
        {
            auto &a = x3::_attr(ctx);
            x3::_val(ctx) = boost::fusion::at_c<0>(a) * 3600 + boost::fusion::at_c<1>(a) * 60 +
                            boost::fusion::at_c<2>(a);
        })] |
    (uint_p >> ':' >> uint_p >> x3::eoi)[(
        [](auto &ctx)
        {
            auto &a = x3::_attr(ctx);
            x3::_val(ctx) = boost::fusion::at_c<0>(a) * 3600 + boost::fusion::at_c<1>(a) * 60;
        })] |
    (uint_p >> x3::eoi)[([](auto &ctx) { x3::_val(ctx) = x3::_attr(ctx) * 60; })];

// Thhmmss (compact)
inline const auto alternative_time = x3::rule<struct alt_time_tag, unsigned>{"alternative_time"} =
    ('T' >> hh >> mm >> ss)[(
        [](auto &ctx)
        {
            auto &a = x3::_attr(ctx);
            x3::_val(ctx) = boost::fusion::at_c<0>(a) * 3600 + boost::fusion::at_c<1>(a) * 60 +
                            boost::fusion::at_c<2>(a);
        })];

// Thh:mm:ss (extended)
inline const auto extended_time = x3::rule<struct ext_time_tag, unsigned>{"extended_time"} =
    ('T' >> hh >> ':' >> mm >> ':' >> ss)[(
        [](auto &ctx)
        {
            auto &a = x3::_attr(ctx);
            x3::_val(ctx) = boost::fusion::at_c<0>(a) * 3600 + boost::fusion::at_c<1>(a) * 60 +
                            boost::fusion::at_c<2>(a);
        })];

// T[nH][nM][nS] (ISO 8601 designator form)
inline const auto standard_time = x3::rule<struct std_time_tag, unsigned>{"standard_time"} =
    ('T' >> -(x3::uint_ >> x3::char_("Hh")) >> -(x3::uint_ >> x3::char_("Mm")) >>
     -(x3::uint_ >> x3::char_("Ss")))[(
        [](auto &ctx)
        {
            auto &a = x3::_attr(ctx);
            unsigned h = 0, m = 0, s = 0;
            if (auto &oh = boost::fusion::at_c<0>(a))
                h = boost::fusion::at_c<0>(*oh);
            if (auto &om = boost::fusion::at_c<1>(a))
                m = boost::fusion::at_c<0>(*om);
            if (auto &os = boost::fusion::at_c<2>(a))
                s = boost::fusion::at_c<0>(*os);
            x3::_val(ctx) = h * 3600 + m * 60 + s;
        })];

inline const auto standard_date = x3::rule<struct std_date_tag, unsigned>{"standard_date"} =
    (x3::uint_ >> x3::char_("Dd"))[(
        [](auto &ctx) { x3::_val(ctx) = boost::fusion::at_c<0>(x3::_attr(ctx)) * 86400; })];

inline const auto standard_week = x3::rule<struct std_week_tag, unsigned>{"standard_week"} =
    (x3::uint_ >> x3::char_("Ww"))[(
        [](auto &ctx) { x3::_val(ctx) = boost::fusion::at_c<0>(x3::_attr(ctx)) * 604800; })];

// Collapse date+time into single value
inline const auto date_time_duration = x3::rule<struct dt_dur_tag, unsigned>{"date_time_duration"} =
    (-standard_date >> -standard_time)[(
        [](auto &ctx)
        {
            auto &a = x3::_attr(ctx);
            unsigned d = 0, t = 0;
            if (boost::fusion::at_c<0>(a))
                d = *boost::fusion::at_c<0>(a);
            if (boost::fusion::at_c<1>(a))
                t = *boost::fusion::at_c<1>(a);
            x3::_val(ctx) = d + t;
        })];

// Collapse time alternatives into single typed rule
inline const auto time_or_datetime = x3::rule<struct tod_tag, unsigned>{"time_or_datetime"} =
    alternative_time | extended_time | date_time_duration;

inline const auto iso_period = x3::rule<struct iso_period_tag, unsigned>{"iso_period"} =
    osm_time | ('P' >> standard_week >> x3::eoi) | ('P' >> time_or_datetime >> x3::eoi);

} // namespace detail

inline bool durationIsValid(const std::string &s)
{
    namespace x3 = boost::spirit::x3;
    std::string::const_iterator iter = s.begin();
    unsigned duration = 0;
    x3::parse(iter, s.end(), detail::iso_period, duration);

    return !s.empty() && iter == s.end();
}

inline unsigned parseDuration(const std::string &s)
{
    namespace x3 = boost::spirit::x3;
    std::string::const_iterator iter = s.begin();
    unsigned duration = 0;
    x3::parse(iter, s.end(), detail::iso_period, duration);

    return !s.empty() && iter == s.end() ? duration : std::numeric_limits<unsigned>::max();
}

inline std::string
trimLaneString(std::string lane_string, std::int32_t count_left, std::int32_t count_right)
{
    return guidance::trimLaneString(std::move(lane_string), count_left, count_right);
}

inline std::string applyAccessTokens(const std::string &lane_string,
                                     const std::string &access_tokens)
{
    return guidance::applyAccessTokens(lane_string, access_tokens);
}

// Takes a string representing a list separated by delim and canonicalizes containing spaces.
// Example: "aaa;bbb; ccc;  d;dd" => "aaa; bbb; ccc; d; dd"
inline std::string canonicalizeStringList(std::string strlist, const std::string &delim)
{
    // expand space after delimiter: ";X" => "; X"
    boost::replace_all(strlist, delim, delim + " ");

    // collapse spaces; this is needed in case we expand "; X" => ";  X" above
    // but also makes sense to do irregardless of the fact - canonicalizing strings.
    const auto spaces = [](unsigned char lhs, unsigned char rhs)
    { return ::isspace(lhs) && ::isspace(rhs); };
    auto it = std::unique(begin(strlist), end(strlist), spaces);
    strlist.erase(it, end(strlist));

    return strlist;
}

} // namespace osrm::extractor

#endif // EXTRACTION_HELPER_FUNCTIONS_HPP

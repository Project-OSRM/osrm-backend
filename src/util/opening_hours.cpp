#include "util/opening_hours.hpp"

#include <boost/fusion/include/at_c.hpp>
#include <boost/spirit/home/x3.hpp>

#include <boost/io/ios_state.hpp>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iterator>

namespace osrm::util
{

#ifndef NDEBUG
// Debug output stream operators for use with BOOST_SPIRIT_DEBUG
inline std::ostream &operator<<(std::ostream &stream, const OpeningHours::Modifier value)
{
    switch (value)
    {
    case OpeningHours::unknown:
        return stream << "unknown";
    case OpeningHours::open:
        return stream << "open";
    case OpeningHours::closed:
        return stream << "closed";
    case OpeningHours::off:
        return stream << "off";
    case OpeningHours::is24_7:
        return stream << "24/7";
    }
    return stream;
}

inline std::ostream &operator<<(std::ostream &stream, const OpeningHours::Time::Event value)
{
    switch (value)
    {
    case OpeningHours::Time::dawn:
        return stream << "dawn";
    case OpeningHours::Time::sunrise:
        return stream << "sunrise";
    case OpeningHours::Time::sunset:
        return stream << "sunset";
    case OpeningHours::Time::dusk:
        return stream << "dusk";
    default:
        break;
    }
    return stream;
}

inline std::ostream &operator<<(std::ostream &stream, const OpeningHours::Time &value)
{
    boost::io::ios_flags_saver ifs(stream);
    if (value.event == OpeningHours::Time::invalid)
        return stream << "???";
    if (value.event == OpeningHours::Time::none)
        return stream << std::setfill('0') << std::setw(2) << value.minutes / 60 << ":"
                      << std::setfill('0') << std::setw(2) << value.minutes % 60;
    stream << value.event;
    if (value.minutes != 0)
        stream << value.minutes;
    return stream;
}

inline std::ostream &operator<<(std::ostream &stream, const OpeningHours::TimeSpan &value)
{
    return stream << value.from << "-" << value.to;
}

inline std::ostream &operator<<(std::ostream &stream, const OpeningHours::Monthday &value)
{
    bool empty = true;
    if (value.year != 0)
    {
        stream << (int)value.year;
        empty = false;
    };
    if (value.month != 0)
    {
        stream << (empty ? "" : "/") << (int)value.month;
        empty = false;
    };
    if (value.day != 0)
    {
        stream << (empty ? "" : "/") << (int)value.day;
    };
    return stream;
}

inline std::ostream &operator<<(std::ostream &stream, const OpeningHours::WeekdayRange &value)
{
    boost::io::ios_flags_saver ifs(stream);
    return stream << std::hex << std::setfill('0') << std::setw(2) << value.weekdays;
}

inline std::ostream &operator<<(std::ostream &stream, const OpeningHours::MonthdayRange &value)
{
    return stream << value.from << "-" << value.to;
}

inline std::ostream &operator<<(std::ostream &stream, const OpeningHours &value)
{
    if (value.modifier == OpeningHours::is24_7)
        return stream << OpeningHours::is24_7;

    for (auto x : value.monthdays)
        stream << x << ", ";
    for (auto x : value.weekdays)
        stream << x << ", ";
    for (auto x : value.times)
        stream << x << ", ";
    return stream << " |" << value.modifier << "|";
}
#endif

namespace detail
{

namespace x3 = boost::spirit::x3;
namespace fusion = boost::fusion;
using oh = OpeningHours;

// --- Parsers ---
inline x3::uint_parser<unsigned, 10, 2, 2> const uint2_p{};
inline x3::uint_parser<unsigned, 10, 4, 4> const uint4_p{};

// --- Symbol tables ---
inline const auto wday = []()
{
    x3::symbols<unsigned char> sym;
    sym.add("Su", 0)("Mo", 1)("Tu", 2)("We", 3)("Th", 4)("Fr", 5)("Sa", 6);
    return sym;
}();

inline const auto month_sym = []()
{
    x3::symbols<unsigned char> sym;
    sym.add("Jan", 1)("Feb", 2)("Mar", 3)("Apr", 4)("May", 5)("Jun", 6)("Jul", 7)("Aug", 8)(
        "Sep", 9)("Oct", 10)("Nov", 11)("Dec", 12);
    return sym;
}();

inline const auto event = []()
{
    x3::symbols<oh::Time::Event> sym;
    sym.add("dawn", oh::Time::dawn)("sunrise", oh::Time::sunrise)("sunset", oh::Time::sunset)(
        "dusk", oh::Time::dusk);
    return sym;
}();

inline const auto rule_modifier = []()
{
    x3::symbols<oh::Modifier> sym;
    sym.add("unknown", oh::unknown)("open", oh::open)("closed", oh::closed)("off", oh::off);
    return sym;
}();

// --- Validated basic rules ---

inline const auto hour = x3::rule<struct hour_tag, unsigned>{"hour"} = uint2_p[(
    [](auto &ctx)
    {
        if (x3::_attr(ctx) > 24)
        {
            x3::_pass(ctx) = false;
            return;
        }
        x3::_val(ctx) = x3::_attr(ctx);
    })];

inline const auto extended_hour = x3::rule<struct ext_hour_tag, unsigned>{"extended_hour"} =
    uint2_p[(
        [](auto &ctx)
        {
            if (x3::_attr(ctx) > 48)
            {
                x3::_pass(ctx) = false;
                return;
            }
            x3::_val(ctx) = x3::_attr(ctx);
        })];

inline const auto oh_minute = x3::rule<struct minute_tag, unsigned>{"minute"} = uint2_p[(
    [](auto &ctx)
    {
        if (x3::_attr(ctx) >= 60)
        {
            x3::_pass(ctx) = false;
            return;
        }
        x3::_val(ctx) = x3::_attr(ctx);
    })];

inline const auto daynum = x3::rule<struct daynum_tag, unsigned>{"daynum"} =
    uint2_p[(
        [](auto &ctx)
        {
            auto v = x3::_attr(ctx);
            if (v < 1 || v > 31)
            {
                x3::_pass(ctx) = false;
                return;
            }
            x3::_val(ctx) = v;
        })] >>
    !x3::lexeme[x3::lit(':') >> uint2_p]; // distinguish from hour:minute

inline const auto weeknum = x3::rule<struct weeknum_tag, unsigned>{"weeknum"} = uint2_p[(
    [](auto &ctx)
    {
        auto v = x3::_attr(ctx);
        if (v < 1 || v > 53)
        {
            x3::_pass(ctx) = false;
            return;
        }
        x3::_val(ctx) = v;
    })];

inline const auto year_val = x3::rule<struct year_tag, unsigned>{"year"} = uint4_p[(
    [](auto &ctx)
    {
        if (x3::_attr(ctx) <= 1900)
        {
            x3::_pass(ctx) = false;
            return;
        }
        x3::_val(ctx) = x3::_attr(ctx);
    })];

inline const auto plus_or_minus = x3::rule<struct pm_tag, bool>{"plus_or_minus"} =
    x3::lit('+')[([](auto &ctx) { x3::_val(ctx) = true; })] |
    x3::lit('-')[([](auto &ctx) { x3::_val(ctx) = false; })];

inline const auto comment = x3::rule<struct comment_tag>{"comment"} =
    x3::omit[x3::lit('"') >> *(~x3::char_('"')) >> x3::lit('"')];

// --- Time rules ---

inline const auto hour_minutes = x3::rule<struct hm_tag, oh::Time>{"hour_minutes"} =
    (hour >> ':' >> oh_minute)[(
        [](auto &ctx)
        {
            auto &a = x3::_attr(ctx);
            x3::_val(ctx) = oh::Time(static_cast<char>(fusion::at_c<0>(a)),
                                     static_cast<char>(fusion::at_c<1>(a)));
        })];

inline const auto extended_hour_minutes =
    x3::rule<struct ehm_tag, oh::Time>{"extended_hour_minutes"} =
        (extended_hour >> ':' >> oh_minute)[(
            [](auto &ctx)
            {
                auto &a = x3::_attr(ctx);
                x3::_val(ctx) = oh::Time(static_cast<char>(fusion::at_c<0>(a)),
                                         static_cast<char>(fusion::at_c<1>(a)));
            })];

inline const auto variable_time = x3::rule<struct vt_tag, oh::Time>{"variable_time"} =
    event[([](auto &ctx) { x3::_val(ctx) = oh::Time(x3::_attr(ctx)); })] |
    ('(' >> event >> plus_or_minus >> hour_minutes >> ')')[(
        [](auto &ctx)
        {
            auto &a = x3::_attr(ctx);
            x3::_val(ctx) = oh::Time(fusion::at_c<0>(a), fusion::at_c<1>(a), fusion::at_c<2>(a));
        })];

// Collapse alternatives into typed rules
inline const auto time_rule = x3::rule<struct time_tag, oh::Time>{"time"} =
    hour_minutes | variable_time;

inline const auto extended_time_rule = x3::rule<struct ext_time_tag2, oh::Time>{"extended_time"} =
    extended_hour_minutes | variable_time;

// Collapse alternative Time producers to avoid variant<Time, Time>
inline const auto to_time_part = x3::rule<struct to_time_tag, oh::Time>{"to_time"} =
    (x3::lit('+') >> x3::attr(oh::Time(24, 0))) |
    ('-' >> extended_time_rule >>
     x3::omit[-(x3::lit('+') | ('/' >> uint2_p >> -(':' >> uint2_p)))]);

inline const auto timespan = x3::rule<struct ts_tag, oh::TimeSpan>{"timespan"} =
    (time_rule >> -to_time_part)[(
        [](auto &ctx)
        {
            auto &a = x3::_attr(ctx);
            auto &from = fusion::at_c<0>(a);
            oh::Time to{};
            if (auto &opt = fusion::at_c<1>(a))
                to = *opt;
            x3::_val(ctx) = oh::TimeSpan(from, to);
        })];

inline const auto time_selector =
    x3::rule<struct tsel_tag, std::vector<oh::TimeSpan>>{"time_selector"} = timespan % ',';

// --- Weekday rules ---

inline const auto day_offset = x3::rule<struct doff_tag>{"day_offset"} =
    x3::omit[plus_or_minus >> x3::uint_ >> x3::lit("days")];

inline const auto nth_entry = x3::rule<struct nth_tag>{"nth_entry"} =
    x3::omit[x3::char_("12345") >> -('-' >> x3::char_("12345"))] |
    x3::omit['-' >> x3::char_("12345")];

inline const auto weekday_range = x3::rule<struct wr_tag, oh::WeekdayRange>{"weekday_range"} =
    // wday-wday range
    (wday >> '-' >> wday)[(
        [](auto &ctx)
        {
            auto &a = x3::_attr(ctx);
            x3::_val(ctx) = oh::WeekdayRange(fusion::at_c<0>(a), fusion::at_c<1>(a));
        })] |
    // wday with nth specifier
    (wday >> x3::omit['[' >> (nth_entry % ',') >> ']' >> -day_offset])[(
        [](auto &ctx) { x3::_val(ctx) = oh::WeekdayRange(x3::_attr(ctx), x3::_attr(ctx)); })] |
    // single wday
    wday[([](auto &ctx) { x3::_val(ctx) = oh::WeekdayRange(x3::_attr(ctx), x3::_attr(ctx)); })];

inline const auto holiday_sequence = x3::rule<struct hol_tag>{"holiday_sequence"} =
    x3::omit[(x3::lit("SH") >> -day_offset) | x3::lit("PH")];

inline const auto weekday_sequence =
    x3::rule<struct wdseq_tag, std::vector<oh::WeekdayRange>>{"weekday_sequence"} =
        weekday_range % ',';

inline const auto weekday_selector =
    x3::rule<struct wdsel_tag, std::vector<oh::WeekdayRange>>{"weekday_selector"} =
        // holiday, then optional (comma/space + weekdays)
    (x3::omit[holiday_sequence >> -(x3::char_(", ") >> weekday_sequence)]) |
    // weekdays, then optional (comma/space + holiday)
    (weekday_sequence >> x3::omit[-(x3::char_(", ") >> holiday_sequence)]);

// --- Week rules ---

inline const auto week = x3::rule<struct week_tag>{"week"} =
    x3::omit[weeknum >> -('-' >> weeknum >> -('/' >> x3::uint_))];

inline const auto week_selector = x3::rule<struct wsel_tag>{"week_selector"} =
    x3::omit[x3::lit("week") >> week % ','];

// --- Monthday rules ---

inline const auto variable_date = x3::rule<struct vd_tag>{"variable_date"} =
    x3::omit[x3::lit("easter")];

inline const auto date_offset = x3::rule<struct do_tag>{"date_offset"} =
    x3::omit[(plus_or_minus >> wday) | day_offset];

inline const auto date_from = x3::rule<struct df_tag, oh::Monthday>{"date_from"} =
    // year month day
    (-year_val >> month_sym >> -daynum)[(
        [](auto &ctx)
        {
            auto &a = x3::_attr(ctx);
            int y = fusion::at_c<0>(a) ? static_cast<int>(*fusion::at_c<0>(a)) : 0;
            char m = static_cast<char>(fusion::at_c<1>(a));
            char d = fusion::at_c<2>(a) ? static_cast<char>(*fusion::at_c<2>(a)) : 0;
            x3::_val(ctx) = oh::Monthday(y, m, d);
        })] |
    // year day (no month)
    (-year_val >> daynum)[(
        [](auto &ctx)
        {
            auto &a = x3::_attr(ctx);
            int y = fusion::at_c<0>(a) ? static_cast<int>(*fusion::at_c<0>(a)) : 0;
            char d = static_cast<char>(fusion::at_c<1>(a));
            x3::_val(ctx) = oh::Monthday(y, 0, d);
        })] |
    // variable date (easter)
    variable_date[([](auto &ctx) { x3::_val(ctx) = oh::Monthday(); })];

inline const auto date_to = x3::rule<struct dt_tag, oh::Monthday>{"date_to"} =
    date_from[([](auto &ctx) { x3::_val(ctx) = x3::_attr(ctx); })] |
    daynum[([](auto &ctx)
            { x3::_val(ctx) = oh::Monthday(0, 0, static_cast<char>(x3::_attr(ctx))); })];

inline const auto monthday_range = x3::rule<struct mr_tag, oh::MonthdayRange>{"monthday_range"} =
    // from-to range
    (date_from >> x3::omit[-date_offset] >> '-' >> date_to >> x3::omit[-date_offset])[(
        [](auto &ctx)
        {
            auto &a = x3::_attr(ctx);
            x3::_val(ctx).from = fusion::at_c<0>(a);
            x3::_val(ctx).to = fusion::at_c<1>(a);
        })] |
    // single date + offset + '+'
    (date_from >> x3::omit[-date_offset] >> x3::lit('+'))[(
        [](auto &ctx) { x3::_val(ctx) = oh::MonthdayRange(oh::Monthday(-1), oh::Monthday{}); })] |
    // single date with optional offset (no '+')
    (date_from >> x3::omit[-date_offset])[(
        [](auto &ctx) { x3::_val(ctx) = oh::MonthdayRange(x3::_attr(ctx), oh::Monthday{}); })];

inline const auto monthday_selector =
    x3::rule<struct mdsel_tag, std::vector<oh::MonthdayRange>>{"monthday_selector"} =
        monthday_range % ',';

// --- Year rules ---

// Collapse year-end alternatives to avoid variant
inline const auto year_end_value = x3::rule<struct ye_tag, unsigned>{"year_end"} =
    ('-' >> year_val >> x3::omit[-('/' >> x3::uint_)]) |
    (x3::lit('+') >> x3::attr(static_cast<unsigned>(-1)));

inline const auto year_range = x3::rule<struct yr_tag, oh::MonthdayRange>{"year_range"} =
    (year_val >> -year_end_value)[(
        [](auto &ctx)
        {
            auto &a = x3::_attr(ctx);
            auto from_year = static_cast<int>(fusion::at_c<0>(a));
            oh::MonthdayRange result(oh::Monthday(from_year), oh::Monthday{});
            if (auto &opt = fusion::at_c<1>(a))
            {
                result.to = oh::Monthday(static_cast<int>(*opt));
            }
            x3::_val(ctx) = result;
        })];

inline const auto year_selector =
    x3::rule<struct ysel_tag, std::vector<oh::MonthdayRange>>{"year_selector"} = year_range % ',';

// --- Selector composition ---

inline const auto selector_sequence = x3::rule<struct sel_seq_tag, oh>{"selector_sequence"} =
    (-monthday_selector >> -year_selector >> x3::omit[-week_selector >> -x3::lit(':')] >>
     -(weekday_selector >> !x3::char_(',')) >> -time_selector)[(
        [](auto &ctx)
        {
            auto &a = x3::_attr(ctx);
            auto &result = x3::_val(ctx);
            if (auto &md = fusion::at_c<0>(a))
                result.monthdays = std::move(*md);
            if (auto &yr = fusion::at_c<1>(a))
                result.monthdays = std::move(*yr);
            if (auto &wd = fusion::at_c<2>(a))
                result.weekdays = std::move(*wd);
            if (auto &ts = fusion::at_c<3>(a))
                result.times = std::move(*ts);
        })];

// --- Rule composition ---

inline const auto any_rule_separator = x3::rule<struct sep_tag>{"separator"} =
    x3::omit[x3::char_(';') | x3::lit("||") | x3::char_(',')];

inline const auto rule_sequence = x3::rule<struct rs_tag, oh>{"rule_sequence"} =
    x3::lit("24/7")[([](auto &ctx) { x3::_val(ctx).modifier = oh::is24_7; })] |
    (selector_sequence >> -rule_modifier >> x3::omit[-comment])[(
        [](auto &ctx)
        {
            auto &a = x3::_attr(ctx);
            x3::_val(ctx) = std::move(fusion::at_c<0>(a));
            if (auto &mod = fusion::at_c<1>(a))
                x3::_val(ctx).modifier = *mod;
        })] |
    comment;

inline const auto time_domain = x3::rule<struct td_tag, std::vector<oh>>{"time_domain"} =
    rule_sequence % any_rule_separator;

} // namespace detail

std::vector<OpeningHours> ParseOpeningHours(const std::string &str)
{
    namespace x3 = boost::spirit::x3;
    auto it(str.begin()), end(str.end());

    std::vector<OpeningHours> result;
    bool ok = x3::phrase_parse(it, end, detail::time_domain, x3::blank, result);

    if (!ok || it != end)
        return std::vector<OpeningHours>();

    return result;
}

bool CheckOpeningHours(const std::vector<OpeningHours> &input, const struct tm &time)
{
    bool is_open = false;
    for (auto &opening_hours : input)
    {
        if (opening_hours.modifier == OpeningHours::is24_7)
            return true;

        if (opening_hours.IsInRange(time))
        {
            is_open = opening_hours.modifier == OpeningHours::open;
        }
    }

    return is_open;
}

} // namespace osrm::util
